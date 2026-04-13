#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool start_ap_fallback;
    const char *ap_ssid;
    const char *ap_pass;
} wifi_manager_config_t;

esp_err_t wifi_manager_init(const wifi_manager_config_t *config);
esp_err_t wifi_manager_start(void);
bool wifi_manager_is_connected(void);

#ifdef __cplusplus
}
#endif
