import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../../core/routes/app_routes.dart';
import '../../core/theme/app_theme.dart';
import '../../models/device.dart';
import '../../providers/device_provider.dart';
import '../../widgets/app_scaffold.dart';
import '../../widgets/control_widgets.dart';
import '../../widgets/dimmer_slider.dart';
import '../../widgets/lux_gauge.dart';
import '../../widgets/status_chip.dart';

class ControlPage extends StatefulWidget {
  final String? deviceId;
  const ControlPage({super.key, this.deviceId});

  @override
  State<ControlPage> createState() => _ControlPageState();
}

class _ControlPageState extends State<ControlPage> {
  String? _deviceId;
  bool _ready = false;

  @override
  void didChangeDependencies() {
    super.didChangeDependencies();
    if (!_ready) {
      _deviceId = widget.deviceId ?? (ModalRoute.of(context)?.settings.arguments as String?);
      _ready = true;
    }
  }

  Future<void> _setMode(Device device, int index) async {
    try {
      await context.read<DeviceProvider>().setMode(device.id, index == 1);
    } catch (e) {
      _showError('Erro ao mudar o modo: $e');
    }
  }

  Future<void> _setDimmer(Device device, double value) async {
    try {
      await context.read<DeviceProvider>().setDimmer(device.id, value.round());
    } catch (e) {
      _showError('Erro ao ajustar o dimmer: $e');
    }
  }

  Future<void> _reconnect(Device device) async {
    try {
      await context.read<DeviceProvider>().refreshDevice(device.id);
    } catch (e) {
      _showError('Ainda não foi possível falar com o dispositivo: $e');
    }
  }

  void _showError(String message) {
    if (!mounted) return;
    ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text(message)));
  }

  @override
  Widget build(BuildContext context) {
    final provider = context.watch<DeviceProvider>();
    final device = _deviceId != null ? provider.deviceById(_deviceId!) : null;

    if (device == null) {
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
        title: device.name,
        subtitle: device.room,
        leading: RoundIconButton(icon: Icons.chevron_left_rounded, onTap: () => Navigator.maybePop(context)),
        trailing: RoundIconButton(
          icon: Icons.more_horiz_rounded,
          color: AppColors.ink2,
          onTap: () => Navigator.pushNamed(context, AppRoutes.settings, arguments: device.id),
        ),
      ),
      body: device.online
          ? _OnlineBody(device: device, onModeChanged: (i) => _setMode(device, i), onDimmerChanged: (v) => _setDimmer(device, v))
          : _OfflineBody(device: device, onReconnect: () => _reconnect(device)),
      bottomNavigationBar: SmartBottomNav(
        currentIndex: 0,
        onTap: (i) {
          if (i == 1) Navigator.pushReplacementNamed(context, AppRoutes.devices);
        },
      ),
    );
  }
}

class _OnlineBody extends StatelessWidget {
  final Device device;
  final ValueChanged<int> onModeChanged;
  final ValueChanged<double> onDimmerChanged;

  const _OnlineBody({required this.device, required this.onModeChanged, required this.onDimmerChanged});

  @override
  Widget build(BuildContext context) {
    final auto = device.automaticMode;
    final coverage = device.setpoint > 0 ? (device.naturalLux / device.setpoint * 100).clamp(0, 100).round() : 0;
    final atTarget = auto && (device.lux - device.setpoint).abs() < 1;

    return ListView(
      padding: const EdgeInsets.fromLTRB(16, 4, 16, 20),
      children: [
        Center(child: StatusChip.connection(online: true)),
        const SizedBox(height: 14),
        ModeSegment(options: const ['Manual', 'Automático'], activeIndex: auto ? 1 : 0, onChanged: onModeChanged),
        const SizedBox(height: 16),
        Center(
          child: Column(
            children: [
              LuxGauge(
                size: 184,
                percent: device.setpoint > 0 ? (device.lux / device.setpoint * 100) : 0,
                big: device.lux.round().toString(),
                unit: auto ? 'lux' : 'lux medido',
                caption: atTarget ? '✓ no alvo · auto' : 'setpoint · ${device.setpoint.round()} lux',
              ),
              const SizedBox(height: 6),
              if (auto)
                Padding(
                  padding: const EdgeInsets.symmetric(horizontal: 12),
                  child: Text(
                    'o sensor mantém ${device.setpoint.round()} lux ajustando\na lâmpada conforme a luz natural muda',
                    textAlign: TextAlign.center,
                    style: const TextStyle(fontSize: 12.5, color: AppColors.accentInk, fontWeight: FontWeight.w500, height: 1.4),
                  ),
                )
              else
                Text.rich(
                  TextSpan(
                    style: const TextStyle(fontSize: 12.5, color: AppColors.ink2, fontWeight: FontWeight.w500),
                    children: [
                      const TextSpan(text: 'luz natural cobrindo '),
                      TextSpan(text: '$coverage%', style: const TextStyle(color: AppColors.coolInk, fontWeight: FontWeight.w700)),
                      const TextSpan(text: ' do alvo'),
                    ],
                  ),
                ),
            ],
          ),
        ),
        const SizedBox(height: 16),
        DimmerSlider(
          percent: device.dimmer.toDouble(),
          label: auto ? 'Dimmer (automático)' : 'Dimmer da lâmpada',
          muted: auto,
          onChanged: auto ? null : onDimmerChanged,
        ),
        if (auto)
          const Padding(
            padding: EdgeInsets.only(top: 8),
            child: Text(
              'controlado pelo sistema · toque em Manual p/ ajustar',
              textAlign: TextAlign.center,
              style: TextStyle(fontSize: 10, color: AppColors.ink2, fontWeight: FontWeight.w500),
            ),
          ),
        const SizedBox(height: 16),
        Row(
          children: [
            StatTile(value: device.naturalLux.round().toString(), unit: 'lux', label: 'luz natural', tone: ChipToneAccent.cool),
            const SizedBox(width: 8),
            StatTile(value: '${device.dimmer}', unit: '%', label: 'dimmer', tone: ChipToneAccent.accent),
            const SizedBox(width: 8),
            StatTile(value: device.powerW.toStringAsFixed(1), unit: 'W', label: 'consumo'),
          ],
        ),
        const SizedBox(height: 16),
        const DayChart(),
      ],
    );
  }
}

class _OfflineBody extends StatelessWidget {
  final Device device;
  final VoidCallback onReconnect;

  const _OfflineBody({required this.device, required this.onReconnect});

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 16),
      child: Column(
        children: [
          const SizedBox(height: 8),
          Center(child: StatusChip.connection(online: false, subtitle: 'visto ${device.lastSeen}')),
          Expanded(
            child: Center(
              child: Column(
                mainAxisSize: MainAxisSize.min,
                children: [
                  const LuxGauge(size: 170, percent: 0, big: '—', unit: 'sem leitura', caption: 'dispositivo desconectado'),
                  const SizedBox(height: 18),
                  const Text(
                    'não conseguimos falar com este interruptor.\nverifique a energia e o Wi-Fi.',
                    textAlign: TextAlign.center,
                    style: TextStyle(fontSize: 13.5, color: AppColors.ink2, fontWeight: FontWeight.w500, height: 1.45),
                  ),
                  const SizedBox(height: 18),
                  SizedBox(
                    width: 220,
                    child: ElevatedButton(onPressed: onReconnect, child: const Text('Tentar reconectar')),
                  ),
                  const SizedBox(height: 10),
                  TextButton(
                    onPressed: () {},
                    child: const Text('Ajuda de conexão', style: TextStyle(color: AppColors.ink, fontWeight: FontWeight.w700)),
                  ),
                ],
              ),
            ),
          ),
        ],
      ),
    );
  }
}
