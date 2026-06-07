# Smart Light Switch — App (MVP)

App Flutter (MVP) para controlar o interruptor inteligente via o backend
FastAPI/MQTT em `../Backend`. Permite login, visualizar o estado do relé
(ligado/desligado), alternar o relé e acompanhar a telemetria mais recente
(tensão, corrente, potência) do dispositivo `switch01`.

## Estrutura

```
lib/
├── api/api_client.dart        # Cliente HTTP para o backend (login, estado, telemetria, relé)
├── models/device_models.dart  # Modelos DeviceState e Telemetry
├── screens/login_screen.dart  # Tela de login
├── screens/dashboard_screen.dart # Painel do dispositivo (relé + telemetria)
└── main.dart                  # Entry point / MaterialApp
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

## Simulando o dispositivo (sem hardware)

Para gerar dados de estado/telemetria que o app exibirá, publique no broker
MQTT (veja `../Backend/README.md`):

```bash
mosquitto_pub -h localhost -t devices/switch01/state -m '{"relay": true}'
mosquitto_pub -h localhost -t devices/switch01/telemetry \
  -m '{"sp": 220.1, "mv": 219.8, "current": 0.55, "power": 121.0}'
```
