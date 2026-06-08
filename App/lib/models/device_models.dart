class DeviceState {
  final String deviceId;
  final bool relay;
  final DateTime updatedAt;

  DeviceState({required this.deviceId, required this.relay, required this.updatedAt});

  factory DeviceState.fromJson(Map<String, dynamic> json) {
    return DeviceState(
      deviceId: json['device_id'] as String,
      relay: json['relay'] as bool,
      updatedAt: DateTime.parse(json['updated_at'] as String),
    );
  }
}

class Telemetry {
  final String deviceId;
  final double? sp;
  final double? mv;
  final double? current;
  final double? power;
  final double? lux;
  final double? naturalLux;
  final double? kwhToday;
  final DateTime timestamp;

  Telemetry({
    required this.deviceId,
    this.sp,
    this.mv,
    this.current,
    this.power,
    this.lux,
    this.naturalLux,
    this.kwhToday,
    required this.timestamp,
  });

  factory Telemetry.fromJson(Map<String, dynamic> json) {
    return Telemetry(
      deviceId: json['device_id'] as String,
      sp: (json['sp'] as num?)?.toDouble(),
      mv: (json['mv'] as num?)?.toDouble(),
      current: (json['current'] as num?)?.toDouble(),
      power: (json['power'] as num?)?.toDouble(),
      lux: (json['lux'] as num?)?.toDouble(),
      naturalLux: (json['natural_lux'] as num?)?.toDouble(),
      kwhToday: (json['kwh_today'] as num?)?.toDouble(),
      timestamp: DateTime.parse(json['timestamp'] as String),
    );
  }
}

/// One sample of the device's telemetry history (GET .../telemetry/history).
class TelemetryPoint {
  final DateTime timestamp;
  final double? lux;
  final double? naturalLux;
  final int? dimmer;

  TelemetryPoint({required this.timestamp, this.lux, this.naturalLux, this.dimmer});

  factory TelemetryPoint.fromJson(Map<String, dynamic> json) {
    return TelemetryPoint(
      timestamp: DateTime.parse(json['timestamp'] as String),
      lux: (json['lux'] as num?)?.toDouble(),
      naturalLux: (json['natural_lux'] as num?)?.toDouble(),
      dimmer: json['dimmer'] as int?,
    );
  }
}

/// A recurring on/off schedule attached to a device.
class Schedule {
  final int id;
  final String label;
  final String time;
  final List<String> daysOfWeek;
  final String action;
  final bool enabled;

  Schedule({
    required this.id,
    required this.label,
    required this.time,
    required this.daysOfWeek,
    required this.action,
    required this.enabled,
  });

  factory Schedule.fromJson(Map<String, dynamic> json) {
    return Schedule(
      id: json['id'] as int,
      label: json['label'] as String,
      time: json['time'] as String,
      daysOfWeek: (json['days_of_week'] as List<dynamic>).map((e) => e as String).toList(),
      action: json['action'] as String,
      enabled: json['enabled'] as bool,
    );
  }
}
