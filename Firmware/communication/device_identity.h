#pragma once

#include <stdio.h>
#include <stdint.h>

#include "esp_mac.h"

/* Identidade do dispositivo derivada do MAC do ESP32 (estável entre boots).
 * - device_id: usado nos tópicos MQTT, ex.: "switch-a1b2c3"
 * - ble_device_name: usado no anúncio BLE de provisionamento, ex.: "SmartLight-A1B2"
 */

static inline void device_identity_get_id(char *out_id, size_t out_len) {
    uint8_t mac[6] = {0};
    esp_efuse_mac_get_default(mac);
    snprintf(out_id, out_len, "switch-%02x%02x%02x", mac[3], mac[4], mac[5]);
}

static inline void device_identity_get_ble_name(char *out_name, size_t out_len) {
    uint8_t mac[6] = {0};
    esp_efuse_mac_get_default(mac);
    snprintf(out_name, out_len, "SmartLight-%02X%02X", mac[4], mac[5]);
}
