#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    APP_MODE_MANUAL = 0,
    APP_MODE_AUTOMATIC,
    APP_MODE_ECO,
} app_mode_t;

esp_err_t app_modes_init(app_mode_t initial_mode);
esp_err_t app_modes_set(app_mode_t mode);
app_mode_t app_modes_get(void);

#ifdef __cplusplus
}
#endif
