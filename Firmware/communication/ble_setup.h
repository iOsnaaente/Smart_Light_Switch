#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Provisionamento Wi-Fi via BLE (servidor GATT - Bluedroid).
 *
 * Anuncia-se como "SmartLight-XXXX" (sufixo derivado do MAC) e expõe um
 * serviço com duas características:
 *   - Credenciais (escrita): recebe um JSON {"ssid":"...","password":"...","user_id":N}
 *   - Status (leitura/notificação): reporta {"status":"idle|connecting|connected|failed","device_id":"..."}
 *
 * Ao validar a conexão Wi-Fi com sucesso, as credenciais são persistidas na
 * NVS (via wifi_manager) e o serviço BLE é encerrado (ble_setup_stop) para
 * liberar memória e o rádio.
 */

esp_err_t ble_setup_init(void);
esp_err_t ble_setup_start(void);
esp_err_t ble_setup_stop(void);
bool ble_setup_is_running(void);

#ifdef __cplusplus
}
#endif
