#pragma once

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "ili9340.h"

esp_err_t display_manager_init(void);
size_t display_manager_get_image_count(void);
esp_err_t display_manager_show_image(size_t index);
const char *display_manager_get_image_name(size_t index);
