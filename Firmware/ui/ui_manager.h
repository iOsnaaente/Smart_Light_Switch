#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "app_modes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool start_in_manual_mode;
    float default_brightness;
} ui_manager_config_t;

esp_err_t ui_manager_init(const ui_manager_config_t *config);
esp_err_t ui_manager_start(void);
esp_err_t ui_manager_set_brightness(float brightness);
esp_err_t ui_manager_set_mode(app_mode_t mode);

#ifdef __cplusplus
}
#endif
