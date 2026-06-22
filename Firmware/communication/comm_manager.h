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

    /* Credenciais do broker MQTT (NULL/"" = broker sem autenticação,
     * ex.: allow_anonymous true no mosquitto.conf). Quando preenchidas,
     * são repassadas ao mqtt_client via mqtt_client_config_t. */
    const char *mqtt_username;
    const char *mqtt_password;
} comm_manager_config_t;

/* Orquestra o ciclo completo de conectividade:
 *   NVS sem credenciais -> provisionamento BLE -> Wi-Fi -> MQTT
 *
 * Se a rede salva existir mas a reconexão falhar repetidamente, o
 * wifi_manager reabre o provisionamento BLE (sem subir AP — BLE e STA
 * coexistem no mesmo rádio) para receber credenciais novas.
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
