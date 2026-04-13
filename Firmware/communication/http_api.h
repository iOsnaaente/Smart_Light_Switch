#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float brightness;
    int mode;
    float power_w;
} http_api_state_t;

esp_err_t http_api_start(void);
esp_err_t http_api_stop(void);
esp_err_t http_api_get_state(http_api_state_t *out_state);
esp_err_t http_api_set_brightness(float brightness);

#ifdef __cplusplus
}
#endif
