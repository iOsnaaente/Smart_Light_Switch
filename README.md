# Smart Light Switch

Sistema completo de **interruptor inteligente com regulagem automática de
luminosidade**: um dispositivo ESP32-C3 mede a luz ambiente e ajusta um
dimmer (TRIAC) para manter o ambiente num nível de lux configurável,
reportando tudo em tempo real — e podendo ser controlado remotamente — via
MQTT/HTTP por um backend e um app móvel.

O projeto tem três partes que conversam entre si por um contrato comum
(MQTT + REST), cada uma em seu diretório com seu próprio README:

| Parte | Diretório | Stack | Papel |
|---|---|---|---|
| **Firmware** | [`Firmware/`](Firmware/) | ESP-IDF v5.5 (FreeRTOS, C++) no ESP32-C3 | Lê o sensor de luz (LDR) e o medidor de potência, controla o relé e o dimmer TRIAC, mostra um painel local em um display TFT (LVGL) e fala MQTT com o backend |
| **Backend** | [`Backend/`](Backend/) | FastAPI + SQLAlchemy + PostgreSQL + Mosquitto, via Docker Compose | Autentica usuários, expõe a API REST usada pelo app, publica comandos MQTT para os dispositivos e persiste estado/telemetria recebidos deles |
| **App** | [`App/`](App/) | Flutter (Material 3, `provider`) | Interface para listar dispositivos, controlar relé/modo/dimmer/setpoint, ver telemetria e configurar cada interruptor — fala apenas com o backend (HTTP), nunca diretamente com o MQTT |

## Arquitetura — visão de ponta a ponta

```text
┌───────────────┐        MQTT         ┌───────────────────┐        MQTT          ┌──────────────────┐
│   Firmware    │ ◄─────────────────► │  Mosquitto Broker │ ◄──────────────────► │   MQTT Consumer  │
│  (ESP32-C3)   │  devices/{u}/{d}/…  │  (Backend/Docker) │  state · telemetry   │ (Backend/Docker) │
│ sensores, relé│                     └─────────┬─────────┘                      └─────────┬────────┘
│ dimmer, LCD   │                               │ command (publica)                        │ grava
└───────────────┘                               │                                          ▼
                                        ┌───────┴────────┐                        ┌──────────────┐
                                        │   FastAPI      │ ◄───────────────────── │  PostgreSQL  │
                                        │   (Backend)    │   lê estado/telemetria │  (Backend)   │
                                        └───────┬────────┘                        └──────────────┘
                                                │ HTTP REST (X-Username / X-Password)
                                                ▼
                                        ┌────────────────┐
                                        │   App Flutter  │
                                        │  (Android/iOS/ │
                                        │  Web/Desktop)  │
                                        └────────────────┘
```

- O **App nunca fala MQTT diretamente** — todo comando (ligar relé, mudar
  modo, ajustar dimmer/setpoint...) vai por HTTP até o backend, que então
  publica no broker; toda leitura (estado, telemetria, lista de
  dispositivos) também vem da API, que por sua vez lê o que o
  **MQTT Consumer** já persistiu no PostgreSQL a partir do que o firmware
  publicou.
- O **Firmware nunca fala com a API REST** — ele só conversa com o broker
  MQTT (assina `command`, publica `state`/`telemetry`).
- O **Backend é o único ponto que enxerga os dois mundos**: publica
  comandos no broker (lado dispositivo) e expõe REST com autenticação por
  usuário (lado app).

## Contrato MQTT — a "língua" comum entre Firmware e Backend

Cada dispositivo é endereçado por `{user_id}/{device_id}` (permite que
usuários diferentes tenham um dispositivo `switch01` sem colidir):

| Tópico | Direção | Payload (exemplos) |
|---|---|---|
| `devices/{user_id}/{device_id}/command` | Backend → Firmware | `{"relay": true}` · `{"mode": "auto"\|"manual"}` · `{"dimmer": 70}` · `{"setpoint": 400}` · `{"dimmer_limits": {"min": 10, "max": 100}}` · `{"calibrate": true}` |
| `devices/{user_id}/{device_id}/state` | Firmware → Backend | `{"relay": true, "automatic_mode": false, "dimmer": 70, "rssi": -55, "firmware": "1.0.0", "wifi_name": "MinhaRede"}` |
| `devices/{user_id}/{device_id}/telemetry` | Firmware → Backend | `{"sp": 220.1, "mv": 219.8, "current": 0.55, "power": 121.0, "lux": 410.5, "natural_lux": 180.2, "dimmer": 70, "kwh_today": 0.532}` |

Qualquer mensagem em `state` ou `telemetry` funciona como heartbeat
implícito (`last_seen`); o backend marca o dispositivo como `online` se
houve uma mensagem nos últimos 2 minutos — é esse mesmo campo que o app
mostra como "visto há Xh" quando o dispositivo está offline. Veja o
contrato completo, exemplos de `curl`/`mosquitto_pub` e o modelo de dados em
[`Backend/README.md`](Backend/README.md).

## Como apresentar o sistema de ponta a ponta

Não é preciso ter um ESP32 físico para demonstrar o fluxo completo —
existem três níveis, do mais ao menos realista:

### 1. Hardware real + Backend + App

```bash
# 1) Suba o backend (API + Postgres + Mosquitto + consumer)
cd Backend && docker compose up --build -d

# 2) Grave o firmware no ESP32-C3 (ajuste antes o IP do broker em
#    Firmware/main/main.cpp::MQTT_BROKER_URI para o IP da máquina rodando o
#    docker compose, e as credenciais Wi-Fi via o provisionamento BLE)
cd ../Firmware
source ~/esp/v5.5.1/esp-idf/export.sh   # ou o caminho onde o ESP-IDF está instalado
idf.py -p /dev/ttyUSB0 flash monitor

# 3) Rode o app apontando para o backend
cd ../App
flutter run --dart-define=API_BASE_URL=http://<ip-da-maquina-com-o-backend>:8000
```

Use `admin` / `admin` para entrar (usuário padrão criado na primeira
inicialização do backend — configurável em `Backend/.env.example`).

### 2. Sem hardware — Backend + simulador Python + App

Reproduz o fluxo completo (API → MQTT → "dispositivo" → MQTT → consumer →
banco → API → App) sem precisar gravar nada num ESP32: o
[`Backend/simulator/esp32_simulator.py`](Backend/simulator/esp32_simulator.py)
se conecta ao broker e se comporta como um dispositivo real (assina
`command`, responde com `state`/`telemetry`).

```bash
cd Backend && docker compose up --build -d
cd simulator && python3 -m venv .venv && source .venv/bin/activate
pip install -r requirements.txt
python esp32_simulator.py 1 switch01      # user_id=1 (admin), device_id=switch01

# em outro terminal
cd ../../App && flutter run --dart-define=API_BASE_URL=http://localhost:8000
```

Passo a passo detalhado (com o que observar em cada terminal) em
["Running Locally"](Backend/README.md#running-locally) no README do Backend.

### 3. Só o App — modo de demonstração offline

Sem subir nada: abra o app e toque em **"Entrar como demonstração"** na
tela de login. Ele carrega dispositivos fictícios em memória
(`App/lib/models/mock_devices.dart`) e deixa navegar/operar a interface
inteira (lista, controle, configurações) sem rede — ideal para mostrar a
experiência do usuário quando backend/hardware não estão disponíveis.

## Credenciais padrão

O backend cria um usuário `admin`/`admin` na primeira inicialização
(variáveis `AUTH_USERNAME`/`AUTH_PASSWORD` em `Backend/.env.example`). A
autenticação é feita por headers HTTP simples (`X-Username`/`X-Password`,
sem JWT) — o app já vem com esse usuário pré-preenchido na tela de login.

## CI/CD

Todo push em `main` que altera `Backend/**` reconstrói e publica a imagem
`iosnaaente/smart-switch-backend` no Docker Hub
([`.github/workflows/docker.yml`](.github/workflows/docker.yml)) — a mesma
imagem usada pelos serviços `api` e `mqtt-consumer` do `docker-compose.yml`.

## Aprofundando em cada parte

- [`Firmware/README.md`](Firmware/README.md) — guia do subsistema de
  imagens/display do firmware (ILI9340 + LVGL); para a arquitetura de
  comunicação, sensores e controle, veja os diretórios
  [`Firmware/communication/`](Firmware/communication/),
  [`Firmware/controllers/`](Firmware/controllers/),
  [`Firmware/sensors/`](Firmware/sensors/) e
  [`Firmware/system/`](Firmware/system/) (event bus
  `SMART_SWITCH_EVENT_*` que conecta sensores, controle e comunicação).
- [`Backend/README.md`](Backend/README.md) — arquitetura, contrato MQTT
  completo, autenticação, todos os endpoints REST (com exemplos de `curl`),
  modelo de dados e o guia "Running Locally" para rodar tudo com o
  simulador.
- [`App/README.md`](App/README.md) — estrutura do app, como rodar/testar
  (`flutter analyze`/`flutter test`/`flutter run`), configuração da URL do
  backend via `--dart-define`, modo de demonstração e como simular o
  dispositivo via MQTT.
