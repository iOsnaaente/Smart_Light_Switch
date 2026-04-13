#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    POWER_METER_SOURCE_ESTIMATION = 0,
    POWER_METER_SOURCE_HLW8110 = 1,
} power_meter_source_t;

typedef struct {
    power_meter_source_t source;
    bool enable_energy_accumulator;
    float nominal_lamp_watts;
} power_meter_config_t;

typedef struct {
    float voltage_rms;
    float current_rms;
    float active_power_w;
    float apparent_power_va;
    float power_factor;
    float energy_wh;
} power_meter_snapshot_t;

esp_err_t power_meter_init(const power_meter_config_t *config);
esp_err_t power_meter_update_from_control(float dimmer_percent);
esp_err_t power_meter_read(power_meter_snapshot_t *out_snapshot);

#ifdef __cplusplus
}
#endif
