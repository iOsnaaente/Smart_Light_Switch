# Smart Light Switch — App

App Flutter que controla os interruptores inteligentes via o backend
FastAPI/MQTT em `../Backend`. Permite login, listar os dispositivos do
usuário, ligar/desligar o relé, alternar entre modo manual e automático,
ajustar dimmer/setpoint/limites, calibrar o sensor de luz, renomear ou
remover dispositivos e acompanhar a telemetria (lux, luz natural, potência,
consumo). Também inclui um **modo de demonstração** 100% offline (dados em
memória) para apresentar o app sem depender do backend.

## Estrutura

```
lib/
├── api/api_client.dart            # Cliente HTTP para o backend (login, devices, estado,
│                                  # telemetria, relé, modo, dimmer, setpoint, agendamentos...)
├── providers/
│   ├── auth_provider.dart         # Sessão (login real / modo demo) — ChangeNotifier
│   └── device_provider.dart       # Lista de dispositivos + comandos — ChangeNotifier
├── models/
│   ├── device.dart                # Modelo Device (fromJson/copyWith) usado pela UI
│   ├── device_models.dart         # DTOs da API: Telemetry, TelemetryPoint, Schedule
│   └── mock_devices.dart          # Dados em memória que alimentam o modo de demonstração
├── screens/
│   ├── login/login_page.dart            # Login (real ou demonstração)
│   ├── dashboard/devices_page.dart      # Lista de dispositivos por ambiente
│   ├── control/control_page.dart        # Controle do dispositivo (relé, modo, dimmer, lux)
│   └── settings/device_settings_page.dart # Setpoint, limites, calibração, renomear, remover
├── widgets/                        # Componentes visuais reutilizáveis (gauges, chips, cards...)
├── core/
│   ├── routes/app_routes.dart      # Rotas nomeadas (login, devices, control, settings)
│   └── theme/app_theme.dart        # Tema Material 3
└── main.dart                       # Entry point / MultiProvider / MaterialApp
```

## Pré-requisitos

- Flutter SDK 3.44+ (instalado em `~/dev/flutter` neste ambiente; adicione ao
  PATH com `export PATH="$HOME/dev/flutter/bin:$PATH"`)
- Backend rodando (veja `../Backend/README.md`):
  ```bash
  cd ../Backend && docker compose up --build
  ```
  A API deve responder em `http://localhost:8000` (teste com
  `curl -X POST http://localhost:8000/login -d '{"username":"admin","password":"admin"}' -H "Content-Type: application/json"`).

## Configuração da URL do backend

Por padrão o app aponta para `http://localhost:8000`. Para apontar para outro
endereço (ex.: emulador Android, que usa `10.0.2.2` para acessar o `localhost`
da máquina host), passe `--dart-define`:

```bash
flutter run --dart-define=API_BASE_URL=http://10.0.2.2:8000
```

## Como rodar/testar

### Verificações estáticas (não precisam de dispositivo gráfico)

```bash
flutter pub get
flutter analyze
flutter test
```

### Servidor web de desenvolvimento (recomendado neste ambiente WSL)

O WSL2 encaminha portas `localhost` automaticamente para o Windows, então é
possível rodar o servidor web aqui e abrir no navegador do Windows:

```bash
flutter run -d web-server --web-port=8080 --dart-define=API_BASE_URL=http://localhost:8000
```

Depois abra `http://localhost:8080` no navegador (Windows ou WSL). Use `r`
no terminal para hot reload e `q` para encerrar.

### Outras opções de dispositivo

- **Chrome/Edge**: instale o Chrome (`flutter doctor` aponta o caminho) e use
  `flutter run -d chrome` para um fluxo de desenvolvimento com hot reload e
  DevTools completos.
- **Linux desktop**: requer `libgtk-3-dev` (`sudo apt install libgtk-3-dev`),
  depois `flutter run -d linux`.
- **Android**: conecte um dispositivo/emulador (`flutter devices`) e rode
  `flutter run --dart-define=API_BASE_URL=http://10.0.2.2:8000`.

## Login de teste

Use as credenciais padrão do backend (configuráveis em `Backend/.env.example`):

```
usuário: admin
senha:   admin
```

## Modo de demonstração (sem backend)

Na tela de login, toque em **"Entrar como demonstração"** para navegar pelo
app inteiro com dados em memória (`lib/models/mock_devices.dart`) — útil para
apresentações quando o backend não está disponível. Nesse modo os comandos
(relé, modo, dimmer, setpoint...) só atualizam o estado local; nada é enviado
à rede.

## Simulando o dispositivo (sem hardware)

Com o backend no ar, use o simulador Python descrito em
`../Backend/README.md` (`Backend/simulator/esp32_simulator.py`) — ele
conversa com a API/MQTT como um ESP32 real e mantém o app sempre com dados
atualizados. Se preferir publicar manualmente, lembre-se de que os tópicos
incluem o `user_id` (`devices/{user_id}/{device_id}/...`; o usuário `admin`
padrão tem `user_id = 1`):

```bash
mosquitto_pub -h localhost -t devices/1/switch01/state \
  -m '{"relay": true, "automatic_mode": false, "dimmer": 70, "rssi": -55}'
mosquitto_pub -h localhost -t devices/1/switch01/telemetry \
  -m '{"sp": 220.1, "mv": 219.8, "current": 0.55, "power": 121.0, "lux": 410.5, "natural_lux": 180.2, "dimmer": 70, "kwh_today": 0.532}'
```

> **Broker com senha:** se o Mosquitto exigir autenticação
> (`allow_anonymous false`), acrescente `-u <usuário> -P <senha>` manualmente
> em cada `mosquitto_pub`/`mosquitto_sub` — as mesmas credenciais configuradas
> em `MQTT_USERNAME`/`MQTT_PASSWORD` (ver `Backend/.env.example` e
> `Firmware/main/main.cpp`):
>
> ```bash
> mosquitto_pub -h localhost -u smart-light-device -P troque-esta-senha \
>   -t devices/1/switch01/state -m '{"relay": true, "dimmer": 70}'
> ```
