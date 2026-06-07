import 'package:flutter/material.dart';

import '../../core/theme/app_theme.dart';
import '../../models/device.dart';
import '../../models/mock_devices.dart';
import '../../widgets/app_scaffold.dart';
import '../../widgets/lux_gauge.dart';
import '../../widgets/status_chip.dart';

class DeviceSettingsPage extends StatefulWidget {
  final String? deviceId;
  const DeviceSettingsPage({super.key, this.deviceId});

  @override
  State<DeviceSettingsPage> createState() => _DeviceSettingsPageState();
}

class _DeviceSettingsPageState extends State<DeviceSettingsPage> {
  late Device _device;
  bool _ready = false;

  @override
  void didChangeDependencies() {
    super.didChangeDependencies();
    if (!_ready) {
      final id = widget.deviceId ?? (ModalRoute.of(context)?.settings.arguments as String?);
      _device = id != null ? deviceById(id) : mockDevices.first;
      _ready = true;
    }
  }

  @override
  Widget build(BuildContext context) {
    final d = _device;
    return Scaffold(
      backgroundColor: AppColors.paper,
      appBar: DeviceAppBar(
        title: d.name,
        subtitle: d.room,
        leading: RoundIconButton(icon: Icons.chevron_left_rounded, onTap: () => Navigator.maybePop(context)),
        trailing: const RoundIconButton(icon: Icons.more_horiz_rounded, color: AppColors.ink2),
      ),
      body: ListView(
        padding: const EdgeInsets.fromLTRB(16, 4, 16, 24),
        children: [
          Container(
            padding: const EdgeInsets.all(15),
            decoration: BoxDecoration(
              color: AppColors.surface,
              borderRadius: BorderRadius.circular(18),
              border: Border.all(color: AppColors.hair),
            ),
            child: Row(
              children: [
                MiniLuxGauge(percent: d.online ? d.dimmer.toDouble() : 0, big: d.online ? d.lux.round().toString() : '—'),
                const SizedBox(width: 13),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      StatusChip.connection(online: d.online, subtitle: d.online ? null : 'visto ${d.lastSeen}'),
                      const SizedBox(height: 8),
                      Text('firmware ${d.firmware} · ${d.rssi} dBm',
                          style: const TextStyle(fontSize: 11, color: AppColors.ink2, fontWeight: FontWeight.w500)),
                    ],
                  ),
                ),
              ],
            ),
          ),
          const SizedBox(height: 16),
          SectionLabel(title: 'Luminosidade'),
          SettingsRow(title: 'Setpoint de luminosidade', value: '${d.setpoint.round()} lux', onTap: () {}),
          SettingsRow(title: 'Limites do dimmer', value: '${d.dimmerMin}–${d.dimmerMax}%', onTap: () {}),
          const SizedBox(height: 14),
          SectionLabel(title: 'Dispositivo'),
          SettingsRow(title: 'Calibrar sensor de luz', onTap: () {}),
          SettingsRow(title: 'Renomear / ambiente', value: '${d.name} · ${d.room}', onTap: () {}),
          SettingsRow(title: 'Agendamentos', value: '${d.activeSchedules} ativos', onTap: () {}),
          SettingsRow(title: 'Wi-Fi e conexão', value: d.wifiName, onTap: () {}),
          const SizedBox(height: 14),
          SettingsRow(title: 'Remover dispositivo', showChevron: false, danger: true, onTap: () {}),
        ],
      ),
    );
  }
}
