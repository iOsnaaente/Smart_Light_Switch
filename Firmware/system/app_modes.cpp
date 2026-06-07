#include "app_modes.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_log.h"

#include "app_events.h"
#include "event_bus.h"

static const char *TAG = "APP_MODES";

static SemaphoreHandle_t s_lock = nullptr;
static app_mode_t s_mode = APP_MODE_MANUAL;
static bool s_initialized = false;

esp_err_t app_modes_init(app_mode_t initial_mode) {
    if (s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    s_lock = xSemaphoreCreateMutex();
    if (s_lock == nullptr) {
        return ESP_ERR_NO_MEM;
    }

    s_mode = initial_mode;
    s_initialized = true;

    ESP_LOGI(TAG, "Modo inicial: %d", (int)initial_mode);
    return ESP_OK;
}

esp_err_t app_modes_set(app_mode_t mode) {
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    bool changed = false;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    changed = (s_mode != mode);
    s_mode = mode;
    xSemaphoreGive(s_lock);

    if (changed) {
        ESP_LOGI(TAG, "Modo alterado para %d", (int)mode);
        event_mode_changed_t evt = { .mode = mode };
        event_bus_post(SMART_SWITCH_EVENT_MODE_CHANGED, &evt, sizeof(evt), pdMS_TO_TICKS(100));
    }

    return ESP_OK;
}

app_mode_t app_modes_get(void) {
    if (!s_initialized) {
        return APP_MODE_MANUAL;
    }

    xSemaphoreTake(s_lock, portMAX_DELAY);
    app_mode_t mode = s_mode;
    xSemaphoreGive(s_lock);
    return mode;
}
