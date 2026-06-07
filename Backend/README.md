# Smart Light Switch — Backend (PoC IoT)

Backend de prova de conceito para controle de interruptores inteligentes baseados em
ESP32, com comunicação via MQTT, persistência em PostgreSQL e API HTTP em FastAPI.

## Arquitetura

- **mosquitto**: broker MQTT.
- **postgres**: banco de dados relacional.
- **api**: API FastAPI que autentica usuários, publica comandos MQTT e consulta dados no banco.
- **mqtt-consumer**: serviço que assina os tópicos `devices/+/state` e `devices/+/telemetry`,
  persistindo o estado e a telemetria dos dispositivos no banco.

## Tópicos MQTT

| Tópico                         | Direção          | Payload                                              |
|--------------------------------|------------------|------------------------------------------------------|
| `devices/{device_id}/command`  | API → dispositivo | `{"relay": true}`                                    |
| `devices/{device_id}/state`    | dispositivo → API | `{"relay": true}`                                    |
| `devices/{device_id}/telemetry`| dispositivo → API | `{"sp": 220.1, "mv": 219.8, "current": 0.55, "power": 121.0}` |

## Autenticação

Autenticação simplificada via middleware HTTP, sem JWT, sem cadastro e sem
recuperação de senha. As credenciais fixas são `admin` / `admin`.

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

As tabelas (`devices`, `device_state`, `telemetry`) são criadas automaticamente
na inicialização da API e do consumidor MQTT.

## Endpoints

### `POST /login`

Valida usuário e senha fixos.

```bash
curl -X POST http://localhost:8000/login \
  -H "Content-Type: application/json" \
  -d '{"username": "admin", "password": "admin"}'
```

Resposta:

```json
{"success": true}
```

### `POST /devices/{device_id}/relay/on`

Publica `{"relay": true}` em `devices/{device_id}/command`.

```bash
curl -X POST http://localhost:8000/devices/switch01/relay/on \
  -H "X-Username: admin" -H "X-Password: admin"
```

### `POST /devices/{device_id}/relay/off`

Publica `{"relay": false}` em `devices/{device_id}/command`.

```bash
curl -X POST http://localhost:8000/devices/switch01/relay/off \
  -H "X-Username: admin" -H "X-Password: admin"
```

### `GET /devices/{device_id}/state`

Retorna o último estado do dispositivo armazenado no banco.

```bash
curl http://localhost:8000/devices/switch01/state \
  -H "X-Username: admin" -H "X-Password: admin"
```

### `GET /devices/{device_id}/telemetry/latest`

Retorna o último registro de telemetria do dispositivo.

```bash
curl http://localhost:8000/devices/switch01/telemetry/latest \
  -H "X-Username: admin" -H "X-Password: admin"
```

## Testando com mosquitto_pub (simulando o ESP32)

Simular o envio de estado e telemetria por um dispositivo `switch01`:

```bash
# Estado
mosquitto_pub -h localhost -t devices/switch01/state -m '{"relay": true}'

# Telemetria
mosquitto_pub -h localhost -t devices/switch01/telemetry \
  -m '{"sp": 220.1, "mv": 219.8, "current": 0.55, "power": 121.0}'
```

Para escutar comandos publicados pela API:

```bash
mosquitto_sub -h localhost -t 'devices/+/command' -v
```

## Estrutura de diretórios

```
Backend/
├── app/
│   ├── api/            # Rotas HTTP (login, devices)
│   ├── core/           # Configuração e middleware de autenticação
│   ├── db/             # Sessão e Base do SQLAlchemy
│   ├── models/         # Modelos ORM (Device, DeviceState, Telemetry)
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

- **devices**: `id`, `device_id`, `name`, `created_at`
- **device_state**: `id`, `device_id`, `relay`, `updated_at`
- **telemetry**: `id`, `device_id`, `sp`, `mv`, `current`, `power`, `timestamp`
