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

  Device copyWith({
    bool? online,
    bool? automaticMode,
    bool? relayOn,
    double? lux,
    double? naturalLux,
    double? setpoint,
    int? dimmer,
    double? powerW,
    double? kwhToday,
  }) {
    return Device(
      id: id,
      name: name,
      room: room,
      online: online ?? this.online,
      automaticMode: automaticMode ?? this.automaticMode,
      relayOn: relayOn ?? this.relayOn,
      lux: lux ?? this.lux,
      naturalLux: naturalLux ?? this.naturalLux,
      setpoint: setpoint ?? this.setpoint,
      dimmer: dimmer ?? this.dimmer,
      powerW: powerW ?? this.powerW,
      kwhToday: kwhToday ?? this.kwhToday,
      firmware: firmware,
      rssi: rssi,
      wifiName: wifiName,
      activeSchedules: activeSchedules,
      lastSeen: lastSeen,
      dimmerMin: dimmerMin,
      dimmerMax: dimmerMax,
    );
  }
}
