# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

Smart Light Switch is a complete smart-dimmer system with three independently
versioned components that talk to each other through one shared MQTT/REST
contract — not three separate apps:

- **`Firmware/`** — ESP32-C3 firmware (ESP-IDF v5.5, FreeRTOS, C++). Reads an
  LDR + power meter, drives a relay and a TRIAC dimmer, shows a local TFT
  panel (LVGL), and speaks MQTT to the backend. **Never** talks to the REST API.
- **`Backend/`** — FastAPI + SQLAlchemy + PostgreSQL + Mosquitto, run via
  Docker Compose. Authenticates users, exposes the REST API the app uses,
  publishes MQTT commands to devices, and persists state/telemetry the
  devices publish. **The only component that sees both worlds** (MQTT device
  side and REST app side).
- **`App/`** — Flutter app (Material 3, `provider`). Lists devices, controls
  relay/mode/dimmer/setpoint, shows telemetry, manages settings. **Never**
  speaks MQTT directly — only HTTP to the backend.

Read the root [README.md](README.md) first for the end-to-end architecture
diagram and the MQTT contract table; it's the map that ties the three
sub-READMEs together. Each component also has its own README with
component-specific detail (`Firmware/README.md`, `Backend/README.md`,
`App/README.md`).

## The MQTT contract (the cross-component "API")

Devices are addressed as `devices/{user_id}/{device_id}/{command|state|telemetry}`
(the `user_id` prefix lets different users have a device named e.g. `switch01`
without colliding). When changing *anything* that touches this contract —
firmware publish/subscribe code, backend `app/mqtt/*`, or app DTOs in
`App/lib/models/device_models.dart` — update **all three** sides and the
contract table in the root README. A mismatch here is the single most likely
way to silently break end-to-end integration.

| Topic | Direction | Payload shape |
|---|---|---|
| `.../command` | Backend → Firmware | `{"relay": bool}` · `{"mode": "auto"\|"manual"}` · `{"dimmer": 0-100}` · `{"setpoint": lux}` · `{"dimmer_limits": {"min", "max"}}` · `{"calibrate": true}` |
| `.../state` | Firmware → Backend | `{"relay", "automatic_mode", "dimmer", "rssi", "firmware", "wifi_name"}` |
| `.../telemetry` | Firmware → Backend | `{"sp", "mv", "current", "power", "lux", "natural_lux", "dimmer", "kwh_today"}` |

Any `state`/`telemetry` message is an implicit heartbeat — the backend marks a
device `online` if it heard from it in the last 2 minutes; that same field
drives the "last seen Xh ago" UI in the app.

## Firmware (`Firmware/`)

ESP-IDF project, organized as components wired together by a custom
`esp_event`-based event bus (`Firmware/system/event_bus.h`/`.cpp`,
events declared in `Firmware/system/app_events.h` as
`SMART_SWITCH_EVENT_{LDR_UPDATE, DIMMER_UPDATE, SETPOINT_CHANGED,
MODE_CHANGED, POWER_UPDATE, NET_STATUS, RELAY_COMMAND, DIMMER_COMMAND,
BLE_PROVISION_COMMAND, BLE_PROVISION_STATUS}`). Sensors, controllers, comms
and UI never call each other directly — they post/subscribe to this bus,
which is what keeps the layers decoupled. When wiring new behavior, prefer
adding an event over adding a direct cross-module call.

Layout: `main/` (entry point + Wi-Fi/MQTT credentials in `credentials.h`,
gitignored), `sensors/{LDR,PowerMeter}/`, `controllers/` (TRIAC dimmer),
`communication/` (`wifi_manager`, `ble_setup`, `mqtt_client`/`mqtt_manager`,
`comm_manager` orchestrates them, `device_identity` derives the MQTT
`device_id`), `system/` (event bus + app modes), `ui/` (LVGL screens,
wireframes), `components/` (vendored: `ili9340`, `lvgl`, `esp-idf-ili9340`).
`Firmware/lib/TFT_eSPI` and `components/lvgl`/`components/esp-idf-ili9340`
are git submodules (`.gitmodules`).

### Build / flash / monitor

ESP-IDF lives at `~/esp/v5.5.1/esp-idf/export.sh`. **You must `source`
it and run `idf.py` in the *same* shell invocation** — the exported PATH
does not survive across separate tool calls:

```bash
cd Firmware && source ~/esp/v5.5.1/esp-idf/export.sh > /dev/null 2>&1 && idf.py build
```

```bash
idf.py -p /dev/ttyUSB0 flash monitor   # flash + serial monitor (after sourcing export.sh)
idf.py menuconfig                       # ESP-IDF/project configuration
idf.py fullclean                         # wipe build/ (rarely needed)
```

A clean build produces `build/Firmware.bin` around `0x1d7270` bytes (~41%
free in the 0x320000 app partition) — a useful sanity check that a change
hasn't bloated the binary unexpectedly.

## Backend (`Backend/`)

FastAPI app at `app/main.py`, organized by concern:
`api/` (routers: `auth`, `devices`), `core/` (`config` settings via
pydantic-settings, `auth` — a `SimpleAuthMiddleware` that reads
`X-Username`/`X-Password` headers, **no JWT**, `security`), `db/`
(SQLAlchemy session/engine), `models/` (`user`, `device`, `device_state`,
`telemetry`, `schedule`), `schemas/` (Pydantic request/response DTOs),
`services/` (`user_service`, `device_service`, `schedule_service` — business
logic sits here, not in routers), `mqtt/` (`publisher` — backend → device
commands, `consumer` — a **separate process** that subscribes to
`state`/`telemetry` and persists them; run via
`python -m app.mqtt.consumer`).

On startup (`lifespan` in `main.py`) it creates tables, ensures a default
user from `AUTH_USERNAME`/`AUTH_PASSWORD` env vars (`admin`/`admin` by
default — see `.env.example`), and connects the MQTT publisher.

### Run / develop

Everything runs via Docker Compose (`postgres`, `mosquitto`, `api`,
`mqtt-consumer` — the latter two share one image, differentiated by the
`command:` override running `app.mqtt.consumer` as a module):

```bash
cd Backend
cp .env.example .env   # fill in real values first time
docker compose up --build -d
docker compose config --quiet   # validate compose file without starting anything
curl http://localhost:8000/health
```

No automated test suite exists for the backend yet. The
[`Backend/simulator/esp32_simulator.py`](Backend/simulator/esp32_simulator.py)
script behaves like a real device over MQTT (subscribes to `command`,
publishes `state`/`telemetry`) — use it to exercise the full
API → MQTT → device → MQTT → consumer → DB → API loop without hardware:

```bash
cd Backend/simulator && python3 -m venv .venv && source .venv/bin/activate
pip install -r requirements.txt
python esp32_simulator.py 1 switch01   # user_id=1 (admin), device_id=switch01
```

CI (`.github/workflows/docker.yml`) builds and pushes
`iosnaaente/smart-switch-backend:{latest,<sha>}` to Docker Hub on every push
to `main` that touches `Backend/**` — that's the same image
`docker-compose.yml` runs for both `api` and `mqtt-consumer`.

## App (`App/`)

Flutter, Material 3, state managed with `provider` (`ChangeNotifier`).
Layout: `api/api_client.dart` (the only thing that talks HTTP to the
backend), `providers/` (`AuthProvider` — session/login incl. demo mode,
`DeviceProvider` — device list + commands), `models/` (`device`,
`device_models` — API DTOs like `Telemetry`/`Schedule`, `mock_devices` — the
in-memory data behind demo mode), `screens/{login,dashboard,control,settings}`,
`widgets/` (reusable presentational components), `core/{routes,theme}`.

Two SDK installs exist on this machine — only one works for this project:

```bash
export PATH="$HOME/dev/flutter/bin:$PATH"   # Linux-native build, matches pubspec's ^3.12.1
# /mnt/c/dev/flutter is a Windows-native install (.exe/CRLF) — unusable from WSL, do not use it
```

```bash
cd App
flutter pub get
flutter analyze
flutter test                                          # run all tests
flutter test test/widget_test.dart                    # run a single test file
flutter test --plain-name "Login button shows error"  # run one test by name
flutter run --dart-define=API_BASE_URL=http://localhost:8000          # default backend URL
flutter run --dart-define=API_BASE_URL=http://10.0.2.2:8000           # Android emulator → host
flutter run -d web-server --web-port=8080 --dart-define=API_BASE_URL=http://localhost:8000  # WSL: forwards to Windows browser
flutter build linux --debug
```

The app has a fully offline **demo mode** ("Entrar como demonstração" on the
login screen) backed by `lib/models/mock_devices.dart` — useful for testing
UI changes or presenting without a backend; commands in this mode only
mutate local state and never hit the network. `lib/main.dart` exposes an
optional `apiClient` constructor param on `SmartLightApp` specifically so
widget tests can inject a fake `ApiClient` instead of making real HTTP calls
(see `_RejectingApiClient` in `test/widget_test.dart` for the pattern).

Default login credentials for both the app and `curl`/simulator testing:
`admin` / `admin` (configurable via `AUTH_USERNAME`/`AUTH_PASSWORD` in
`Backend/.env.example`), sent as `X-Username`/`X-Password` headers.
