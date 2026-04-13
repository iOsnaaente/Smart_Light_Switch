#pragma once

#include <freertos/FreeRTOS.h>
#include "esp_err.h"
#include "esp_event.h"
#include "app_events.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t event_bus_init(void);
esp_event_loop_handle_t event_bus_get_loop(void);
esp_err_t event_bus_post(smart_switch_event_id_t id, const void *event_data, size_t event_size, TickType_t ticks_to_wait);
esp_err_t event_bus_register(smart_switch_event_id_t id, esp_event_handler_t handler, void *handler_arg);
esp_err_t event_bus_unregister(smart_switch_event_id_t id, esp_event_handler_t handler);

#ifdef __cplusplus
}
#endif
