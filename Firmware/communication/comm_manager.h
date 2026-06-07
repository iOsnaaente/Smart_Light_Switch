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

    /* URI do broker MQTT (ex.: "mqtt://192.168.1.10:1883"). Ignorado se enable_mqtt = false. */
    const char *mqtt_broker_uri;

    /* SSID/senha do AP de emergência usado pelo wifi_manager caso a
     * reconexão à rede salva falhe repetidamente. */
    const char *wifi_ap_fallback_ssid;
    const char *wifi_ap_fallback_pass;
} comm_manager_config_t;

/* Orquestra o ciclo completo de conectividade:
 *   NVS sem credenciais -> provisionamento BLE -> Wi-Fi -> MQTT
 *
 * comm_manager_init prepara NVS/netif/event loops e os submódulos
 * (wifi_manager, ble_setup, mqtt_client) de acordo com os flags habilitados.
 * comm_manager_start dispara a sequência de conexão em uma task dedicada,
 * deixando o restante do sistema desacoplado via event_bus. */
esp_err_t comm_manager_init(const comm_manager_config_t *config);
esp_err_t comm_manager_start(void);
esp_err_t comm_manager_stop(void);

#ifdef __cplusplus
}
#endif
