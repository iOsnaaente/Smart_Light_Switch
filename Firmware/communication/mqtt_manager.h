#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *broker_uri;
    const char *topic_set;
    const char *topic_state;
} mqtt_client_config_t;

esp_err_t mqtt_client_init(const mqtt_client_config_t *config);
esp_err_t mqtt_client_start(void);
esp_err_t mqtt_client_stop(void);
esp_err_t mqtt_client_publish_state(float brightness);

#ifdef __cplusplus
}
#endif
