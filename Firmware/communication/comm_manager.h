#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool enable_http;
    bool enable_mqtt;
    bool enable_ble;
} comm_manager_config_t;

esp_err_t comm_manager_init(const comm_manager_config_t *config);
esp_err_t comm_manager_start(void);
esp_err_t comm_manager_stop(void);

#ifdef __cplusplus
}
#endif
