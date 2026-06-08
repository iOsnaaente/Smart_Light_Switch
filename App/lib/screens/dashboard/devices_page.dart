import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../../core/routes/app_routes.dart';
import '../../core/theme/app_theme.dart';
import '../../models/device.dart';
import '../../providers/device_provider.dart';
import '../../widgets/app_scaffold.dart';
import '../../widgets/device_card.dart';

class DevicesPage extends StatefulWidget {
  const DevicesPage({super.key});

  @override
  State<DevicesPage> createState() => _DevicesPageState();
}

class _DevicesPageState extends State<DevicesPage> {
  String _query = '';

  @override
  void initState() {
    super.initState();
    final provider = context.read<DeviceProvider>();
    if (!provider.demoMode && provider.devices.isEmpty) {
      WidgetsBinding.instance.addPostFrameCallback((_) => provider.loadDevices());
    }
  }

  void _openDevice(Device device) {
    Navigator.pushNamed(context, AppRoutes.control, arguments: device.id);
  }

  Future<void> _toggleRelay(Device device, bool value) async {
    try {
      await context.read<DeviceProvider>().setRelay(device.id, value);
    } catch (e) {
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text('Erro ao alterar relé: $e')));
    }
  }

  @override
  Widget build(BuildContext context) {
    final provider = context.watch<DeviceProvider>();
    final devices = _query.trim().isEmpty
        ? provider.devices
        : provider.devices.where((d) {
            final q = _query.trim().toLowerCase();
            return d.name.toLowerCase().contains(q) || d.room.toLowerCase().contains(q);
          }).toList();

    final online = provider.devices.where((d) => d.online).length;
    final byRoom = <String, List<Device>>{};
    for (final d in devices) {
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
                      Text('${provider.devices.length} dispositivos · $online online',
                          style: const TextStyle(fontSize: 11, color: AppColors.ink2, fontWeight: FontWeight.w500)),
                    ],
                  ),
                ),
                RoundIconButton(
                  icon: Icons.refresh_rounded,
                  color: AppColors.accentInk,
                  onTap: provider.demoMode ? null : () => provider.loadDevices(),
                ),
              ],
            ),
          ),
        ),
      ),
      body: _buildBody(provider, devices, byRoom),
      bottomNavigationBar: SmartBottomNav(
        currentIndex: 1,
        onTap: (i) {
          if (i == 0 && provider.devices.isNotEmpty) {
            Navigator.pushReplacementNamed(context, AppRoutes.control, arguments: provider.devices.first.id);
          }
        },
      ),
    );
  }

  Widget _buildBody(DeviceProvider provider, List<Device> devices, Map<String, List<Device>> byRoom) {
    if (provider.loading && provider.devices.isEmpty) {
      return const Center(child: CircularProgressIndicator());
    }

    if (provider.error != null && provider.devices.isEmpty) {
      return ListView(
        padding: const EdgeInsets.fromLTRB(24, 80, 24, 24),
        children: [
          const Icon(Icons.cloud_off_rounded, size: 44, color: AppColors.ink3),
          const SizedBox(height: 14),
          Text(provider.error!, textAlign: TextAlign.center, style: const TextStyle(color: AppColors.ink2, fontWeight: FontWeight.w500)),
          const SizedBox(height: 16),
          Center(child: OutlinedButton(onPressed: provider.loadDevices, child: const Text('Tentar novamente'))),
        ],
      );
    }

    return RefreshIndicator(
      onRefresh: provider.demoMode ? () async {} : provider.loadDevices,
      child: ListView(
        padding: const EdgeInsets.fromLTRB(16, 6, 16, 12),
        children: [
          TextField(
            onChanged: (value) => setState(() => _query = value),
            decoration: InputDecoration(
              hintText: 'Buscar dispositivo ou ambiente',
              prefixIcon: const Padding(
                padding: EdgeInsets.all(16),
                child: SizedBox(width: 6, height: 6, child: DecoratedBox(decoration: BoxDecoration(color: AppColors.ink3, shape: BoxShape.circle))),
              ),
              prefixIconConstraints: const BoxConstraints(minWidth: 36, minHeight: 36),
            ),
          ),
          if (devices.isEmpty)
            Padding(
              padding: const EdgeInsets.only(top: 40),
              child: Center(
                child: Text(
                  provider.devices.isEmpty ? 'nenhum dispositivo cadastrado ainda' : 'nada encontrado para "$_query"',
                  style: const TextStyle(color: AppColors.ink2, fontWeight: FontWeight.w500),
                ),
              ),
            ),
          for (final room in byRoom.keys) ...[
            const SizedBox(height: 18),
            SectionLabel(title: room, action: 'ligar todos', onAction: () => _toggleAllInRoom(byRoom[room]!, true)),
            for (final device in byRoom[room]!)
              DeviceListTile(
                device: device,
                onTap: () => _openDevice(device),
                onToggle: (v) => _toggleRelay(device, v),
              ),
          ],
        ],
      ),
    );
  }

  void _toggleAllInRoom(List<Device> devices, bool value) {
    for (final device in devices) {
      if (device.online) _toggleRelay(device, value);
    }
  }
}
