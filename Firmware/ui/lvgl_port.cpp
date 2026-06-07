#include "lvgl_port.h"

#include "../board_config.h"
#include "ili9340.h"
#include "lvgl.h"

#include "freertos/semphr.h"

#include <esp_log.h>
#include <esp_timer.h>

static const char *TAG = "LVGL_PORT";

static constexpr int32_t     LVGL_DRAW_BUF_LINES  = 40;     // ~37 KB (240*40*2B) — varredura completa em 8 flushes
static constexpr uint16_t    SPI_CHUNK_PIXELS     = 512;    // limite de Byte[1024] em spi_master_write_colors (2B/pixel)
static constexpr uint32_t    LVGL_TASK_PERIOD_MS  = 10;
static constexpr uint32_t    LVGL_TASK_STACK_SIZE = 1024 * 8;
static constexpr UBaseType_t LVGL_TASK_PRIORITY   = 5;      // mesma prioridade das demais tasks de aplicação

static TFT_t             s_dev = {};
static lv_display_t     *s_display = nullptr;
static lv_indev_t       *s_indev = nullptr;
static SemaphoreHandle_t s_lock = nullptr;
static lv_color_t        s_draw_buf[(size_t)LCD_WIDTH * LVGL_DRAW_BUF_LINES];

/**
 * @brief   O painel está em modo BGR (MADCTL = 0x08), mas o LVGL renderiza em
 *          RGB565 "puro" — troca os campos R<->B mantendo G, espelhando a
 *          antiga image_rgb565_to_panel() de display_manager.cpp.
 */
static inline uint16_t rgb565_to_bgr_panel(uint16_t c) {
    return (uint16_t)(((c & 0x001F) << 11) | (c & 0x07E0) | ((c & 0xF800) >> 11));
}

/**
 * @brief   Callback de flush do LVGL: desenha o retângulo "sujo" (area) lendo
 *          pixels já renderizados em px_map. Define a janela de endereçamento
 *          (CASET/PASET/RAMWR) UMA vez para o retângulo inteiro — o
 *          controlador então auto-incrementa por linha — e envia os pixels em
 *          blocos de até SPI_CHUNK_PIXELS, já convertidos para BGR.
 */
static void disp_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    const uint16_t *src = (const uint16_t *)px_map;
    const uint32_t total = (uint32_t)lv_area_get_width(area) * (uint32_t)lv_area_get_height(area);

    spi_master_write_comm_byte(&s_dev, 0x2A); // set column (x) address
    spi_master_write_addr(&s_dev, area->x1 + s_dev._offsetx, area->x2 + s_dev._offsetx);
    spi_master_write_comm_byte(&s_dev, 0x2B); // set page (y) address
    spi_master_write_addr(&s_dev, area->y1 + s_dev._offsety, area->y2 + s_dev._offsety);
    spi_master_write_comm_byte(&s_dev, 0x2C); // memory write

    static uint16_t chunk[SPI_CHUNK_PIXELS];
    for (uint32_t sent = 0; sent < total;) {
        uint32_t n = total - sent;
        if (n > SPI_CHUNK_PIXELS) {
            n = SPI_CHUNK_PIXELS;
        }
        for (uint32_t i = 0; i < n; i++) {
            chunk[i] = rgb565_to_bgr_panel(src[sent + i]);
        }
        spi_master_write_colors(&s_dev, chunk, (uint16_t)n);
        sent += n;
    }

    lv_display_flush_ready(disp);
}

/**
 * @brief   Lê o XPT2046 (valores brutos de 12 bits do ADC — não existe rotina
 *          de calibração no driver) e mapeia linearmente para coordenadas de
 *          tela via TOUCH_RAW_*_MIN/MAX (board_config.h). Esses limites são
 *          "chutes" de partida, então o resultado é grampeado em [0, LCD_*-1]
 *          — leituras perto da borda física podem extrapolar a faixa.
 */
static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data) {
    int xp = 0, yp = 0;
    if (!touch_getxy(&s_dev, &xp, &yp)) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    int32_t x = (xp - TOUCH_RAW_X_MIN) * LCD_WIDTH  / (TOUCH_RAW_X_MAX - TOUCH_RAW_X_MIN);
    int32_t y = (yp - TOUCH_RAW_Y_MIN) * LCD_HEIGHT / (TOUCH_RAW_Y_MAX - TOUCH_RAW_Y_MIN);
    if (x < 0) x = 0; else if (x >= LCD_WIDTH)  x = LCD_WIDTH  - 1;
    if (y < 0) y = 0; else if (y >= LCD_HEIGHT) y = LCD_HEIGHT - 1;

    data->point.x = x;
    data->point.y = y;
    data->state = LV_INDEV_STATE_PRESSED;
}

/**
 * @brief   "Coração" do LVGL: avança o tick pelo tempo realmente decorrido
 *          (esp_timer, em vez de um esp_timer dedicado — uma fonte a menos
 *          para sincronizar) e despacha lv_timer_handler() sob o lock, que
 *          renderiza, drena entradas de toque e dispara os event callbacks
 *          das telas.
 */
static void lvgl_task(void *arg) {
    int64_t last_us = esp_timer_get_time();
    while (true) {
        const int64_t now_us = esp_timer_get_time();
        lv_tick_inc((uint32_t)((now_us - last_us) / 1000));
        last_us = now_us;

        if (lvgl_port_lock(50)) {
            lv_timer_handler();
            lvgl_port_unlock();
        }
        vTaskDelay(pdMS_TO_TICKS(LVGL_TASK_PERIOD_MS));
    }
}

esp_err_t lvgl_port_init(void) {
    if (s_lock != nullptr) {
        return ESP_ERR_INVALID_STATE;
    }

    // Recursivo: lv_timer_handler() roda sob o lock e pode disparar
    // event callbacks das telas que, por sua vez, chamam lv_obj_*/lvgl_port_lock
    // de novo na MESMA task — um mutex comum bloquearia (deadlock) nesse caso.
    s_lock = xSemaphoreCreateRecursiveMutex();
    if (s_lock == nullptr) {
        return ESP_ERR_NO_MEM;
    }

    spi_master_init(&s_dev,
                    PIN_NUM_MOSI, PIN_NUM_CLK, PIN_NUM_CS, PIN_NUM_DC, PIN_NUM_RST, -1,
                    PIN_NUM_MISO, PIN_NUM_T_CS, PIN_NUM_T_IRQ, -1, -1);
    lcdInit(&s_dev, TFT_MODEL, LCD_WIDTH, LCD_HEIGHT, 0, 0);
    lcdFillScreen(&s_dev, BLACK);
    lcdDrawFinish(&s_dev);

    lv_init();

    s_display = lv_display_create(LCD_WIDTH, LCD_HEIGHT);
    lv_display_set_color_format(s_display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(s_display, s_draw_buf, nullptr, sizeof(s_draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(s_display, disp_flush_cb);

    s_indev = lv_indev_create();
    lv_indev_set_type(s_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_display(s_indev, s_display);
    lv_indev_set_read_cb(s_indev, touch_read_cb);

    ESP_LOGI(TAG, "Painel %dx%d (retrato) e touch XPT2046 prontos para o LVGL", LCD_WIDTH, LCD_HEIGHT);
    return ESP_OK;
}

esp_err_t lvgl_port_start(void) {
    if (s_lock == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }

    BaseType_t ok = xTaskCreate(lvgl_task, "lvgl", LVGL_TASK_STACK_SIZE, nullptr, LVGL_TASK_PRIORITY, nullptr);
    return (ok == pdPASS) ? ESP_OK : ESP_ERR_NO_MEM;
}

bool lvgl_port_lock(uint32_t timeout_ms) {
    if (s_lock == nullptr) {
        return false;
    }
    TickType_t ticks = (timeout_ms == 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(s_lock, ticks) == pdTRUE;
}

void lvgl_port_unlock(void) {
    if (s_lock != nullptr) {
        xSemaphoreGiveRecursive(s_lock);
    }
}
