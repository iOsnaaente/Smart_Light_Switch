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

---

# Calibração de touch, gestos da tela Principal e correções de inicialização

Sessão de depuração ponta-a-ponta do touch (XPT2046) e do modelo SP/MV/PV de
controle exibido na tela Principal, incluindo um bug de inicialização que
impedia a UI de receber qualquer evento do `event_bus` desde o primeiro boot.

## `board_config.h` — recalibração do touch

- Teste de 4 quinas revelou que a calibração anterior estava completamente
  errada para a orientação atual do painel: `TOUCH_INVERT_X` estava `true`
  quando o bruto X na verdade **cresce** da esquerda para a direita (não
  precisa inverter), e os limites Y cobriam uma faixa maior que o alcance
  físico real do painel (cortava ~1/3 da tela na borda inferior).
  - `TOUCH_INVERT_X`: `true` → `false`.
  - `TOUCH_RAW_X_MIN/MAX`: `370/1415` → `150/950` (950 = mínimo bruto
    observado na borda direita, entre os cantos sup-dir e inf-dir — garante
    que ambos os cantos alcancem `x=239` via clamping).
  - `TOUCH_RAW_Y_MIN/MAX`: `210/1780` → `100/1150` (extremos reais
    observados no teste de 4 quinas).
  - `TOUCH_RAW_NOISE_MAX`: `130` → `120`, para abrir mais margem ao toque
    real na borda esquerda sem aceitar o "toque fantasma" (bruto ~94 estável
    com o painel em repouso).

## `ui/lvgl_port.{h,cpp}`

- **Toques não chegavam a nenhum widget real**: todo container puramente
  visual de `screen_main.cpp` (criado via `bare()`) herdava
  `LV_OBJ_FLAG_CLICKABLE` de `lv_obj_create` por padrão — o LVGL roteava o
  toque para o filho clicável mais próximo (camadas de gradiente, glow,
  marcadores), que absorvia o evento antes de chegar a `s_main.root`, onde
  ficam os callbacks de arrasto. Corrigido removendo também `CLICKABLE` em
  `bare()` (`screen_main.cpp`) — só `cfg_btn` reativa o flag explicitamente.
- Exposta `lvgl_port_get_raw_touch(int *xp, int *yp)`: retorna o último par
  bruto de 12 bits do ADC do XPT2046 (antes de `INVERT_X`/`SWAP_XY`),
  armazenado em `touch_read_cb` a cada leitura válida (não-fantasma). Usado
  por `ui_manager.cpp` para calcular o SP direto do bruto, sem depender da
  coordenada de tela já calibrada pelo LVGL.

## `main/main.cpp` — ordem de inicialização

- **Nenhum handler do `event_bus` registrado por `ui_manager_init()` jamais
  funcionou desde o primeiro boot do projeto**: `event_bus_init()` só era
  chamado dentro de `comm_manager_init()`, mas `app_main()` chamava
  `ui_manager_init()` (que registra `on_ldr_update`, `on_dimmer_update`,
  `on_net_status` etc. via `event_bus_register`) *antes* de
  `comm_manager_init()`. Sem o loop do `event_bus` já criado,
  `event_bus_register()` retornava `ESP_ERR_INVALID_STATE` silenciosamente
  (retorno nunca checado) — PV, MV, status de Wi-Fi e modo nunca atualizavam
  via evento; só o gesto de toque "funcionava" por atualizar a tela
  diretamente, sem passar pelo `event_bus`. Corrigido chamando
  `event_bus_init()` explicitamente no início de `app_main()`, antes de
  `ui_manager_init()` (idempotente — a chamada dentro de `comm_manager_init`
  continua e agora é um no-op).

## `communication/wifi_manager.cpp` — sincronização de hora (SNTP)

- Relógio da tela Principal (`ui_manager.cpp:clock_tick_cb`) ficava sempre em
  `--:--` porque nada no firmware sincronizava o RTC do ESP32 (sem SNTP, o
  relógio fica parado em 1970). Adicionado `start_time_sync()`, chamado em
  `IP_EVENT_STA_GOT_IP` (mesmo instante em que a conexão à internet é
  estabelecida): configura `TZ=<-03>3` (UTC-3 sem horário de verão — Brasil
  aboliu o DST em 2019; newlib só entende o offset POSIX cru, não a tzdata
  IANA completa) e inicia `esp_netif_sntp_init`/`esp_netif_sntp_start` contra
  `pool.ntp.org`. Requer `esp_netif_sntp.h` (já cobertos pelo `REQUIRES
  esp_netif` existente no `CMakeLists.txt` de `communication/`).
- Relógio também passou a exibir segundos (`HH:MM:SS`, era `HH:MM`) —
  `clock_tick_cb` e o texto inicial em `screen_main.cpp` atualizados.

## `ui/screens/screen_main.{h,cpp}` e `ui/ui_manager.cpp` — modelo SP/MV/PV

- Redesenhada a tela Principal em torno de terminologia de malha de
  controle, substituindo o antigo par "intensidade %"/"acionamento %":
  - **SP** (setpoint) — `target_lux`, alvo em lux definido pelo toque.
  - **MV** (manipulated variable) — `brightness_pct`, saída real do
    dimmer/TRIAC (eco de `DIMMER_UPDATE`); mostrado como `MV: XX%` na barra
    azul e no readout central.
  - **PV** (process variable) — `natural_lux`, leitura ao vivo do sensor LDR
    (`LDR_UPDATE`); mostrado como `PV: XXX lx`, abaixo do MV no readout —
    sempre local, nunca depende de rede.
  - Barra âmbar passou a refletir o SP (era a intensidade bruta); barra azul
    passou a refletir o MV real (era uma estimativa derivada do SP, nunca o
    valor real do atuador).
- **MV não atuava sem rede** (regressão introduzida e corrigida na mesma
  sessão): o gesto de toque chegou a postar só `SETPOINT_CHANGED`, que
  `main.cpp` não consome para acionar o TRIAC — em modo Manual, sem
  `DIMMER_COMMAND` chegando via MQTT, a lâmpada nunca respondia ao toque
  offline. Corrigido: `on_main_released` volta a postar `DIMMER_COMMAND`
  diretamente no `event_bus` (não MQTT), calculado a partir do SP
  (`target_lux/UI_AMBIENT_LUX_SCALE*100`) — `main.cpp:on_dimmer_command`
  aplica no TRIAC imediatamente, independente de Wi-Fi/MQTT.
  `SETPOINT_CHANGED` continua sendo postado à parte só para
  telemetria/sincronismo com app+backend.

## `ui/ui_manager.cpp` — gestos da tela Principal reescritos

- Removido todo o sistema de deslizamento/swipe (deslize-esquerda abria
  Ajustes, deslize-direita desligava a lâmpada) e a zona de toque bruto
  amarrada ao botão de configurações — substituídos por toque absoluto
  puro, conforme pedido: só toque define o SP e liga/desliga, sem gestos.
- `apply_touch_y()`: lê o bruto do XPT2046 via `lvgl_port_get_raw_touch`
  (não a coordenada de tela já calibrada), aplica a mesma correção de eixo
  (`TOUCH_SWAP_XY`/`TOUCH_INVERT_X`) usada em `touch_read_cb`, e mapeia por
  limiares fixos no valor bruto:
  - bruto `<= 350` → SP = 100% (liga no máximo);
  - bruto `>= 750` → SP = 0% (desliga);
  - entre os dois → interpolação linear 100% → 0%.
  Essa janela (350–750) é mais estreita que o range completo calibrado em
  `board_config.h` de propósito — a barra/marcador segue o toque em tempo
  real (recalculado a cada `PRESSING`), mas a % de brilho resultante não
  precisa corresponder à posição "justa" da barra na tela.
- Botão de configurações (engrenagem no topo) desativado por enquanto —
  `CFG_ZONE_ENABLED = false` e o `lv_obj_add_event_cb` de clique comentado.
  Ícone continua visível, só não navega mais para Ajustes (causava falsos
  positivos durante o arrasto normal). Sem caminho de toque para Ajustes a
  partir da Principal até essa decisão ser revertida.
