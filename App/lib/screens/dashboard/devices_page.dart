import 'package:flutter/material.dart';

import '../../core/routes/app_routes.dart';
import '../../core/theme/app_theme.dart';
import '../../models/device.dart';
import '../../models/mock_devices.dart';
import '../../widgets/app_scaffold.dart';
import '../../widgets/device_card.dart';

class DevicesPage extends StatefulWidget {
  const DevicesPage({super.key});

  @override
  State<DevicesPage> createState() => _DevicesPageState();
}

class _DevicesPageState extends State<DevicesPage> {
  late List<Device> _devices;

  @override
  void initState() {
    super.initState();
    _devices = List.of(mockDevices);
  }

  void _openDevice(Device device) {
    Navigator.pushNamed(context, AppRoutes.control, arguments: device.id);
  }

  void _toggleRelay(Device device, bool value) {
    setState(() {
      final i = _devices.indexWhere((d) => d.id == device.id);
      _devices[i] = device.copyWith(relayOn: value);
    });
  }

  @override
  Widget build(BuildContext context) {
    final online = _devices.where((d) => d.online).length;
    final byRoom = <String, List<Device>>{};
    for (final d in _devices) {
      byRoom.putIfAbsent(d.room, () => []).add(d);
    }

    return Scaffold(
      backgroundColor: AppColors.paper,
      appBar: PreferredSize(
        preferredSize: const Size.fromHeight(64),
        child: SafeArea(
          child: Padding(
            padding: const EdgeInsets.fromLTRB(18, 8, 18, 6),
            child: Row(
              children: [
                const RoundIconButton(icon: Icons.menu_rounded),
                Expanded(
                  child: Column(
                    children: [
                      const Text('Meus interruptores', style: TextStyle(fontSize: 17, fontWeight: FontWeight.w700)),
                      Text('${_devices.length} dispositivos · $online online',
                          style: const TextStyle(fontSize: 11, color: AppColors.ink2, fontWeight: FontWeight.w500)),
                    ],
                  ),
                ),
                RoundIconButton(icon: Icons.add_rounded, color: AppColors.accentInk, onTap: () {}),
              ],
            ),
          ),
        ),
      ),
      body: ListView(
        padding: const EdgeInsets.fromLTRB(16, 6, 16, 12),
        children: [
          TextField(
            decoration: InputDecoration(
              hintText: 'Buscar dispositivo ou ambiente',
              prefixIcon: const Padding(
                padding: EdgeInsets.all(16),
                child: SizedBox(width: 6, height: 6, child: DecoratedBox(decoration: BoxDecoration(color: AppColors.ink3, shape: BoxShape.circle))),
              ),
              prefixIconConstraints: const BoxConstraints(minWidth: 36, minHeight: 36),
            ),
          ),
          for (final room in byRoom.keys) ...[
            const SizedBox(height: 18),
            SectionLabel(title: room, action: 'ligar todos', onAction: () {}),
            for (final device in byRoom[room]!)
              DeviceListTile(
                device: device,
                onTap: () => _openDevice(device),
                onToggle: (v) => _toggleRelay(device, v),
              ),
          ],
        ],
      ),
      bottomNavigationBar: SmartBottomNav(
        currentIndex: 1,
        onTap: (i) {
          if (i == 0) Navigator.pushReplacementNamed(context, AppRoutes.control, arguments: _devices.first.id);
        },
      ),
    );
  }
}
