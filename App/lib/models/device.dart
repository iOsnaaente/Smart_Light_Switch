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
