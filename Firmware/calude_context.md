# Contexto do Firmware — Smart Light Switch (para Claude)

Este arquivo resume o entendimento acumulado sobre o firmware durante o
trabalho de implementação do ciclo de conectividade, para acelerar sessões
futuras.

## Visão geral

Firmware para um interruptor/dimmer de luz inteligente baseado em **ESP32-C3**,
com display TFT (ILI9341 via `esp-idf-ili9340` + LVGL), controle de dimmer por
TRIAC sincronizado com zero-cross, sensor de luminosidade (LDR) e — agora —
um ciclo completo de conectividade: **provisionamento via BLE → conexão
Wi-Fi → cliente MQTT** para integração com o backend (`Backend/`, broker
Mosquitto).

- Build: ESP-IDF v5.5.1, `idf.py build` (ativar com
  `source /home/iosnaaente/esp/v5.5.1/esp-idf/export.sh`)
- Alvo: `esp32c3`, flash de 4 MB, partição `factory` customizada de 3200 KB
  (ver `partitions.csv` e `roadmap.md`)
- Idioma dos comentários/logs no código: português

## Estrutura por componentes (cada um com seu `CMakeLists.txt`)

- **`main/`** — `app_main()`; inicializa o `comm_manager`, o sensor LDR, o
  controlador de TRIAC, a tarefa de controle da lâmpada (`lamp_control_task`)
  e a tarefa de atualização do display.
- **`board_config.h`** (raiz) — pinagem central da placa (SPI do display,
  zero-cross, gate do TRIAC, sensor LDR, UART do sensor de energia, botão de
  reset de provisionamento). Arrasta `esp_lcd`/display — por isso componentes
  que não usam display (ex.: `communication`) **não devem incluí-lo**, sob
  pena de erro de `esp_lcd_panel_io.h: No such file or directory`.
  TODAS as definições Hardcoded devem ficar em board_config, sem exceção, como por exemplo, endereços IP, portas e definições do sistema como um todo.
- **`system/`** — infraestrutura compartilhada:
  - `app_events.h` / `event_bus.{h,cpp}`: barramento de eventos baseado no
    `esp_event` para desacoplar componentes (ex.: status de rede, mudança de
    modo, comandos de relé/dimmer).
  - `app_modes.{h,cpp}`: máquina de modos da aplicação (ex.: `APP_MODE_MANUAL`).
- **`communication/`** — pilha de conectividade (foco do trabalho recente):
  - `device_identity.h`: helper para gerar `device_id` / nome BLE a partir do
    MAC.
  - `ble_setup.{h,cpp}`: provisionamento Wi-Fi via BLE (GATT server,
    Bluedroid). Usa APIs legadas de advertising BLE 4.2
    (`esp_ble_gap_start_advertising`, `esp_ble_gap_config_adv_data`,
    `esp_ble_gap_stop_advertising`) — exigem `CONFIG_BT_BLE_ENABLED=y` e
    `CONFIG_BT_BLE_42_FEATURES_SUPPORTED=y` no `sdkconfig` (ver detalhes em
    `roadmap.md`).
  - `wifi_manager.{h,cpp}`: gerencia conexão Wi-Fi (STA), persistência de
    credenciais na NVS, fallback para AP de emergência
    (`wifi_manager_clear_credentials`, `wifi_manager_has_credentials`,
    `wifi_manager_is_connected`, `wifi_manager_start/stop`).
  - `mqtt_client.cpp` / `mqtt_manager.h`: cliente MQTT (`mqtt_client_init/
    start/stop`), publica telemetria/heartbeat e assina comandos.
  - `http_api.h`: stub para futura API HTTP local (não habilitada por padrão —
    `comm_cfg.enable_http = false`).
  - `comm_manager.{h,cpp}`: **orquestrador** criado nesta sessão — sequencia
    BLE → Wi-Fi → MQTT numa única task FreeRTOS (`comm_manager_task`),
    expondo `comm_manager_init/start/stop`. Detecta o botão de reset de
    provisionamento (GPIO18, `PIN_NUM_PROVISION_RESET`, definido localmente
    para não depender de `board_config.h`) e limpa credenciais Wi-Fi se
    pressionado no boot.
- **`controllers/`** — `triac_controller.{h,cpp}`: controle de dimmer via
  TRIAC com detecção de zero-cross (interrupção em `PIN_NUM_ZEROCROSS`,
  disparo em `PIN_NUM_GATE_TRIAC`).
- **`sensors/`** — `LDR/` (sensor de luminosidade, leitura normalizada via
  ADC) e `PowerMeter/` (medição de energia via UART).
- **`ui/`** — `display_manager.{h,cpp}` (gerencia o display TFT/LVGL,
  mostra imagens em rotação) e `ui_manager.h`/`ui_stub.cpp`.
- **`components/`** — bibliotecas vendorizadas: `esp-idf-ili9340` (driver do
  display) e `lvgl`.

## Fluxo de conectividade (implementado nesta sessão)

`comm_manager_task`:
1. `init_nvs()` + `event_bus_init()` + `app_modes_init(APP_MODE_MANUAL)`.
2. Verifica `PIN_NUM_PROVISION_RESET` no boot → se pressionado, chama
   `wifi_manager_clear_credentials()`.
3. Se não há credenciais Wi-Fi salvas:
   - requer `enable_ble = true` (caso contrário, encerra a task com erro);
   - chama `ble_setup_start()` e aguarda `wifi_manager_has_credentials()`.
4. Chama `wifi_manager_start()` e aguarda `wifi_manager_is_connected()`.
5. Se `enable_mqtt = true`, chama `mqtt_client_start()`.
6. Task se encerra (`vTaskDelete`) após a sequência — a manutenção da conexão
   fica a cargo dos próprios `wifi_manager`/`mqtt_client` (event-driven).

Configuração injetada via `comm_manager_config_t` (em `main.cpp`):
- `enable_ble = true`, `enable_mqtt = true`, `enable_http = false`
- `mqtt_broker_uri = "mqtt://192.168.1.100:1883"` ⚠️ **placeholder** — ajustar
  para o IP real do broker Mosquitto (ver `Backend/docker-compose.yml` /
  `Backend/.env.example`: `MQTT_HOST=mosquitto`, `MQTT_PORT=1883`)
- `wifi_ap_fallback_ssid = "SmartLight-Setup"` /
  `wifi_ap_fallback_pass = "smartlight123"`

## Pinagem relevante (`board_config.h`)

| Pino | Constante | Uso |
|---|---|---|
| GPIO9 | `PIN_NUM_RST` | Reset do display TFT |
| GPIO18 | `PIN_NUM_PROVISION_RESET` | Botão de reset de provisionamento (pull-up, nível baixo = pressionado) — **cuidado**: já houve conflito acidental com GPIO9 (`PIN_NUM_RST`), corrigido nesta sessão |
| GPIO1 | `PIN_NUM_ZEROCROSS` | Interrupção de zero-cross (TRIAC) |
| GPIO10 | `PIN_NUM_GATE_TRIAC` | Disparo do TRIAC |
| GPIO0 | `PIN_NUM_SENSOR_LDR` | Leitura ADC do LDR |
| GPIO20/21 | `PIN_NUM_SENSOR_TXD/RXD` | UART do sensor de energia |

## Particularidades de build / Kconfig descobertas

- **Mismatch de venv Python**: se o erro `'.../python_env/idf5.5_pyX.Y_env/...
  is currently active...'` aparecer, rodar `idf.py fullclean` (seguro, só
  limpa artefatos de build) e rebuildar.
- **BLE 4.2 advertising**: `esp_ble_gap_start_advertising` /
  `esp_ble_gap_config_adv_data` / `esp_ble_gap_stop_advertising` são
  compiladas condicionalmente em `esp_gap_ble_api.c` via
  `#if (BLE_42_FEATURE_SUPPORT == TRUE)` / `#if (BLE_42_ADV_EN == TRUE)`
  (macros derivadas de `CONFIG_BT_BLE_42_FEATURES_SUPPORTED` em `bt_target.h`).
  Sem isso, há *undefined reference* no link.
- Qualquer edição manual em `sdkconfig` deve ser seguida de
  `idf.py reconfigure` para popular defaults dependentes
  (ex.: `CONFIG_BT_GATTS_ENABLE`, `CONFIG_BT_BLE_42_ADV_EN/SCAN_EN`).
- Mudanças de Kconfig "profundas" (stack BT, tamanho de flash/partições)
  disparam rebuild quase completo (~2060 alvos via Ninja).
- `app_check_size` valida se `Firmware.bin` cabe na partição `factory` —
  ver `roadmap.md` para o histórico do estouro de partição e a correção
  (tabela customizada `partitions.csv` + flash 4 MB).

## Como implementar/documentar um novo módulo (`TEMPLATE.md`)

O projeto define um padrão de documentação por módulo, que
deve ser preenchido (como um `README.md` do módulo) ao criar um componente
novo. Estrutura esperada:

1. **Título + descrição breve** — nome e propósito do módulo.
2. **O que faz** — objetivo direto; o módulo deve **abstrair a lógica interna
   e expor só uma interface simples**; listar os recursos/estado gerenciados
   internamente (ex.: filas, timers, estados operacionais).
3. **Conceito principal** — o conceito físico/eletrônico/lógico por trás
   (ex.: zero-cross + ângulo de disparo do TRIAC); explicar entrada→saída,
   linearidade, limitações, sincronismo, comportamento temporal.
4. **Por que foi desenhado assim** — justificar decisões de arquitetura:
   separação ISR/task, uso de timers/watchdogs, sincronismo, debounce,
   thread-safety, fallback, precisão temporal — e o benefício técnico de
   cada uma.
5. **Como funciona** — fluxo principal em poucos passos (evento inicial →
   captura/processamento → cálculo → atuação → atualização de estado).
6. **Como usar** — exemplos de código cobrindo:
   - *Inicialização* (`module.init(...)`, `module.start()`): pinos, parâmetros,
     configuração inicial;
   - *Controle* (`module.set_xxx(...)`): faixa válida, comportamento esperado,
     origem típica dos dados (sensores, UI, rede);
   - *Leitura de estado* (`module.get_status()`): métodos públicos principais;
   - *Encerramento* (`module.stop()`): o que é desligado/finalizado.
7. **Tabela de Estados** — `STATUS_OK`, `STATUS_ERROR` etc. com descrição.
8. **Tabela de Parâmetros importantes** — nome do parâmetro e sua função.
9. **Dependências** — bibliotecas/drivers usados.
10. **Restrições** — limitações conhecidas, dependências de hardware,
    limitações temporais.

### Convenções de implementação observadas no projeto

- Cada módulo vira um **componente ESP-IDF** com seu próprio `CMakeLists.txt`
  (`idf_component_register` com `SRCS`, `INCLUDE_DIRS`, `REQUIRES`) — seguir o
  padrão usado em `communication/CMakeLists.txt` e `controllers/CMakeLists.txt`.
- API pública enxuta: `<modulo>_init()`, `<modulo>_start()`, `<modulo>_stop()`
  (+ getters/setters específicos), escondendo filas, tasks e estado interno
  como `static`/privados no `.cpp`.
- Comentários e logs em **português**, explicando o "porquê" (constraints,
  workarounds, invariantes) — não o "o quê".
- Componentes que não precisam de display **não devem incluir
  `board_config.h`** (ele arrasta `esp_lcd`); se precisar de um pino definido
  lá, redefinir localmente uma constante `constexpr gpio_num_t` equivalente
  (como feito em `comm_manager.cpp` com `PIN_NUM_PROVISION_RESET`).
- Declarar todas as dependências de componentes IDF em `REQUIRES` no
  `CMakeLists.txt` — omissões só aparecem como erros de `#include` no build.
- Após registrar/alterar um componente novo, adicioná-lo ao `REQUIRES` do
  componente consumidor (ex.: `main/CMakeLists.txt` precisou listar
  `communication`).

## Documentos relacionados

Sempre ler os contextos antes de começar qualqu

- `changelog.md` — changelog detalhado das mudanças das sessões. Toda vez que for solicitado uma mudança grande de funcionalidade, deve ser feito um append do que foi modificado para poder manter um histórico do projeto e contexto expandido.
- `README.md` — documentação existente do projeto.

Em dúvida, SEMPRE pergunte o que fazer para o usuário antes de começar;
