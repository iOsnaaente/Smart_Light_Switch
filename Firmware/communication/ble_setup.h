#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t ble_setup_init(void);
esp_err_t ble_setup_start(void);
esp_err_t ble_setup_stop(void);

#ifdef __cplusplus
}
#endif
