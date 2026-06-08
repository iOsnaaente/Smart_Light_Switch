# Smart Light Switch — Backend (PoC IoT)

Backend de prova de conceito para controle de interruptores inteligentes baseados em
ESP32, com comunicação via MQTT, persistência em PostgreSQL e API HTTP em FastAPI.

## Arquitetura

```txt
Flutter App
      │
      ▼
  FastAPI
      │
 ┌────┴────┐
 ▼         ▼
Postgres  MQTT Publish
               │
               ▼
         Mosquitto Broker
               │
        ┌──────┴──────┐
        ▼             ▼
      ESP32    MQTT Consumer
                      │
                      ▼
                 PostgreSQL
```

- **mosquitto**: broker MQTT.
- **postgres**: banco de dados relacional.
- **api**: API FastAPI que autentica usuários, publica comandos MQTT e consulta dados no banco.
- **mqtt-consumer**: serviço que assina os tópicos `devices/+/+/state` e `devices/+/+/telemetry`,
  persistindo o estado e a telemetria dos dispositivos no banco.

## Tópicos MQTT

Cada dispositivo é identificado pela combinação `{user_id}/{device_id}`, o que permite
que usuários diferentes tenham dispositivos com o mesmo `device_id` sem conflito:

| Tópico                                      | Direção          | Payload                                              |
|---------------------------------------------|------------------|------------------------------------------------------|
| `devices/{user_id}/{device_id}/command`    | API → dispositivo | `{"relay": true}`, `{"mode": "auto"\|"manual"}`, `{"dimmer": 70}`, `{"setpoint": 400}`, `{"dimmer_limits": {"min": 10, "max": 100}}`, `{"calibrate": true}` |
| `devices/{user_id}/{device_id}/state`      | dispositivo → API | `{"relay": true, "automatic_mode": false, "dimmer": 70, "rssi": -55, "firmware": "1.0.0", "wifi_name": "MinhaRede"}` |
| `devices/{user_id}/{device_id}/telemetry`  | dispositivo → API | `{"sp": 220.1, "mv": 219.8, "current": 0.55, "power": 121.0, "lux": 410.5, "natural_lux": 180.2, "dimmer": 70, "kwh_today": 0.532}` |

Qualquer mensagem recebida em `state` ou `telemetry` atualiza `last_seen` do
dispositivo (heartbeat implícito); o backend considera o dispositivo `online`
se `last_seen` estiver a menos de 2 minutos do momento atual. `firmware` e
`wifi_name`, quando presentes no payload de `state`, são persistidos em
`devices` (somente leitura pelo app).

## Autenticação

Autenticação simplificada via middleware HTTP (sem JWT), com usuários cadastrados
no banco de dados (tabela `users`, com senha armazenada com hash). Na primeira
inicialização um usuário padrão é criado a partir de `auth_username`/`auth_password`
(`admin` / `admin` por padrão).

O `/login` retorna o `user_id` do usuário autenticado. As rotas de
`/devices/...` **não** levam `user_id` na URL — o backend resolve o usuário a
partir da sessão (headers `X-Username`/`X-Password`) e só enxerga/opera os
dispositivos pertencentes a ele (`404` para dispositivos de outro usuário ou
inexistentes). Internamente, os tópicos MQTT continuam usando
`devices/{user_id}/{device_id}/...` para separar dispositivos de usuários
diferentes que usem o mesmo `device_id`.

Endpoints (exceto `/login` e a documentação) exigem os headers:

```
X-Username: admin
X-Password: admin
```

## Como executar

Pré-requisitos: Docker e Docker Compose.

```bash
cd Backend
docker compose up --build
```

Isso inicia os containers `postgres`, `mosquitto`, `api` e `mqtt-consumer`.
A API ficará disponível em `http://localhost:8000` e a documentação interativa em
`http://localhost:8000/docs`.

> Para um passo a passo completo de como rodar tudo localmente e simular um
> ESP32 conversando com a API e o broker em tempo real, veja

As tabelas (`users`, `devices`, `device_state`, `telemetry`) são criadas
automaticamente na inicialização da API e do consumidor MQTT.

## Endpoints

### `POST /login`

Valida usuário e senha contra a tabela `users` e retorna o `user_id`.

```bash
curl -X POST http://localhost:8000/login \
  -H "Content-Type: application/json" \
  -d '{"username": "admin", "password": "admin"}'
```

Resposta:

```json
{"success": true, "user_id": 1}
```

Todas as rotas abaixo exigem os headers `X-Username`/`X-Password` e operam
sobre os dispositivos do usuário autenticado (resolvido via
`request.state.user_id`); dispositivos inexistentes ou de outro usuário
respondem `404`.

| Rota | Ação |
|---|---|
| `GET /devices` | Lista os dispositivos do usuário (resumo) |
| `GET /devices/{device_id}` | Detalhe completo do dispositivo |
| `PUT /devices/{device_id}` | Renomeia / altera o ambiente (`name`, `room`) |
| `DELETE /devices/{device_id}` | Remove o dispositivo (e estado/telemetria/agendamentos associados) |
| `GET /devices/{device_id}/state` | Último estado relatado (`relay`, `automatic_mode`, `dimmer`, `online`, ...) |
| `GET /devices/{device_id}/telemetry/latest` | Última leitura de telemetria |
| `GET /devices/{device_id}/telemetry/history?range=today` | Histórico do dia agregado em buckets de 15 min (`lux`, `natural_lux`, `dimmer`) |
| `POST /devices/{device_id}/relay/on` `/off` | Publica `{"relay": true\|false}` |
| `POST /devices/{device_id}/mode` | Publica `{"mode": "auto"\|"manual"}` |
| `POST /devices/{device_id}/dimmer` | Publica `{"dimmer": N}` |
| `PUT /devices/{device_id}/setpoint` | Grava `setpoint` no banco e publica `{"setpoint": N}` |
| `PUT /devices/{device_id}/dimmer-limits` | Grava `dimmer_min`/`dimmer_max` e publica `{"dimmer_limits": {...}}` |
| `POST /devices/{device_id}/calibrate-sensor` | Publica `{"calibrate": true}` |
| `GET /devices/{device_id}/schedules` | Lista agendamentos |
| `POST /devices/{device_id}/schedules` | Cria agendamento (`label`, `time`, `days_of_week`, `action`) |
| `DELETE /devices/{device_id}/schedules/{schedule_id}` | Remove agendamento |

`relay`, `mode`, `dimmer` e `calibrate-sensor` seguem o padrão "publish-only":
a API publica o comando MQTT e devolve `published: bool`; o estado real chega
de volta pelo tópico `state` e é persistido pelo consumer (sem escrita dupla).
Já `setpoint`, `dimmer-limits` e o nome/ambiente (`PUT /devices/{device_id}`)
são configuração do backend — gravados diretamente em `devices` e replicados
ao dispositivo via MQTT.

Exemplos:

```bash
# Ligar o relé — publica em devices/{user_id}/switch01/command
curl -X POST http://localhost:8000/devices/switch01/relay/on \
  -H "X-Username: admin" -H "X-Password: admin"

# Ativar modo automático
curl -X POST http://localhost:8000/devices/switch01/mode \
  -H "X-Username: admin" -H "X-Password: admin" -H "Content-Type: application/json" \
  -d '{"automatic_mode": true}'

# Ajustar o dimmer manualmente
curl -X POST http://localhost:8000/devices/switch01/dimmer \
  -H "X-Username: admin" -H "X-Password: admin" -H "Content-Type: application/json" \
  -d '{"dimmer": 70}'

# Definir o setpoint de luminosidade
curl -X PUT http://localhost:8000/devices/switch01/setpoint \
  -H "X-Username: admin" -H "X-Password: admin" -H "Content-Type: application/json" \
  -d '{"setpoint": 350}'

# Consultar o estado e a telemetria mais recentes
curl http://localhost:8000/devices/switch01/state -H "X-Username: admin" -H "X-Password: admin"
curl http://localhost:8000/devices/switch01/telemetry/latest -H "X-Username: admin" -H "X-Password: admin"

# Histórico do dia (gráfico)
curl "http://localhost:8000/devices/switch01/telemetry/history?range=today" \
  -H "X-Username: admin" -H "X-Password: admin"
```

## Testando com mosquitto_pub (simulando o ESP32)

Simular o envio de estado e telemetria pelo dispositivo `switch01` do usuário `1`:

```bash
# Estado
mosquitto_pub -h localhost -t devices/1/switch01/state \
  -m '{"relay": true, "automatic_mode": false, "dimmer": 70, "rssi": -55}'

# Telemetria
mosquitto_pub -h localhost -t devices/1/switch01/telemetry \
  -m '{"sp": 220.1, "mv": 219.8, "current": 0.55, "power": 121.0, "lux": 410.5, "natural_lux": 180.2, "dimmer": 70, "kwh_today": 0.532}'
```

Para escutar comandos publicados pela API:

```bash
mosquitto_sub -h localhost -t 'devices/+/+/command' -v
```

> **Broker com autenticação:** se o `mosquitto.conf` tiver
> `allow_anonymous false` + `password_file` (criado com `mosquitto_passwd`),
> todo cliente precisa se autenticar — inclusive `mosquitto_pub`/
> `mosquitto_sub` na linha de comando. Adicione manualmente as flags
> `-u <usuário> -P <senha>` (mesmas credenciais configuradas em
> `MQTT_USERNAME`/`MQTT_PASSWORD` no `.env` e em `MQTT_USERNAME`/
> `MQTT_PASSWORD` no firmware — ver `Firmware/main/main.cpp`):
>
> ```bash
> mosquitto_pub -h localhost -u smart-light-device -P troque-esta-senha \
>   -t devices/1/switch01/state -m '{"relay": true}'
>
> mosquitto_sub -h localhost -u smart-light-device -P troque-esta-senha \
>   -t 'devices/+/+/command' -v
> ```

## Estrutura de diretórios

```
Backend/
├── app/
│   ├── api/            # Rotas HTTP (login, devices)
│   ├── core/           # Configuração e middleware de autenticação
│   ├── db/             # Sessão e Base do SQLAlchemy
│   ├── models/         # Modelos ORM (User, Device, DeviceState, Telemetry, Schedule)
│   ├── mqtt/           # Publisher (API -> broker) e Consumer (broker -> banco)
│   ├── schemas/        # Schemas Pydantic
│   ├── services/       # Regras de acesso a dados dos dispositivos
│   └── main.py         # Inicialização da aplicação FastAPI
├── mosquitto/
│   └── config/mosquitto.conf
├── Dockerfile
├── docker-compose.yml
├── requirements.txt
└── .env.example
```

## Modelo de dados

- **users**: `id`, `username`, `password_hash`, `created_at`
- **devices**: `id`, `user_id`, `device_id`, `name`, `room`, `firmware`, `wifi_name`,
  `setpoint`, `dimmer_min`, `dimmer_max`, `last_seen`, `created_at`
  (único por `user_id` + `device_id`)
- **device_state**: `id`, `user_id`, `device_id`, `relay`, `automatic_mode`, `dimmer`,
  `rssi`, `updated_at`
- **telemetry**: `id`, `user_id`, `device_id`, `sp`, `mv`, `current`, `power`,
  `lux`, `natural_lux`, `dimmer`, `kwh_today`, `timestamp`
- **schedules**: `id`, `user_id`, `device_id`, `label`, `time`, `days_of_week`
  (CSV, ex. `mon,tue,wed`), `action` (`relay_on`/`relay_off`), `enabled`, `created_at`



# Running Locally 

Este guia mostra como subir o backend na sua máquina e como criar/usar um
"sistema externo" (um simulador de ESP32 + chamadas HTTP) que conversa com a
API pelo `localhost`, para você acompanhar o fluxo completo em tempo real:

```
[seu terminal/curl] --HTTP--> [API FastAPI] --MQTT--> [Mosquitto] <--MQTT-- [simulador ESP32]
                                    ^                                            |
                                    |                                            v
                              [PostgreSQL] <-------- [MQTT Consumer] <---- state/telemetry
```

## 1. Pré-requisitos

- Docker e Docker Compose
- Python 3.12 (ou 3.11+) na sua máquina, para rodar o simulador fora do Docker

## 2. Subir o backend

```bash
cd Backend
docker compose up --build -d
docker compose ps
```

Você deve ver 4 containers rodando: `smart_switch_postgres`,
`smart_switch_mosquitto`, `smart_switch_api` e `smart_switch_mqtt_consumer`.

A API estará em `http://localhost:8000` (docs interativas em
`http://localhost:8000/docs`) e o broker MQTT em `localhost:1883`.

Acompanhe os logs em tempo real (ótimo para ver o consumer recebendo mensagens):

```bash
docker compose logs -f api mqtt-consumer
```

## 3. Criar o "outro sistema": um simulador de ESP32

Para ver o fluxo completo acontecendo ao vivo, você precisa de algo que se
comporte como o dispositivo real: que escute comandos em
`devices/{user_id}/{device_id}/command` e publique `state`/`telemetry`. Em vez
de usar um ESP32 físico, criamos um pequeno script Python que faz exatamente
isso — ele roda como um processo independente, fora do `docker compose`,
conectando no broker pelo `localhost:1883`.

Os tópicos incluem o `user_id` para que dispositivos de usuários diferentes
não conflitem entre si, mesmo que usem o mesmo `device_id`.

O script já está em [`simulator/esp32_simulator.py`](simulator/esp32_simulator.py).
Ele:

- conecta no Mosquitto local;
- assina `devices/{user_id}/{device_id}/command` e, ao receber `relay`,
  `mode` (`"auto"`/`"manual"`), `dimmer`, `setpoint`, `dimmer_limits` ou
  `calibrate`, atualiza seu estado interno e republica em
  `devices/{user_id}/{device_id}/state` (incluindo `automatic_mode`, `dimmer`,
  `rssi`, `firmware`, `wifi_name`);
- publica telemetria simulada (`sp`, `mv`, `current`, `power`, `lux`,
  `natural_lux`, `dimmer`, `kwh_today`) a cada 5 segundos em
  `devices/{user_id}/{device_id}/telemetry`.

### Rodando o simulador

Em um terminal separado (com o backend já no ar):

```bash
cd Backend/simulator
python3 -m venv .venv
source .venv/bin/activate          # Windows: .venv\Scripts\activate
pip install -r requirements.txt
python esp32_simulator.py 1 switch01
```

O primeiro argumento é o `user_id` (o usuário padrão criado na inicialização
tem `id = 1`) e o segundo é o `device_id`. Você verá no console algo como:

```
[1/switch01] conectado ao broker (reason_code=Success)
[1/switch01] escutando comandos em devices/1/switch01/command
[1/switch01] -> devices/1/switch01/state: {"relay": false}
[1/switch01] -> devices/1/switch01/telemetry: {"sp": 220.3, "mv": 219.6, "current": 0.0, "power": 1.2}
```

> Quer simular vários dispositivos? Basta rodar o script novamente com outro
> `device_id` (e/ou `user_id`), ex.: `python esp32_simulator.py 1 switch02`.

## 4. Ver tudo acontecendo ao vivo

Com o backend e o simulador rodando, abra um terceiro terminal e use a API
como se fosse o app/usuário controlando o dispositivo:

```bash
# 1) Login (não precisa de headers extras) — guarde o user_id retornado
curl -s -X POST http://localhost:8000/login \
  -H "Content-Type: application/json" \
  -d '{"username": "admin", "password": "admin"}'
# -> {"success": true, "user_id": 1}

# 2) Ligar o relé — publica em devices/1/switch01/command
curl -s -X POST http://localhost:8000/devices/switch01/relay/on \
  -H "X-Username: admin" -H "X-Password: admin"

# 3) Desligar o relé
curl -s -X POST http://localhost:8000/devices/switch01/relay/off \
  -H "X-Username: admin" -H "X-Password: admin"

# 4) Consultar o último estado relatado pelo dispositivo
curl -s http://localhost:8000/devices/switch01/state \
  -H "X-Username: admin" -H "X-Password: admin"

# 5) Consultar a última telemetria recebida
curl -s http://localhost:8000/devices/switch01/telemetry/latest \
  -H "X-Username: admin" -H "X-Password: admin"

# 6) Listar todos os dispositivos do usuário
curl -s http://localhost:8000/devices -H "X-Username: admin" -H "X-Password: admin"
```

> Repare que a URL não leva `user_id` — o backend identifica o usuário pelos
> headers `X-Username`/`X-Password` e só enxerga os dispositivos dele
> (`404` para dispositivos de outro usuário ou inexistentes). Internamente, o
> comando publicado pela API usa o tópico `devices/1/switch01/command`, com o
> `user_id` do usuário autenticado.

O que observar em cada terminal:

- **Terminal do simulador**: ao rodar o passo 2/3, aparece a linha
  `[1/switch01] <- devices/1/switch01/command: {"relay": true}` seguida da
  republicação do novo `state` — é o "dispositivo" reagindo ao comando.
- **Terminal `docker compose logs -f mqtt-consumer`**: mostra
  `Updated state for user 1 device switch01: relay=True` e
  `Inserted telemetry for user 1 device switch01` — é o consumer persistindo
  no PostgreSQL o que o dispositivo publicou.
- **Terminal com `curl`**: os comandos 4 e 5 retornam exatamente os dados que
  acabaram de ser persistidos, fechando o ciclo API → MQTT → dispositivo →
  MQTT → consumer → banco → API.

Para inspecionar o tráfego MQTT diretamente (sem passar pela API nem pelo
simulador), você pode escutar qualquer tópico usando o próprio container do
broker:

```bash
docker compose exec mosquitto mosquitto_sub -h localhost -t 'devices/#' -v
```

Se o broker exigir autenticação, acrescente `-u <usuário> -P <senha>`
manualmente ao comando acima (mesmas credenciais de `MQTT_USERNAME`/
`MQTT_PASSWORD`).

## 5. Encerrar tudo

```bash
# Pare o simulador com Ctrl+C no terminal dele

# Derrube os containers
cd Backend
docker compose down
```
