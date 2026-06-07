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
  final DateTime timestamp;

  Telemetry({
    required this.deviceId,
    this.sp,
    this.mv,
    this.current,
    this.power,
    required this.timestamp,
  });

  factory Telemetry.fromJson(Map<String, dynamic> json) {
    return Telemetry(
      deviceId: json['device_id'] as String,
      sp: (json['sp'] as num?)?.toDouble(),
      mv: (json['mv'] as num?)?.toDouble(),
      current: (json['current'] as num?)?.toDouble(),
      power: (json['power'] as num?)?.toDouble(),
      timestamp: DateTime.parse(json['timestamp'] as String),
    );
  }
}
