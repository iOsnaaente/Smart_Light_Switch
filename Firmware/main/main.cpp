#include "../board_config.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#include "esp_random.h"
#include "lamp_controller.h"
#include "LDR/LDR_sensor.h"
#include "display_manager.h"

// Imagens agora encapsuladas em ui/display_manager.cpp
static const char *TAG = "ILI9340_EXAMPLE";

static constexpr TickType_t IMAGE_SWITCH_INTERVAL = pdMS_TO_TICKS(5ULL * 60ULL * 1000ULL);

static constexpr int64_t MAINS_HALF_CYCLE_US_NOMINAL = 8333;
static constexpr int64_t MAINS_HALF_CYCLE_US_MIN = 7000;
static constexpr int64_t MAINS_HALF_CYCLE_US_MAX = 10000;
static constexpr int TRIAC_GATE_PULSE_US = 120;
static constexpr TickType_t LDR_CONTROL_PERIOD = pdMS_TO_TICKS(100);

static LampController s_lamp_controller;
static LDRSensor s_ldr_sensor;

static float clamp01(float value) {
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

static bool init_ldr_sensor() {
    esp_err_t err = s_ldr_sensor.init(
        ADC_UNIT_1,
        ADC_CHANNEL_0,
        50,
        ADC_ATTEN_DB_12,
        ADC_BITWIDTH_DEFAULT,
        0.2f
    );
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao inicializar o LDR: %s", esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(TAG, "LDR configurado no GPIO %d", (int)PIN_NUM_SENSOR_LDR);
    return true;
}

static void ldr_control_task(void *arg) {
    auto *lamp_controller = static_cast<LampController *>(arg);
    while (true) {
        float normalized = 0.0f;
        if (s_ldr_sensor.read_normalized(&normalized) == ESP_OK && lamp_controller != nullptr) {
            lamp_controller->set_setpoint(clamp01(normalized));
        }
        vTaskDelay(LDR_CONTROL_PERIOD);
    }
}

// Funções de display agora encapsuladas em display_manager

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Inicializando slideshow de imagens");

    if (!init_ldr_sensor()) {
        ESP_LOGE(TAG, "ADC do LDR nao foi inicializado; abortando");
        abort();
    }

    s_lamp_controller.zero_cross_edge = GPIO_INTR_ANYEDGE;
    s_lamp_controller.debounce_us = 400;
    s_lamp_controller.nominal_half_cycle_us = MAINS_HALF_CYCLE_US_NOMINAL;
    s_lamp_controller.min_half_cycle_us = MAINS_HALF_CYCLE_US_MIN;
    s_lamp_controller.max_half_cycle_us = MAINS_HALF_CYCLE_US_MAX;
    s_lamp_controller.zc_timeout_us = MAINS_HALF_CYCLE_US_NOMINAL * 2;
    s_lamp_controller.triac_gate_pulse_us = TRIAC_GATE_PULSE_US;
    s_lamp_controller.task_stack_size = 4096;
    s_lamp_controller.task_priority = 8;

    ESP_ERROR_CHECK(s_lamp_controller.init(PIN_NUM_ZEROCROSS, PIN_NUM_GATE_TRIAC));
    ESP_ERROR_CHECK(s_lamp_controller.start());

    ESP_ERROR_CHECK(s_ldr_sensor.start());
    xTaskCreate(ldr_control_task, "ldr_ctrl", 3072, &s_lamp_controller, 5, nullptr);

    ESP_ERROR_CHECK(display_manager_init());

    size_t image_count = display_manager_get_image_count();
    size_t image_index = esp_random() % image_count;

    while (true) {
        image_index = (image_index + 1) % image_count;
        display_manager_show_image(image_index);
        vTaskDelay(IMAGE_SWITCH_INTERVAL);
    }
}
