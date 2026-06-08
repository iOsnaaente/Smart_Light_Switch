import 'package:flutter/foundation.dart';

import '../api/api_client.dart';
import '../models/device.dart';
import '../models/device_models.dart';
import '../models/mock_devices.dart';

/// Owns the device list shown across the app and mediates every read/command
/// against the [ApiClient] — or, in demo mode, against the in-memory
/// [mockDevices], so the UI stays fully explorable with no Backend running.
///
/// Commands (relay/mode/dimmer/setpoint/...) only publish an MQTT command to
/// the device (see Backend/app/mqtt/publisher.py); the response carries just
/// an ack, not the device's confirmed state. We fold that ack into the local
/// copy so the UI reacts immediately, the same way it would once the device
/// itself echoes the change back over `.../state` or `.../telemetry`.
class DeviceProvider extends ChangeNotifier {
  final ApiClient _api;

  bool _demoMode = false;
  List<Device> _devices = const [];
  bool _loading = false;
  String? _error;

  DeviceProvider(this._api);

  List<Device> get devices => _devices;
  bool get loading => _loading;
  String? get error => _error;
  bool get demoMode => _demoMode;

  Device? deviceById(String id) {
    for (final device in _devices) {
      if (device.id == id) return device;
    }
    return null;
  }

  /// Switches to fully offline demo data — backs the "entrar como
  /// demonstração" flow so the UI can be presented without a Backend.
  void enableDemoMode() {
    _demoMode = true;
    _loading = false;
    _error = null;
    _devices = List.of(mockDevices);
    notifyListeners();
  }

  /// Drops all session state — called on logout so a later login starts clean.
  void reset() {
    _demoMode = false;
    _loading = false;
    _error = null;
    _devices = const [];
    notifyListeners();
  }

  Future<void> loadDevices() async {
    if (_demoMode) return;
    _loading = true;
    _error = null;
    notifyListeners();
    try {
      _devices = await _api.listDevices();
    } catch (e) {
      _error = e.toString();
    } finally {
      _loading = false;
      notifyListeners();
    }
  }

  Future<void> refreshDevice(String id) async {
    if (_demoMode) {
      // Não há servidor para "religar" o dispositivo — simula a reconexão
      // localmente para o botão "Tentar reconectar" continuar útil na demo.
      _update(id, (d) => d.copyWith(online: true, relayOn: true, lastSeen: 'agora'));
      return;
    }
    final updated = await _api.getDevice(id);
    _replace(updated);
  }

  Future<void> setRelay(String id, bool value) async {
    if (_demoMode) {
      _update(id, (d) => d.copyWith(relayOn: value));
      return;
    }
    final relay = await _api.setRelay(id, value);
    _update(id, (d) => d.copyWith(relayOn: relay));
  }

  Future<void> setMode(String id, bool automaticMode) async {
    if (_demoMode) {
      _update(id, (d) => d.copyWith(automaticMode: automaticMode));
      return;
    }
    final mode = await _api.setMode(id, automaticMode);
    _update(id, (d) => d.copyWith(automaticMode: mode));
  }

  Future<void> setDimmer(String id, int value) async {
    if (_demoMode) {
      _update(id, (d) => d.copyWith(dimmer: value));
      return;
    }
    final dimmer = await _api.setDimmer(id, value);
    _update(id, (d) => d.copyWith(dimmer: dimmer));
  }

  Future<void> setSetpoint(String id, double setpoint) async {
    if (_demoMode) {
      _update(id, (d) => d.copyWith(setpoint: setpoint));
      return;
    }
    final updated = await _api.setSetpoint(id, setpoint);
    _replace(updated);
  }

  Future<void> setDimmerLimits(String id, int dimmerMin, int dimmerMax) async {
    if (_demoMode) {
      _update(id, (d) => d.copyWith(dimmerMin: dimmerMin, dimmerMax: dimmerMax));
      return;
    }
    final updated = await _api.setDimmerLimits(id, dimmerMin, dimmerMax);
    _replace(updated);
  }

  Future<void> calibrateSensor(String id) async {
    if (_demoMode) return;
    await _api.calibrateSensor(id);
  }

  Future<void> renameDevice(String id, {required String name, required String room}) async {
    if (_demoMode) {
      _update(id, (d) => d.copyWith(name: name, room: room));
      return;
    }
    final updated = await _api.updateDevice(id, name: name, room: room);
    _replace(updated);
  }

  Future<void> removeDevice(String id) async {
    if (!_demoMode) {
      await _api.deleteDevice(id);
    }
    _devices = _devices.where((d) => d.id != id).toList(growable: false);
    notifyListeners();
  }

  Future<List<TelemetryPoint>> telemetryHistory(String id) {
    if (_demoMode) return Future.value(const []);
    return _api.getTelemetryHistory(id);
  }

  void _update(String id, Device Function(Device current) transform) {
    final index = _devices.indexWhere((d) => d.id == id);
    if (index == -1) return;
    final next = List<Device>.of(_devices);
    next[index] = transform(next[index]);
    _devices = next;
    notifyListeners();
  }

  void _replace(Device device) {
    final index = _devices.indexWhere((d) => d.id == device.id);
    final next = List<Device>.of(_devices);
    if (index == -1) {
      next.add(device);
    } else {
      next[index] = device;
    }
    _devices = next;
    notifyListeners();
  }
}
