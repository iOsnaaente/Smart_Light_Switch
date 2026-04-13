#pragma once

#include <stdint.h>
#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

ESP_EVENT_DECLARE_BASE(SMART_SWITCH_EVENT);

typedef enum {
    SMART_SWITCH_EVENT_LDR_UPDATE = 0,
    SMART_SWITCH_EVENT_DIMMER_UPDATE,
    SMART_SWITCH_EVENT_SETPOINT_CHANGED,
    SMART_SWITCH_EVENT_MODE_CHANGED,
    SMART_SWITCH_EVENT_POWER_UPDATE,
    SMART_SWITCH_EVENT_NET_STATUS,
} smart_switch_event_id_t;

typedef struct {
    float normalized;
    float voltage;
} event_ldr_update_t;

typedef struct {
    float duty;
    int64_t delay_us;
} event_dimmer_update_t;

typedef struct {
    float setpoint;
} event_setpoint_changed_t;

typedef struct {
    float active_power_w;
    float energy_wh;
} event_power_update_t;

#ifdef __cplusplus
}
#endif
