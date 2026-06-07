#!/bin/bash

set -e

echo "======================================="
echo " Smart Light Switch Bootstrap"
echo "======================================="

# ----------------------------
# Estrutura de diretórios
# ----------------------------

mkdir -p lib/core/theme
mkdir -p lib/core/routes
mkdir -p lib/core/constants

mkdir -p lib/models

mkdir -p lib/services

mkdir -p lib/repositories

mkdir -p lib/providers

mkdir -p lib/screens/login
mkdir -p lib/screens/dashboard
mkdir -p lib/screens/control
mkdir -p lib/screens/settings

mkdir -p lib/widgets

mkdir -p assets/images
mkdir -p assets/icons

# ----------------------------
# pubspec.yaml reminder
# ----------------------------

echo ""
echo "Não esqueça de adicionar:"
echo "provider"
echo "dio"
echo ""

# ----------------------------
# main.dart
# ----------------------------

cat > lib/main.dart << 'EOF'
import 'package:flutter/material.dart';

import 'core/routes/app_routes.dart';
import 'core/theme/app_theme.dart';

void main() {
  runApp(const SmartLightApp());
}

class SmartLightApp extends StatelessWidget {
  const SmartLightApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Smart Light Switch',
      debugShowCheckedModeBanner: false,
      theme: AppTheme.lightTheme,
      initialRoute: AppRoutes.login,
      routes: AppRoutes.routes,
    );
  }
}
EOF

# ----------------------------
# Theme
# ----------------------------

cat > lib/core/theme/app_theme.dart << 'EOF'
import 'package:flutter/material.dart';

class AppTheme {
  static ThemeData get lightTheme {
    return ThemeData(
      useMaterial3: true,
      colorSchemeSeed: Colors.amber,
      scaffoldBackgroundColor: Colors.white,
    );
  }
}
EOF

# ----------------------------
# Routes
# ----------------------------

cat > lib/core/routes/app_routes.dart << 'EOF'
import 'package:flutter/material.dart';

import '../../screens/login/login_page.dart';
import '../../screens/dashboard/devices_page.dart';
import '../../screens/control/control_page.dart';
import '../../screens/settings/device_settings_page.dart';

class AppRoutes {
  static const login = '/';
  static const devices = '/devices';
  static const control = '/control';
  static const settings = '/settings';

  static Map<String, WidgetBuilder> get routes => {
        login: (_) => const LoginPage(),
        devices: (_) => const DevicesPage(),
        control: (_) => const ControlPage(),
        settings: (_) => const DeviceSettingsPage(),
      };
}
EOF

# ----------------------------
# API Constants
# ----------------------------

cat > lib/core/constants/api_constants.dart << 'EOF'
class ApiConstants {
  static const String baseUrl = String.fromEnvironment(
    'API_BASE_URL',
    defaultValue: 'http://localhost:8000',
  );
}
EOF

# ----------------------------
# Models
# ----------------------------

cat > lib/models/device.dart << 'EOF'
class Device {
  final String id;
  final String name;
  final String room;

  final bool online;
  final bool automaticMode;

  final double lux;
  final double setpoint;

  final int dimmer;

  Device({
    required this.id,
    required this.name,
    required this.room,
    required this.online,
    required this.automaticMode,
    required this.lux,
    required this.setpoint,
    required this.dimmer,
  });

  factory Device.fromJson(Map<String, dynamic> json) {
    return Device(
      id: json['id'],
      name: json['name'],
      room: json['room'],
      online: json['online'],
      automaticMode: json['automatic_mode'],
      lux: (json['lux'] as num).toDouble(),
      setpoint: (json['setpoint'] as num).toDouble(),
      dimmer: json['dimmer'],
    );
  }
}
EOF

cat > lib/models/telemetry.dart << 'EOF'
class Telemetry {
  final double lux;
  final double naturalLux;
  final double power;
  final int dimmer;

  Telemetry({
    required this.lux,
    required this.naturalLux,
    required this.power,
    required this.dimmer,
  });
}
EOF

cat > lib/models/user.dart << 'EOF'
class User {
  final String username;

  User(this.username);
}
EOF

# ----------------------------
# Services
# ----------------------------

cat > lib/services/api_service.dart << 'EOF'
import 'package:dio/dio.dart';

import '../core/constants/api_constants.dart';

class ApiService {
  final Dio dio = Dio(
    BaseOptions(
      baseUrl: ApiConstants.baseUrl,
      connectTimeout: const Duration(seconds: 5),
      receiveTimeout: const Duration(seconds: 5),
    ),
  );
}
EOF

cat > lib/services/auth_service.dart << 'EOF'
class AuthService {}
EOF

cat > lib/services/device_service.dart << 'EOF'
class DeviceService {}
EOF

# ----------------------------
# Repositories
# ----------------------------

cat > lib/repositories/auth_repository.dart << 'EOF'
class AuthRepository {}
EOF

cat > lib/repositories/device_repository.dart << 'EOF'
class DeviceRepository {}
EOF

# ----------------------------
# Providers
# ----------------------------

cat > lib/providers/auth_provider.dart << 'EOF'
import 'package:flutter/foundation.dart';

class AuthProvider extends ChangeNotifier {}
EOF

cat > lib/providers/device_provider.dart << 'EOF'
import 'package:flutter/foundation.dart';

class DeviceProvider extends ChangeNotifier {}
EOF

# ----------------------------
# Widgets
# ----------------------------

cat > lib/widgets/status_chip.dart << 'EOF'
import 'package:flutter/material.dart';

class StatusChip extends StatelessWidget {
  final bool online;

  const StatusChip({
    super.key,
    required this.online,
  });

  @override
  Widget build(BuildContext context) {
    return Chip(
      label: Text(
        online ? 'ONLINE' : 'OFFLINE',
      ),
    );
  }
}
EOF

touch lib/widgets/device_card.dart
touch lib/widgets/lux_gauge.dart
touch lib/widgets/dimmer_slider.dart
touch lib/widgets/app_scaffold.dart

# ----------------------------
# Login Page
# ----------------------------

cat > lib/screens/login/login_page.dart << 'EOF'
import 'package:flutter/material.dart';

class LoginPage extends StatelessWidget {
  const LoginPage({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('SmartLight'),
      ),
      body: Center(
        child: ElevatedButton(
          onPressed: () {
            Navigator.pushReplacementNamed(
              context,
              '/devices',
            );
          },
          child: const Text('Entrar'),
        ),
      ),
    );
  }
}
EOF

# ----------------------------
# Devices Page
# ----------------------------

cat > lib/screens/dashboard/devices_page.dart << 'EOF'
import 'package:flutter/material.dart';

class DevicesPage extends StatelessWidget {
  const DevicesPage({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Dispositivos'),
      ),
      body: ListView(
        children: [
          ListTile(
            title: const Text('Mesa de Trabalho'),
            subtitle: const Text('Sala'),
            onTap: () {
              Navigator.pushNamed(
                context,
                '/control',
              );
            },
          ),
        ],
      ),
    );
  }
}
EOF

# ----------------------------
# Control Page
# ----------------------------

cat > lib/screens/control/control_page.dart << 'EOF'
import 'package:flutter/material.dart';

class ControlPage extends StatelessWidget {
  const ControlPage({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Controle'),
      ),
      body: const Center(
        child: Text('Tela de Controle'),
      ),
    );
  }
}
EOF

# ----------------------------
# Settings Page
# ----------------------------

cat > lib/screens/settings/device_settings_page.dart << 'EOF'
import 'package:flutter/material.dart';

class DeviceSettingsPage extends StatelessWidget {
  const DeviceSettingsPage({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Configurações'),
      ),
      body: const Center(
        child: Text('Configurações do dispositivo'),
      ),
    );
  }
}
EOF

echo ""
echo "======================================="
echo " Estrutura criada com sucesso"
echo "======================================="
echo ""
echo "Próximos passos:"
echo ""
echo "flutter pub add provider"
echo "flutter pub add dio"
echo ""
echo "flutter run -d web-server"
echo ""
