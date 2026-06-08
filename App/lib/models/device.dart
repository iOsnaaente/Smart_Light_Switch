/// Converts the ISO-8601 timestamp the backend sends for `last_seen` (or
/// `null`, for devices never seen online) into the short relative label the
/// wireframe screens expect to display directly (e.g. "agora", "há 2h").
String _relativeLastSeen(dynamic isoTimestamp) {
  if (isoTimestamp is! String) return 'nunca';
  final seen = DateTime.tryParse(isoTimestamp);
  if (seen == null) return 'nunca';

  final diff = DateTime.now().toUtc().difference(seen.toUtc());
  if (diff.inSeconds < 45) return 'agora';
  if (diff.inMinutes < 60) return 'há ${diff.inMinutes} min';
  if (diff.inHours < 24) return 'há ${diff.inHours}h';
  return 'há ${diff.inDays}d';
}

double _asDouble(dynamic value, [double fallback = 0]) => (value as num?)?.toDouble() ?? fallback;

class Device {
  final String id;
  final String name;
  final String room;

  final bool online;
  final bool automaticMode;
  final bool relayOn;

  /// Total lux measured by the sensor.
  final double lux;
  /// Lux contributed by natural (daylight) light.
  final double naturalLux;
  /// Target lux configured by the user.
  final double setpoint;

  /// Lamp dimmer level, 0-100.
  final int dimmer;
  /// Current power draw in watts.
  final double powerW;
  /// Energy consumed today, in kWh.
  final double kwhToday;

  final String firmware;
  final int rssi;
  final String wifiName;
  final int activeSchedules;
  final String lastSeen;
  final int dimmerMin;
  final int dimmerMax;

  Device({
    required this.id,
    required this.name,
    required this.room,
    required this.online,
    required this.automaticMode,
    this.relayOn = true,
    required this.lux,
    this.naturalLux = 0,
    required this.setpoint,
    required this.dimmer,
    this.powerW = 0,
    this.kwhToday = 0,
    this.firmware = 'v2.1',
    this.rssi = -52,
    this.wifiName = 'CasaWiFi',
    this.activeSchedules = 0,
    this.lastSeen = 'agora',
    this.dimmerMin = 10,
    this.dimmerMax = 100,
  });

  /// Builds a [Device] from either a `DeviceSummary` (device list) or a
  /// `DeviceDetail` (single device) backend payload — the former omits most
  /// of the detail-only fields, so they fall back to sensible defaults until
  /// the detail endpoint is fetched.
  factory Device.fromJson(Map<String, dynamic> json) {
    final lux = _asDouble(json['lux']);
    return Device(
      id: json['id'] as String,
      name: json['name'] as String,
      room: json['room'] as String,
      online: json['online'] as bool? ?? false,
      automaticMode: json['automatic_mode'] as bool? ?? false,
      relayOn: json['relay_on'] as bool? ?? false,
      lux: lux,
      naturalLux: _asDouble(json['natural_lux']),
      setpoint: _asDouble(json['setpoint'], lux),
      dimmer: json['dimmer'] as int? ?? 0,
      powerW: _asDouble(json['power_w']),
      kwhToday: _asDouble(json['kwh_today']),
      firmware: json['firmware'] as String? ?? '—',
      rssi: json['rssi'] as int? ?? 0,
      wifiName: json['wifi_name'] as String? ?? '—',
      activeSchedules: json['active_schedules'] as int? ?? 0,
      lastSeen: _relativeLastSeen(json['last_seen']),
      dimmerMin: json['dimmer_min'] as int? ?? 0,
      dimmerMax: json['dimmer_max'] as int? ?? 100,
    );
  }

  Device copyWith({
    String? name,
    String? room,
    bool? online,
    bool? automaticMode,
    bool? relayOn,
    double? lux,
    double? naturalLux,
    double? setpoint,
    int? dimmer,
    double? powerW,
    double? kwhToday,
    String? firmware,
    int? rssi,
    String? wifiName,
    int? activeSchedules,
    String? lastSeen,
    int? dimmerMin,
    int? dimmerMax,
  }) {
    return Device(
      id: id,
      name: name ?? this.name,
      room: room ?? this.room,
      online: online ?? this.online,
      automaticMode: automaticMode ?? this.automaticMode,
      relayOn: relayOn ?? this.relayOn,
      lux: lux ?? this.lux,
      naturalLux: naturalLux ?? this.naturalLux,
      setpoint: setpoint ?? this.setpoint,
      dimmer: dimmer ?? this.dimmer,
      powerW: powerW ?? this.powerW,
      kwhToday: kwhToday ?? this.kwhToday,
      firmware: firmware ?? this.firmware,
      rssi: rssi ?? this.rssi,
      wifiName: wifiName ?? this.wifiName,
      activeSchedules: activeSchedules ?? this.activeSchedules,
      lastSeen: lastSeen ?? this.lastSeen,
      dimmerMin: dimmerMin ?? this.dimmerMin,
      dimmerMax: dimmerMax ?? this.dimmerMax,
    );
  }
}
