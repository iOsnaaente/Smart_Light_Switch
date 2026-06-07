#include "../board_config.h"
#include "display_manager.h"

#include "ili9340.h"

#include "images/image_olharpredador.h"
#include "images/image_lindademais.h"
#include "images/image_milkshake.h"
#include "images/image_formanda.h"
#include "images/image_skincare.h"
#include "images/image_natal.h"
#include "images/image_noiva.h"
#include "images/image_linda.h"

#include <esp_log.h>

static const char *LOG_TAG = "DISPLAY_MGR";
static constexpr size_t IMAGE_BUFFER_SIZE = 512;
static uint16_t s_image_buffer[IMAGE_BUFFER_SIZE];
static TFT_t s_dev = {};
static bool s_initialized = false;

/**
 * @brief   Converte cor RGB565 para o formato esperado pelo painel
 */
static uint16_t image_rgb565_to_panel(uint16_t color) {
    const uint16_t red = (color >> 11) & 0x1F;
    const uint16_t green = (color >> 5) & 0x3F;
    const uint16_t blue = color & 0x1F;
    return (blue << 11) | (green << 5) | red;
}

/**
 * @brief   Renderiza uma imagem no display
 */
static void render_image(TFT_t *dev, const uint8_t *image_data, int width, int height) {
    const bool rotate_to_landscape = (width == LCD_V_RES && height == LCD_H_RES);
    const int output_width = rotate_to_landscape ? height : width;
    const int output_height = rotate_to_landscape ? width : height;

    for (int y = 0; y < output_height; y++) {
        for (int x = 0; x < output_width; x += IMAGE_BUFFER_SIZE) {
            const int pixels_this_batch = 
                (x + IMAGE_BUFFER_SIZE > output_width)
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
            lcdDrawMultiPixels(dev, x, y, pixels_this_batch, s_image_buffer);
        }
    }
    lcdDrawFinish(dev);
}

/**
 * @brief   Estrutura com dados de cada imagem
 */
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
    {"SkinCare", image_skincare_data, IMAGE_SKINCARE_WIDTH, IMAGE_SKINCARE_HEIGHT}
};

static constexpr size_t IMAGE_COUNT = sizeof(s_images) / sizeof(s_images[0]);

esp_err_t display_manager_init(void) {
    if (s_initialized) {
        return ESP_OK;
    }

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

    s_initialized = true;
    ESP_LOGI(LOG_TAG, "Display manager inicializado com %zu imagens", IMAGE_COUNT);
    return ESP_OK;
}

size_t display_manager_get_image_count(void) {
    return IMAGE_COUNT;
}

const char *display_manager_get_image_name(size_t index) {
    if (index >= IMAGE_COUNT) {
        return "INVALID";
    }
    return s_images[index].name;
}

esp_err_t display_manager_show_image(size_t index) {
    if (index >= IMAGE_COUNT) {
        ESP_LOGE(LOG_TAG, "Índice de imagem inválido: %zu (máx: %zu)", index, IMAGE_COUNT - 1);
        return ESP_ERR_INVALID_ARG;
    }

    if (!s_initialized) {
        ESP_LOGE(LOG_TAG, "Display não foi inicializado com set_device");
        return ESP_ERR_INVALID_STATE;
    }

    const ImageEntry &img = s_images[index];
    ESP_LOGI(LOG_TAG, "Exibindo: %s (%zu/%zu)", img.name, index + 1, IMAGE_COUNT);
    
    render_image(&s_dev, img.data, img.width, img.height);
    return ESP_OK;
}
