import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../../core/routes/app_routes.dart';
import '../../core/theme/app_theme.dart';
import '../../models/device.dart';
import '../../providers/device_provider.dart';
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
  String? _deviceId;
  bool _ready = false;
  bool _busy = false;

  @override
  void didChangeDependencies() {
    super.didChangeDependencies();
    if (!_ready) {
      _deviceId = widget.deviceId ?? (ModalRoute.of(context)?.settings.arguments as String?);
      _ready = true;
    }
  }

  DeviceProvider get _provider => context.read<DeviceProvider>();

  void _notify(String message) {
    if (!mounted) return;
    ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text(message)));
  }

  Future<void> _run(Future<void> Function() action, {required String successMessage, required String failurePrefix}) async {
    if (_busy) return;
    setState(() => _busy = true);
    try {
      await action();
      _notify(successMessage);
    } catch (e) {
      _notify('$failurePrefix: $e');
    } finally {
      if (mounted) setState(() => _busy = false);
    }
  }

  Future<void> _editSetpoint(Device device) async {
    final controller = TextEditingController(text: device.setpoint.round().toString());
    final value = await showDialog<double>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('Setpoint de luminosidade'),
        content: TextField(
          controller: controller,
          autofocus: true,
          keyboardType: TextInputType.number,
          decoration: const InputDecoration(labelText: 'Lux desejado', suffixText: 'lux'),
        ),
        actions: [
          TextButton(onPressed: () => Navigator.pop(ctx), child: const Text('Cancelar')),
          TextButton(
            onPressed: () => Navigator.pop(ctx, double.tryParse(controller.text.trim().replaceAll(',', '.'))),
            child: const Text('Salvar'),
          ),
        ],
      ),
    );
    if (value == null || value <= 0) return;
    await _run(
      () => _provider.setSetpoint(device.id, value),
      successMessage: 'Setpoint atualizado para ${value.round()} lux',
      failurePrefix: 'Erro ao salvar o setpoint',
    );
  }

  Future<void> _editDimmerLimits(Device device) async {
    final minController = TextEditingController(text: device.dimmerMin.toString());
    final maxController = TextEditingController(text: device.dimmerMax.toString());
    final limits = await showDialog<(int, int)>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('Limites do dimmer'),
        content: Row(
          children: [
            Expanded(
              child: TextField(
                controller: minController,
                autofocus: true,
                keyboardType: TextInputType.number,
                decoration: const InputDecoration(labelText: 'Mínimo', suffixText: '%'),
              ),
            ),
            const SizedBox(width: 12),
            Expanded(
              child: TextField(
                controller: maxController,
                keyboardType: TextInputType.number,
                decoration: const InputDecoration(labelText: 'Máximo', suffixText: '%'),
              ),
            ),
          ],
        ),
        actions: [
          TextButton(onPressed: () => Navigator.pop(ctx), child: const Text('Cancelar')),
          TextButton(
            onPressed: () {
              final min = int.tryParse(minController.text.trim());
              final max = int.tryParse(maxController.text.trim());
              if (min == null || max == null) return Navigator.pop(ctx);
              Navigator.pop(ctx, (min, max));
            },
            child: const Text('Salvar'),
          ),
        ],
      ),
    );
    if (limits == null) return;
    final (min, max) = limits;
    if (min < 0 || max > 100 || min > max) {
      _notify('Limites inválidos: use valores entre 0 e 100, com mínimo ≤ máximo');
      return;
    }
    await _run(
      () => _provider.setDimmerLimits(device.id, min, max),
      successMessage: 'Limites do dimmer atualizados para $min–$max%',
      failurePrefix: 'Erro ao salvar os limites do dimmer',
    );
  }

  Future<void> _calibrate(Device device) async {
    await _run(
      () => _provider.calibrateSensor(device.id),
      successMessage: 'Calibração do sensor solicitada',
      failurePrefix: 'Erro ao calibrar o sensor',
    );
  }

  Future<void> _rename(Device device) async {
    final nameController = TextEditingController(text: device.name);
    final roomController = TextEditingController(text: device.room);
    final result = await showDialog<(String, String)>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('Renomear / ambiente'),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            TextField(controller: nameController, autofocus: true, decoration: const InputDecoration(labelText: 'Nome do dispositivo')),
            const SizedBox(height: 12),
            TextField(controller: roomController, decoration: const InputDecoration(labelText: 'Ambiente')),
          ],
        ),
        actions: [
          TextButton(onPressed: () => Navigator.pop(ctx), child: const Text('Cancelar')),
          TextButton(
            onPressed: () {
              final name = nameController.text.trim();
              final room = roomController.text.trim();
              if (name.isEmpty || room.isEmpty) return Navigator.pop(ctx);
              Navigator.pop(ctx, (name, room));
            },
            child: const Text('Salvar'),
          ),
        ],
      ),
    );
    if (result == null) return;
    final (name, room) = result;
    await _run(
      () => _provider.renameDevice(device.id, name: name, room: room),
      successMessage: 'Dispositivo atualizado',
      failurePrefix: 'Erro ao renomear o dispositivo',
    );
  }

  Future<void> _remove(Device device) async {
    final confirmed = await showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('Remover dispositivo'),
        content: Text('Tem certeza de que deseja remover "${device.name}"? Essa ação não pode ser desfeita.'),
        actions: [
          TextButton(onPressed: () => Navigator.pop(ctx, false), child: const Text('Cancelar')),
          TextButton(
            onPressed: () => Navigator.pop(ctx, true),
            child: const Text('Remover', style: TextStyle(color: AppColors.danger)),
          ),
        ],
      ),
    );
    if (confirmed != true) return;
    if (_busy) return;
    setState(() => _busy = true);
    try {
      await _provider.removeDevice(device.id);
      if (!mounted) return;
      Navigator.popUntil(context, ModalRoute.withName(AppRoutes.devices));
    } catch (e) {
      _notify('Erro ao remover o dispositivo: $e');
      if (mounted) setState(() => _busy = false);
    }
  }

  void _comingSoon(String feature) {
    _notify('$feature: em breve');
  }

  @override
  Widget build(BuildContext context) {
    final provider = context.watch<DeviceProvider>();
    final d = _deviceId != null ? provider.deviceById(_deviceId!) : null;

    if (d == null) {
      return Scaffold(
        backgroundColor: AppColors.paper,
        appBar: DeviceAppBar(
          title: 'Dispositivo',
          subtitle: '—',
          leading: RoundIconButton(icon: Icons.chevron_left_rounded, onTap: () => Navigator.maybePop(context)),
        ),
        body: const Center(child: Text('dispositivo não encontrado', style: TextStyle(color: AppColors.ink2, fontWeight: FontWeight.w500))),
      );
    }

    return Scaffold(
      backgroundColor: AppColors.paper,
      appBar: DeviceAppBar(
        title: d.name,
        subtitle: d.room,
        leading: RoundIconButton(icon: Icons.chevron_left_rounded, onTap: () => Navigator.maybePop(context)),
        trailing: const RoundIconButton(icon: Icons.more_horiz_rounded, color: AppColors.ink2),
      ),
      body: AbsorbPointer(
        absorbing: _busy,
        child: Opacity(
          opacity: _busy ? 0.6 : 1,
          child: ListView(
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
              SettingsRow(title: 'Setpoint de luminosidade', value: '${d.setpoint.round()} lux', onTap: () => _editSetpoint(d)),
              SettingsRow(title: 'Limites do dimmer', value: '${d.dimmerMin}–${d.dimmerMax}%', onTap: () => _editDimmerLimits(d)),
              const SizedBox(height: 14),
              SectionLabel(title: 'Dispositivo'),
              SettingsRow(title: 'Calibrar sensor de luz', onTap: () => _calibrate(d)),
              SettingsRow(title: 'Renomear / ambiente', value: '${d.name} · ${d.room}', onTap: () => _rename(d)),
              SettingsRow(title: 'Agendamentos', value: '${d.activeSchedules} ativos', onTap: () => _comingSoon('Agendamentos')),
              SettingsRow(title: 'Wi-Fi e conexão', value: d.wifiName, onTap: () => _comingSoon('Wi-Fi e conexão')),
              const SizedBox(height: 14),
              SettingsRow(title: 'Remover dispositivo', showChevron: false, danger: true, onTap: () => _remove(d)),
            ],
          ),
        ),
      ),
    );
  }
}
