#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t control_stack_words;
    uint32_t ui_stack_words;
    uint32_t network_stack_words;
    uint32_t dimmer_stack_words;
    uint32_t control_priority;
    uint32_t ui_priority;
    uint32_t network_priority;
    uint32_t dimmer_priority;
} app_tasks_config_t;

esp_err_t app_tasks_start(const app_tasks_config_t *config);
esp_err_t app_tasks_stop(void);

#ifdef __cplusplus
}
#endif
