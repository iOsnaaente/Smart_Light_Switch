#include "../board_config.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_timer.h>

#include "esp_random.h"

extern "C" {
#include "ili9340.h"
}

#include "images/image_olharpredador.h"
#include "images/image_lindademais.h"
#include "images/image_milkshake.h"
#include "images/image_formanda.h"
#include "images/image_skincare.h"
#include "images/image_natal.h"
#include "images/image_noiva.h"
#include "images/image_linda.h"
// #include "images/image_linguinha.h"
// #include "images/image_beijinho.h"
// #include "images/image_dedinho.h"
// #include "images/image_paz.h"

static const char *TAG = "ILI9340_EXAMPLE";

static TFT_t s_dev = {};
static constexpr int64_t IMAGE_SWITCH_INTERVAL_US = 5LL * 60LL * 1000000LL;

// Image display buffer for optimized drawing
#define IMAGE_BUFFER_SIZE 512  // Number of pixels to write at once (512 = 1KB per transfer)
static uint16_t s_image_buffer[IMAGE_BUFFER_SIZE];

struct ImageEntry {
    const char *name;
    const uint8_t *data;
    int width;
    int height;
};

static const ImageEntry s_images[] = {
    {"Formanda", image_formanda_data, IMAGE_FORMANDA_WIDTH, IMAGE_FORMANDA_HEIGHT},
    {"Linda", image_linda_data, IMAGE_LINDA_WIDTH, IMAGE_LINDA_HEIGHT},
    {"LindaDeMais", image_lindademais_data, IMAGE_LINDADEMAIS_WIDTH, IMAGE_LINDADEMAIS_HEIGHT},
    {"Milkshake", image_milkshake_data, IMAGE_MILKSHAKE_WIDTH, IMAGE_MILKSHAKE_HEIGHT},
    {"Natal", image_natal_data, IMAGE_NATAL_WIDTH, IMAGE_NATAL_HEIGHT},
    {"Noiva", image_noiva_data, IMAGE_NOIVA_WIDTH, IMAGE_NOIVA_HEIGHT},
    {"OlharPredador", image_olharpredador_data, IMAGE_OLHARPREDADOR_WIDTH, IMAGE_OLHARPREDADOR_HEIGHT},
    // {"Paz", image_paz_data, IMAGE_PAZ_WIDTH, IMAGE_PAZ_HEIGHT},
    // {"Linguinha", image_linguinha_data, IMAGE_LINGUINHA_WIDTH, IMAGE_LINGUINHA_HEIGHT},
    // {"Beijinho", image_beijinho_data, IMAGE_BEIJINHO_WIDTH, IMAGE_BEIJINHO_HEIGHT},
    // {"Dedinho", image_dedinho_data, IMAGE_DEDINHO_WIDTH, IMAGE_DEDINHO_HEIGHT},
    {"SkinCare", image_skincare_data, IMAGE_SKINCARE_WIDTH, IMAGE_SKINCARE_HEIGHT}
};

static constexpr size_t IMAGE_COUNT = sizeof(s_images) / sizeof(s_images[0]);
static uint16_t image_rgb565_to_panel(uint16_t color) {
    const uint16_t red = (color >> 11) & 0x1F;
    const uint16_t green = (color >> 5) & 0x3F;
    const uint16_t blue = color & 0x1F;
    return (blue << 11) | (green << 5) | red;
}

// Optimized image drawing - uses bulk pixel writes for speed
static void display_image(const uint8_t *image_data, int width, int height) {
    const bool rotate_to_landscape = (width == LCD_V_RES && height == LCD_H_RES);
    const int output_width = rotate_to_landscape ? height : width;
    const int output_height = rotate_to_landscape ? width : height;
    ESP_LOGI(TAG, "Renderizando imagem %dx%d em area %dx%d", width, height, output_width, output_height);
    for (int y = 0; y < output_height; y++) {
        for (int x = 0; x < output_width; x += IMAGE_BUFFER_SIZE) {
            const int pixels_this_batch = (x + IMAGE_BUFFER_SIZE > output_width)
                                          ? (output_width - x)
                                          : IMAGE_BUFFER_SIZE;
            for (int i = 0; i < pixels_this_batch; i++) {
                int source_x = x + i;
                int source_y = y;
                if (rotate_to_landscape) {
                    source_x = y;
                    source_y = height - 1 - (x + i);
                }
                const int pixel_linear_idx = source_y * width + source_x;
                const int byte_idx = pixel_linear_idx * 2;
                const uint16_t color = image_data[byte_idx] | (image_data[byte_idx + 1] << 8);
                s_image_buffer[i] = image_rgb565_to_panel(color);
            }
            lcdDrawMultiPixels(&s_dev, x, y, pixels_this_batch, s_image_buffer);
        }
    }

    lcdDrawFinish(&s_dev);
    ESP_LOGI(TAG, "Imagem renderizada com sucesso");
}

static void show_image_by_index(size_t index) {
    const ImageEntry &img = s_images[index];
    ESP_LOGI(TAG, "Exibindo imagem %s (%d/%d)", img.name, (int)(index + 1), (int)IMAGE_COUNT);
    display_image(img.data, img.width, img.height);
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Inicializando slideshow de imagens");
    spi_master_init(
        &s_dev,
        PIN_NUM_MOSI,
        PIN_NUM_CLK,
        PIN_NUM_CS,
        PIN_NUM_DC,
        PIN_NUM_RST,
        -1,
        PIN_NUM_MISO,
        PIN_NUM_T_CS,
        PIN_NUM_T_IRQ,
        -1,
        -1
    );
    lcdInit(&s_dev, TFT_MODEL, LCD_H_RES, LCD_V_RES, 0, 0);

    lcdFillScreen(&s_dev, BLACK);
    lcdDrawFinish(&s_dev);

    size_t image_index = esp_random() % IMAGE_COUNT;

    while (true) {
        image_index = (image_index + 1) % IMAGE_COUNT;
        show_image_by_index(image_index);
        vTaskDelay(pdMS_TO_TICKS(IMAGE_SWITCH_INTERVAL_US / 1000));
    }
}
