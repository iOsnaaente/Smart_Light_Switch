import 'dart:convert';

import 'package:http/http.dart' as http;

import '../models/device.dart';
import '../models/device_models.dart';

const String _defaultBaseUrl = 'http://159.223.128.43:8000';

class ApiException implements Exception {
  final String message;
  ApiException(this.message);

  @override
  String toString() => message;
}

/// Thin HTTP client for the FastAPI backend documented in `Backend/README.md`.
///
/// Auth is header-based (`X-Username`/`X-Password`, set by [login]) rather
/// than token-based — every authenticated request resends the credentials,
/// matching `SimpleAuthMiddleware` on the server.
class ApiClient {
  final String baseUrl;
  String? username;
  String? password;
  int? userId;

  ApiClient({String? baseUrl})
      : baseUrl = baseUrl ??
            const String.fromEnvironment('API_BASE_URL', defaultValue: _defaultBaseUrl);

  bool get isAuthenticated => username != null && password != null;

  Map<String, String> get _authHeaders => {
        'X-Username': ?username,
        'X-Password': ?password,
      };

  Map<String, String> get _jsonHeaders => {
        ..._authHeaders,
        'Content-Type': 'application/json',
      };

  Uri _uri(String path) => Uri.parse('$baseUrl$path');

  /* ---- autenticação -------------------------------------------------------- */

  Future<bool> login(String username, String password) async {
    final response = await http.post(
      _uri('/login'),
      headers: const {'Content-Type': 'application/json'},
      body: jsonEncode({'username': username, 'password': password}),
    );
    if (response.statusCode != 200) {
      throw ApiException('Falha ao conectar ao servidor (HTTP ${response.statusCode})');
    }
    final body = jsonDecode(response.body) as Map<String, dynamic>;
    final success = body['success'] as bool? ?? false;
    if (success) {
      this.username = username;
      this.password = password;
      userId = body['user_id'] as int?;
    }
    return success;
  }

  void logout() {
    username = null;
    password = null;
    userId = null;
  }

  /* ---- dispositivos --------------------------------------------------------- */

  Future<List<Device>> listDevices() async {
    final response = await http.get(_uri('/devices'), headers: _authHeaders);
    _checkStatus(response);
    final list = jsonDecode(response.body) as List<dynamic>;
    return list.map((e) => Device.fromJson(e as Map<String, dynamic>)).toList();
  }

  Future<Device> getDevice(String deviceId) async {
    final response = await http.get(_uri('/devices/$deviceId'), headers: _authHeaders);
    _checkStatus(response);
    return Device.fromJson(jsonDecode(response.body) as Map<String, dynamic>);
  }

  Future<Device> updateDevice(String deviceId, {String? name, String? room}) async {
    final response = await http.put(
      _uri('/devices/$deviceId'),
      headers: _jsonHeaders,
      body: jsonEncode({
        'name': ?name,
        'room': ?room,
      }),
    );
    _checkStatus(response);
    return Device.fromJson(jsonDecode(response.body) as Map<String, dynamic>);
  }

  Future<void> deleteDevice(String deviceId) async {
    final response = await http.delete(_uri('/devices/$deviceId'), headers: _authHeaders);
    _checkStatus(response);
  }

  Future<DeviceState> getState(String deviceId) async {
    final response = await http.get(_uri('/devices/$deviceId/state'), headers: _authHeaders);
    _checkStatus(response);
    return DeviceState.fromJson(jsonDecode(response.body) as Map<String, dynamic>);
  }

  Future<Telemetry> getLatestTelemetry(String deviceId) async {
    final response = await http.get(_uri('/devices/$deviceId/telemetry/latest'), headers: _authHeaders);
    _checkStatus(response);
    return Telemetry.fromJson(jsonDecode(response.body) as Map<String, dynamic>);
  }

  Future<List<TelemetryPoint>> getTelemetryHistory(String deviceId, {String range = 'today'}) async {
    final response = await http.get(
      _uri('/devices/$deviceId/telemetry/history').replace(queryParameters: {'range': range}),
      headers: _authHeaders,
    );
    _checkStatus(response);
    final list = jsonDecode(response.body) as List<dynamic>;
    return list.map((e) => TelemetryPoint.fromJson(e as Map<String, dynamic>)).toList();
  }

  /* ---- comandos -------------------------------------------------------------- */
  /* Cada um publica em devices/{user_id}/{device_id}/command (ver Backend/app/mqtt/publisher.py)
   * e devolve só a confirmação de que a mensagem foi publicada — o novo
   * estado "de fato" chega depois, via telemetria/estado MQTT do dispositivo. */

  Future<bool> setRelay(String deviceId, bool on) async {
    final action = on ? 'on' : 'off';
    final response = await http.post(_uri('/devices/$deviceId/relay/$action'), headers: _authHeaders);
    _checkStatus(response);
    final body = jsonDecode(response.body) as Map<String, dynamic>;
    return body['relay'] as bool;
  }

  Future<bool> setMode(String deviceId, bool automaticMode) async {
    final response = await http.post(
      _uri('/devices/$deviceId/mode'),
      headers: _jsonHeaders,
      body: jsonEncode({'automatic_mode': automaticMode}),
    );
    _checkStatus(response);
    final body = jsonDecode(response.body) as Map<String, dynamic>;
    return body['automatic_mode'] as bool;
  }

  Future<int> setDimmer(String deviceId, int value) async {
    final response = await http.post(
      _uri('/devices/$deviceId/dimmer'),
      headers: _jsonHeaders,
      body: jsonEncode({'dimmer': value}),
    );
    _checkStatus(response);
    final body = jsonDecode(response.body) as Map<String, dynamic>;
    return body['dimmer'] as int;
  }

  Future<Device> setSetpoint(String deviceId, double setpoint) async {
    final response = await http.put(
      _uri('/devices/$deviceId/setpoint'),
      headers: _jsonHeaders,
      body: jsonEncode({'setpoint': setpoint}),
    );
    _checkStatus(response);
    return Device.fromJson(jsonDecode(response.body) as Map<String, dynamic>);
  }

  Future<Device> setDimmerLimits(String deviceId, int dimmerMin, int dimmerMax) async {
    final response = await http.put(
      _uri('/devices/$deviceId/dimmer-limits'),
      headers: _jsonHeaders,
      body: jsonEncode({'dimmer_min': dimmerMin, 'dimmer_max': dimmerMax}),
    );
    _checkStatus(response);
    return Device.fromJson(jsonDecode(response.body) as Map<String, dynamic>);
  }

  Future<bool> calibrateSensor(String deviceId) async {
    final response = await http.post(_uri('/devices/$deviceId/calibrate-sensor'), headers: _authHeaders);
    _checkStatus(response);
    final body = jsonDecode(response.body) as Map<String, dynamic>;
    return body['published'] as bool? ?? false;
  }

  /* ---- agendamentos ---------------------------------------------------------- */

  Future<List<Schedule>> listSchedules(String deviceId) async {
    final response = await http.get(_uri('/devices/$deviceId/schedules'), headers: _authHeaders);
    _checkStatus(response);
    final list = jsonDecode(response.body) as List<dynamic>;
    return list.map((e) => Schedule.fromJson(e as Map<String, dynamic>)).toList();
  }

  Future<Schedule> createSchedule(
    String deviceId, {
    required String label,
    required String time,
    required List<String> daysOfWeek,
    required String action,
  }) async {
    final response = await http.post(
      _uri('/devices/$deviceId/schedules'),
      headers: _jsonHeaders,
      body: jsonEncode({
        'label': label,
        'time': time,
        'days_of_week': daysOfWeek,
        'action': action,
      }),
    );
    _checkStatus(response);
    return Schedule.fromJson(jsonDecode(response.body) as Map<String, dynamic>);
  }

  Future<void> deleteSchedule(String deviceId, int scheduleId) async {
    final response = await http.delete(_uri('/devices/$deviceId/schedules/$scheduleId'), headers: _authHeaders);
    _checkStatus(response);
  }

  /* ---- infra ------------------------------------------------------------------ */

  void _checkStatus(http.Response response) {
    if (response.statusCode == 401 || response.statusCode == 403) {
      throw ApiException('Credenciais inválidas');
    }
    if (response.statusCode == 404) {
      throw ApiException('Recurso não encontrado');
    }
    if (response.statusCode >= 400) {
      throw ApiException('Erro ao comunicar com o servidor (HTTP ${response.statusCode})');
    }
  }
}
