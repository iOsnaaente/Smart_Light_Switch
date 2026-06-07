# Roadmap — Ciclo de Conectividade (BLE → Wi-Fi → MQTT)

Resumo das mudanças implementadas para colocar o firmware do Smart Light Switch
para compilar com o ciclo completo de conectividade orquestrado.

## Código novo / alterado

- **`communication/comm_manager.h` / `comm_manager.cpp`** (novo)
  Orquestrador da sequência de conectividade:
  1. Inicializa NVS e o `event_bus`.
  2. Verifica o botão de reset de provisionamento (`PIN_NUM_PROVISION_RESET`,
     GPIO18) no boot — se pressionado, apaga as credenciais Wi-Fi salvas.
  3. Sem credenciais salvas → inicia o provisionamento BLE (`ble_setup_start`)
     e aguarda o recebimento e persistência das credenciais na NVS.
  4. Com credenciais → conecta ao Wi-Fi (`wifi_manager_start`) e aguarda a
     conexão.
  5. Conectado → inicia o cliente MQTT (`mqtt_client_start`), se habilitado.
  Expõe a API pública `comm_manager_init / start / stop`.

- **`communication/CMakeLists.txt`** (reescrito)
  Componente `communication` agora compila `ble_setup.cpp`,
  `wifi_manager.cpp`, `mqtt_client.cpp` e `comm_manager.cpp`, com
  `REQUIRES`: `system bt esp_wifi esp_netif esp_event esp_timer mqtt json
  nvs_flash driver freertos`.

- **`communication/communication_stub.cpp`** — removido (`git rm`), substituído
  pela implementação real.

- **`main/CMakeLists.txt`** — adicionado `communication` em `REQUIRES`.

- **`main/main.cpp`**
  - `#include "comm_manager.h"` e constantes de configuração:
    `MQTT_BROKER_URI`, `WIFI_AP_FALLBACK_SSID`, `WIFI_AP_FALLBACK_PASS`.
  - No início do `app_main()`, monta `comm_manager_config_t` (BLE + MQTT
    habilitados, HTTP desabilitado) e chama `comm_manager_init` /
    `comm_manager_start`.

- **`board_config.h`**
  - Corrigido conflito de pino: `PIN_NUM_PROVISION_RESET` estava definido como
    `GPIO_NUM_9`, colidindo com `PIN_NUM_RST` (reset do display, também GPIO9).
    Alterado para `GPIO_NUM_18` (livre no mapeamento atual).

## Ajustes de configuração (sdkconfig)

Necessários para o stack Bluedroid/BLE usado pelo provisionamento compilar e
linkar:

- `CONFIG_BT_BLE_ENABLED=y`
- `CONFIG_BT_BLE_42_FEATURES_SUPPORTED=y` (habilita também, via
  `idf.py reconfigure`, `CONFIG_BT_BLE_42_ADV_EN` e `CONFIG_BT_BLE_42_SCAN_EN`,
  necessários para `esp_ble_gap_start_advertising` /
  `esp_ble_gap_config_adv_data` / `esp_ble_gap_stop_advertising`)

## Tabela de partições e tamanho de flash

O binário final cresceu para ~2,1–2,8 MB com o stack Bluedroid habilitado,
estourando a partição `factory` padrão de 1500 KB (em flash de 2 MB). Solução:

- **`partitions.csv`** (novo, na raiz do `Firmware/`): tabela customizada com
  partição `factory` de **3200 KB**:
  ```
  # Name,   Type, SubType, Offset,  Size, Flags
  nvs,       data, nvs,     0x9000,  0x6000,
  phy_init,  data, phy,     0xf000,  0x1000,
  factory,   app,  factory, 0x10000, 0x320000,
  ```
- `CONFIG_PARTITION_TABLE_CUSTOM=y` / `CONFIG_PARTITION_TABLE_FILENAME="partitions.csv"`
- `CONFIG_ESPTOOLPY_FLASHSIZE` alterado de `2MB` para `4MB`

## Resultado da build

```
Firmware.bin binary size 0x212d90 bytes (~2,13 MB)
Smallest app partition is 0x320000 bytes (3200 KB)
0x10d270 bytes (34%) livres
Project build complete.
```

## Pendência para o usuário

- `MQTT_BROKER_URI` em `main/main.cpp` está fixo em
  `mqtt://192.168.1.100:1883` — ajustar para o endereço real do broker
  Mosquitto na rede local antes de gravar o firmware.
