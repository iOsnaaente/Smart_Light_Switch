#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Cliente MQTT (esp-mqtt) que conversa com o broker Mosquitto do backend.
 *
 * Tópicos (contrato app/firmware/backend, ver Backend/README.md):
 *   devices/{user_id}/{device_id}/command    backend -> dispositivo
 *   devices/{user_id}/{device_id}/state      dispositivo -> backend
 *   devices/{user_id}/{device_id}/telemetry  dispositivo -> backend
 *
 * device_id é derivado do MAC (ex.: "switch-a1b2c3") e user_id é o dono do
 * dispositivo, recebido durante o provisionamento BLE e persistido na NVS
 * (ver wifi_manager_save_credentials / wifi_manager_load_user_id).
 */

typedef struct {
    const char *broker_uri;            /* ex.: "mqtt://192.168.1.10:1883"            */
    const char *mqtt_username;         /* usuário do broker (NULL/"" = sem autenticação) */
    const char *mqtt_password;         /* senha correspondente a mqtt_username           */
    uint32_t state_heartbeat_ms;       /* intervalo do heartbeat de estado (ex. 30s) */
    uint32_t telemetry_interval_ms;    /* intervalo de telemetria (ex. 5-10s)        */
} mqtt_client_config_t;

esp_err_t mqtt_client_init(const mqtt_client_config_t *config);
esp_err_t mqtt_client_start(void);
esp_err_t mqtt_client_stop(void);

bool mqtt_client_is_connected(void);

/* Publica o snapshot atual de estado/telemetria sob demanda (além das
 * publicações automáticas disparadas por eventos e pelos timers periódicos). */
esp_err_t mqtt_client_publish_state(void);
esp_err_t mqtt_client_publish_telemetry(void);

#ifdef __cplusplus
}
#endif
