import 'device.dart';

/// In-memory sample data used to drive the UI before the real API is wired
/// in — mirrors the devices shown across the wireframes.
final List<Device> mockDevices = [
  Device(
    id: 'mesa-trabalho',
    name: 'Mesa de trabalho',
    room: 'Sala de estar',
    online: true,
    automaticMode: false,
    lux: 320,
    naturalLux: 180,
    setpoint: 400,
    dimmer: 62,
    powerW: 8.4,
    kwhToday: 0.21,
    activeSchedules: 2,
  ),
  Device(
    id: 'luz-leitura',
    name: 'Luz de leitura',
    room: 'Sala de estar',
    online: true,
    automaticMode: true,
    lux: 410,
    naturalLux: 220,
    setpoint: 410,
    dimmer: 48,
    powerW: 6.2,
    kwhToday: 0.16,
    activeSchedules: 1,
  ),
  Device(
    id: 'teto-quarto',
    name: 'Teto',
    room: 'Quarto',
    online: true,
    automaticMode: true,
    lux: 280,
    naturalLux: 150,
    setpoint: 300,
    dimmer: 70,
    powerW: 7.1,
    kwhToday: 0.18,
    activeSchedules: 3,
  ),
  Device(
    id: 'closet',
    name: 'Closet',
    room: 'Quarto',
    online: false,
    automaticMode: false,
    relayOn: false,
    lux: 0,
    naturalLux: 0,
    setpoint: 250,
    dimmer: 0,
    lastSeen: 'há 2h',
  ),
];

Device deviceById(String id) => mockDevices.firstWhere((d) => d.id == id, orElse: () => mockDevices.first);
