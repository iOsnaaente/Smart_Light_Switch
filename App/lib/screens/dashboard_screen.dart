import 'package:flutter/material.dart';

import '../api/api_client.dart';
import '../models/device_models.dart';
import 'login_screen.dart';

const String _deviceId = 'switch01';

class DashboardScreen extends StatefulWidget {
  final ApiClient apiClient;

  const DashboardScreen({super.key, required this.apiClient});

  @override
  State<DashboardScreen> createState() => _DashboardScreenState();
}

class _DashboardScreenState extends State<DashboardScreen> {
  DeviceState? _state;
  Telemetry? _telemetry;
  bool _loading = true;
  bool _togglingRelay = false;
  String? _error;

  @override
  void initState() {
    super.initState();
    _refresh();
  }

  Future<void> _refresh() async {
    setState(() {
      _loading = true;
      _error = null;
    });
    try {
      final results = await Future.wait([
        widget.apiClient.getState(_deviceId),
        widget.apiClient.getLatestTelemetry(_deviceId),
      ]);
      if (!mounted) return;
      setState(() {
        _state = results[0] as DeviceState;
        _telemetry = results[1] as Telemetry;
      });
    } catch (e) {
      if (!mounted) return;
      setState(() => _error = e.toString());
    } finally {
      if (mounted) setState(() => _loading = false);
    }
  }

  Future<void> _toggleRelay(bool value) async {
    setState(() => _togglingRelay = true);
    try {
      final relay = await widget.apiClient.setRelay(_deviceId, value);
      if (!mounted) return;
      setState(() {
        if (_state != null) {
          _state = DeviceState(
            deviceId: _state!.deviceId,
            relay: relay,
            updatedAt: DateTime.now(),
          );
        }
      });
    } catch (e) {
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Erro ao alterar relé: $e')),
      );
    } finally {
      if (mounted) setState(() => _togglingRelay = false);
    }
  }

  void _logout() {
    widget.apiClient.username = null;
    widget.apiClient.password = null;
    Navigator.of(context).pushReplacement(
      MaterialPageRoute(builder: (_) => LoginScreen(apiClient: widget.apiClient)),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Smart Light Switch'),
        actions: [
          IconButton(
            onPressed: _logout,
            icon: const Icon(Icons.logout),
            tooltip: 'Sair',
          ),
        ],
      ),
      body: RefreshIndicator(
        onRefresh: _refresh,
        child: _buildBody(),
      ),
    );
  }

  Widget _buildBody() {
    if (_loading && _state == null) {
      return const Center(child: CircularProgressIndicator());
    }

    if (_error != null && _state == null) {
      return ListView(
        children: [
          const SizedBox(height: 80),
          Icon(Icons.cloud_off, size: 48, color: Theme.of(context).colorScheme.error),
          const SizedBox(height: 16),
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 24),
            child: Text(
              _error!,
              textAlign: TextAlign.center,
              style: TextStyle(color: Theme.of(context).colorScheme.error),
            ),
          ),
          const SizedBox(height: 16),
          Center(
            child: OutlinedButton(onPressed: _refresh, child: const Text('Tentar novamente')),
          ),
        ],
      );
    }

    final state = _state!;
    final telemetry = _telemetry;

    return ListView(
      padding: const EdgeInsets.all(16),
      children: [
        Card(
          child: Padding(
            padding: const EdgeInsets.all(16),
            child: Row(
              children: [
                Icon(
                  state.relay ? Icons.lightbulb : Icons.lightbulb_outline,
                  size: 40,
                  color: state.relay ? Colors.amber : null,
                ),
                const SizedBox(width: 16),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        _deviceId,
                        style: Theme.of(context).textTheme.titleMedium,
                      ),
                      Text(state.relay ? 'Ligado' : 'Desligado'),
                    ],
                  ),
                ),
                _togglingRelay
                    ? const SizedBox(
                        width: 24,
                        height: 24,
                        child: CircularProgressIndicator(strokeWidth: 2),
                      )
                    : Switch(
                        value: state.relay,
                        onChanged: _toggleRelay,
                      ),
              ],
            ),
          ),
        ),
        const SizedBox(height: 16),
        if (telemetry != null) _buildTelemetryCard(telemetry),
        const SizedBox(height: 16),
        Center(
          child: TextButton.icon(
            onPressed: _loading ? null : _refresh,
            icon: const Icon(Icons.refresh),
            label: const Text('Atualizar'),
          ),
        ),
      ],
    );
  }

  Widget _buildTelemetryCard(Telemetry telemetry) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text('Telemetria', style: Theme.of(context).textTheme.titleMedium),
            const SizedBox(height: 12),
            _telemetryRow('Tensão de pico (sp)', telemetry.sp, 'V'),
            _telemetryRow('Tensão RMS (mv)', telemetry.mv, 'V'),
            _telemetryRow('Corrente', telemetry.current, 'A'),
            _telemetryRow('Potência', telemetry.power, 'W'),
            const SizedBox(height: 8),
            Text(
              'Atualizado em ${telemetry.timestamp.toLocal()}',
              style: Theme.of(context).textTheme.bodySmall,
            ),
          ],
        ),
      ),
    );
  }

  Widget _telemetryRow(String label, double? value, String unit) {
    final text = value == null ? '--' : '${value.toStringAsFixed(1)} $unit';
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 4),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(label),
          Text(text, style: const TextStyle(fontWeight: FontWeight.w600)),
        ],
      ),
    );
  }
}
