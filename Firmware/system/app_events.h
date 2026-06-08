#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_event.h"
#include "app_modes.h"

#ifdef __cplusplus
extern "C" {
#endif

ESP_EVENT_DECLARE_BASE(SMART_SWITCH_EVENT);

typedef enum {
    SMART_SWITCH_EVENT_LDR_UPDATE = 0,
    SMART_SWITCH_EVENT_DIMMER_UPDATE,
    SMART_SWITCH_EVENT_SETPOINT_CHANGED,
    SMART_SWITCH_EVENT_MODE_CHANGED,
    SMART_SWITCH_EVENT_POWER_UPDATE,
    SMART_SWITCH_EVENT_NET_STATUS,
    SMART_SWITCH_EVENT_RELAY_COMMAND,
    SMART_SWITCH_EVENT_DIMMER_COMMAND,
    SMART_SWITCH_EVENT_BLE_PROVISION_COMMAND,
    SMART_SWITCH_EVENT_BLE_PROVISION_STATUS,
} smart_switch_event_id_t;

typedef struct {
    float normalized;
    float voltage;
} event_ldr_update_t;

typedef struct {
    float duty;
    int64_t delay_us;
} event_dimmer_update_t;

typedef struct {
    float setpoint;
} event_setpoint_changed_t;

typedef struct {
    float voltage_rms;
    float current_rms;
    float active_power_w;
    float energy_wh;
} event_power_update_t;

/* Estado da conectividade: publicado pelo wifi_manager / mqtt_client. */
typedef struct {
    bool wifi_connected;
    bool mqtt_connected;
    int8_t rssi;
    char ip_addr[16];
} event_net_status_t;

/* Mudança de modo de operação (Manual/Automático/Eco). */
typedef struct {
    app_mode_t mode;
} event_mode_changed_t;

/* Comando de relé recebido via devices/{user_id}/{device_id}/command -> {"relay": true|false} */
typedef struct {
    bool relay_on;
} event_relay_command_t;

/* Comando de dimmer recebido via devices/{user_id}/{device_id}/command -> {"dimmer": 0..100} */
typedef struct {
    uint8_t value;
} event_dimmer_command_t;

/* Pedido para ligar/desligar o pareamento BLE (provisionamento de Wi-Fi +
 * user_id) — disparado pelo toque na linha "Wi-Fi" da tela de Ajustes (ou
 * por um comando externo equivalente). O comm_manager trata este evento
 * chamando ble_setup_start()/ble_setup_stop(). */
typedef struct {
    bool enable;
} event_ble_provision_command_t;

/* Eco do estado real do serviço de pareamento BLE — publicado por ble_setup
 * ao iniciar/encerrar o anúncio (inclusive quando ele se encerra sozinho
 * após receber e validar credenciais com sucesso), para a UI refletir o
 * que de fato está acontecendo no rádio. */
typedef struct {
    bool active;
} event_ble_provision_status_t;

#ifdef __cplusplus
}
#endif
