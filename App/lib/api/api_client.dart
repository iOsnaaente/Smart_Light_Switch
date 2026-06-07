import 'dart:convert';

import 'package:http/http.dart' as http;

import '../models/device_models.dart';

const String _defaultBaseUrl = 'http://localhost:8000';

class ApiException implements Exception {
  final String message;
  ApiException(this.message);

  @override
  String toString() => message;
}

class ApiClient {
  final String baseUrl;
  String? username;
  String? password;

  ApiClient({String? baseUrl})
      : baseUrl = baseUrl ??
            const String.fromEnvironment('API_BASE_URL', defaultValue: _defaultBaseUrl);

  Map<String, String> get _authHeaders => {
        'X-Username': ?username,
        'X-Password': ?password,
      };

  Future<bool> login(String username, String password) async {
    final response = await http.post(
      Uri.parse('$baseUrl/login'),
      headers: {'Content-Type': 'application/json'},
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
    }
    return success;
  }

  Future<DeviceState> getState(String deviceId) async {
    final response = await http.get(
      Uri.parse('$baseUrl/devices/$deviceId/state'),
      headers: _authHeaders,
    );
    _checkStatus(response);
    return DeviceState.fromJson(jsonDecode(response.body) as Map<String, dynamic>);
  }

  Future<Telemetry> getLatestTelemetry(String deviceId) async {
    final response = await http.get(
      Uri.parse('$baseUrl/devices/$deviceId/telemetry/latest'),
      headers: _authHeaders,
    );
    _checkStatus(response);
    return Telemetry.fromJson(jsonDecode(response.body) as Map<String, dynamic>);
  }

  Future<bool> setRelay(String deviceId, bool on) async {
    final action = on ? 'on' : 'off';
    final response = await http.post(
      Uri.parse('$baseUrl/devices/$deviceId/relay/$action'),
      headers: _authHeaders,
    );
    _checkStatus(response);
    final body = jsonDecode(response.body) as Map<String, dynamic>;
    return body['relay'] as bool;
  }

  void _checkStatus(http.Response response) {
    if (response.statusCode == 401 || response.statusCode == 403) {
      throw ApiException('Credenciais inválidas');
    }
    if (response.statusCode == 404) {
      throw ApiException('Dispositivo não encontrado');
    }
    if (response.statusCode >= 400) {
      throw ApiException('Erro ao comunicar com o servidor (HTTP ${response.statusCode})');
    }
  }
}
