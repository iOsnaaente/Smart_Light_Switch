#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool start_ap_fallback;
    const char *ap_ssid;
    const char *ap_pass;
} wifi_manager_config_t;

typedef enum {
    WIFI_MANAGER_STATE_IDLE = 0,
    WIFI_MANAGER_STATE_CONNECTING,
    WIFI_MANAGER_STATE_CONNECTED,
    WIFI_MANAGER_STATE_AP_FALLBACK,
    WIFI_MANAGER_STATE_FAILED,
} wifi_manager_state_t;

esp_err_t wifi_manager_init(const wifi_manager_config_t *config);
esp_err_t wifi_manager_start(void);
esp_err_t wifi_manager_stop(void);

bool wifi_manager_is_connected(void);
wifi_manager_state_t wifi_manager_get_state(void);
int8_t wifi_manager_get_rssi(void);
esp_err_t wifi_manager_get_ip(char *out_ip, size_t out_len);

/* Credenciais persistidas em NVS (namespace "wifi_cfg"). */
bool wifi_manager_has_credentials(void);
esp_err_t wifi_manager_save_credentials(const char *ssid, const char *password, int32_t user_id);
esp_err_t wifi_manager_clear_credentials(void);
esp_err_t wifi_manager_load_user_id(int32_t *out_user_id);

/* Conecta de forma síncrona (bloqueante) com as credenciais informadas, sem persistir.
 * Usado pelo fluxo de provisionamento BLE para validar SSID/senha antes de gravar na NVS.
 * Retorna ESP_OK se a conexão foi estabelecida dentro de timeout_ms. */
esp_err_t wifi_manager_connect_with(const char *ssid, const char *password, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif
