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

---

# Interface local (LVGL) — telas do dispositivo

Port completo da UI local para LVGL v9, substituindo o antigo carrossel de
imagens (`display_manager`/`ui_stub`) por 3 telas interativas via touch,
implementadas a partir dos wireframes em `ui/wireframes/`.

## Código novo

- **`ui/ui_state.h`** — `ui_lang_t` (PT/EN) e `ui_runtime_state_t`: retrato
  único do estado exibido pelas 3 telas (modo, brilho, alvo de iluminação,
  sensibilidade, lux ambiente, status de rede/IP, idioma).

- **`ui/ui_theme.h`** — paleta de cores, fontes (`UI_FONT_BODY`/`UI_FONT_HERO`
  via `lv_font_montserrat_14/28`), raios, `UI_HAIRLINE_W`,
  `UI_AMBIENT_LUX_SCALE` e a tabela de strings PT/EN.

- **`ui/ui_widgets.{h,cpp}`** — componentes reaproveitados pelas 3 telas:
  Card, cabeçalho de tela (`ui_screen_header_t`), botão de força
  (`ui_power_btn_t`), stepper +/-, segmento PT/EN (`ui_seg2_t`), ícones
  sun/bulb e legenda.

- **`ui/lvgl_port.{h,cpp}`** — porte do driver: inicialização do painel
  ILI9341 em retrato (240×320), flush, leitura de touch, tick e task do
  LVGL, com mutex recursivo (`lvgl_port_lock`/`unlock`) para permitir que
  toques (já sob lock) e handlers do `event_bus` (em task separada)
  acessem `lv_obj_*` de forma segura.

- **`ui/screens/screen_main.{h,cpp}`** — tela Principal: cartão "Dashboard"
  com leitura de lux, stepper de intensidade e botão de força.

- **`ui/screens/screen_auto.{h,cpp}`** — tela Modo Automático: interruptor
  mestre, alvo de iluminação (+/-) e slider de sensibilidade.

- **`ui/screens/screen_settings.{h,cpp}`** — tela Ajustes: idioma (PT/EN),
  brilho da tela, calibração do sensor, status de Wi-Fi/IP e versão.

- **`ui/ui_manager.{h,cpp}`** (novo) — orquestra as 3 telas: monta-as, liga
  cada toque ao comando correspondente do sistema (`event_bus`/`app_modes`)
  e mantém o que é exibido em sincronia com o estado real assinando
  `LDR_UPDATE`/`DIMMER_UPDATE`/`MODE_CHANGED`/`SETPOINT_CHANGED`/`NET_STATUS`.

- **`ui/CMakeLists.txt`** (reescrito) — novo conjunto de fontes
  (`lvgl_port`, `ui_widgets`, `ui_manager`, `screens/*`) e `REQUIRES`
  explícito incluindo `esp_event`/`esp_driver_gpio`/`freertos`.

## `main/main.cpp`

- Removida a `display_update_task` (carrossel de imagens) — substituída pela
  inicialização da interface local: `lvgl_port_init()` → `ui_manager_init()`
  → `ui_manager_start()`.
- Adicionados `on_dimmer_command`/`on_relay_command`: assinam
  `DIMMER_COMMAND`/`RELAY_COMMAND` no `event_bus` e aplicam o setpoint
  diretamente no `triac_controller` — fechando uma lacuna pré-existente
  (nada no projeto consumia esses eventos publicados pelo `mqtt_client`).
- `lamp_control_task` agora:
  - publica `LDR_UPDATE` (leitura crua do sensor) e `DIMMER_UPDATE` (eco do
    setpoint real do triac) a cada ciclo — também lacunas pré-existentes
    (nada publicava esses eventos consumidos pelo `mqtt_client`/UI);
  - só aplica o ajuste automático por LDR (guardrail + `set_setpoint`)
    quando `app_modes_get() == APP_MODE_AUTOMATIC` — no modo Manual, quem
    decide o setpoint são os comandos acima.

## Ajustes de configuração (sdkconfig)

- `CONFIG_LV_FONT_MONTSERRAT_28=y` — fonte usada em `UI_FONT_HERO`
  (valores em destaque, como o de lux na tela Principal).

## Removido

- `ui/ui_stub.cpp`, `system/system_stub.cpp`, `ui/display_manager.{h,cpp}` e
  os recursos associados ao antigo carrossel (fotos pessoais, ferramentas de
  conversão de imagem) — fora do escopo do firmware do dispositivo.
