#include "event_bus.h"

#include "esp_log.h"

ESP_EVENT_DEFINE_BASE(SMART_SWITCH_EVENT);

static const char *TAG = "EVENT_BUS";

static esp_event_loop_handle_t s_loop = nullptr;

esp_err_t event_bus_init(void) {
    if (s_loop != nullptr) {
        return ESP_OK;
    }

    esp_event_loop_args_t loop_args = {};
    loop_args.queue_size = 32;
    loop_args.task_name = "event_bus";
    loop_args.task_priority = 5;
    loop_args.task_stack_size = 4096;
    loop_args.task_core_id = tskNO_AFFINITY;

    esp_err_t err = esp_event_loop_create(&loop_args, &s_loop);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao criar o event loop: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Event bus inicializado");
    return ESP_OK;
}

esp_event_loop_handle_t event_bus_get_loop(void) {
    return s_loop;
}

esp_err_t event_bus_post(smart_switch_event_id_t id, const void *event_data, size_t event_size, TickType_t ticks_to_wait) {
    if (s_loop == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }
    return esp_event_post_to(s_loop, SMART_SWITCH_EVENT, (int32_t)id, event_data, event_size, ticks_to_wait);
}

esp_err_t event_bus_register(smart_switch_event_id_t id, esp_event_handler_t handler, void *handler_arg) {
    if (s_loop == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }
    return esp_event_handler_register_with(s_loop, SMART_SWITCH_EVENT, (int32_t)id, handler, handler_arg);
}

esp_err_t event_bus_unregister(smart_switch_event_id_t id, esp_event_handler_t handler) {
    if (s_loop == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }
    return esp_event_handler_unregister_with(s_loop, SMART_SWITCH_EVENT, (int32_t)id, handler);
}
