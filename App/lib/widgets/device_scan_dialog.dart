import 'dart:async';

import 'package:flutter/material.dart';

import '../core/theme/app_theme.dart';

enum _ScanState { scanning, done }

/// Popup shown for scanning nearby devices to register.
///
/// Placeholder for now — the scan is simulated with a timer. Will be wired
/// up to a real BLE scan (mirroring `Firmware/communication/ble_setup`) once
/// device provisioning is implemented.
class DeviceScanDialog extends StatefulWidget {
  const DeviceScanDialog({super.key});

  static Future<void> show(BuildContext context) {
    return showDialog(context: context, builder: (_) => const DeviceScanDialog());
  }

  @override
  State<DeviceScanDialog> createState() => _DeviceScanDialogState();
}

class _DeviceScanDialogState extends State<DeviceScanDialog> {
  _ScanState _state = _ScanState.scanning;
  Timer? _timer;

  @override
  void initState() {
    super.initState();
    _startScan();
  }

  void _startScan() {
    setState(() => _state = _ScanState.scanning);
    _timer?.cancel();
    _timer = Timer(const Duration(seconds: 2), () {
      if (mounted) setState(() => _state = _ScanState.done);
    });
  }

  @override
  void dispose() {
    _timer?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return AlertDialog(
      title: const Text('Procurar dispositivos'),
      content: SizedBox(
        width: 280,
        child: _state == _ScanState.scanning ? _buildScanning() : _buildEmptyResult(),
      ),
      actions: [
        TextButton(onPressed: () => Navigator.pop(context), child: const Text('Fechar')),
        if (_state == _ScanState.done)
          TextButton(onPressed: _startScan, child: const Text('Buscar novamente')),
      ],
    );
  }

  Widget _buildScanning() {
    return const Padding(
      padding: EdgeInsets.symmetric(vertical: 12),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          CircularProgressIndicator(),
          SizedBox(height: 16),
          Text(
            'Procurando dispositivos próximos via Bluetooth...',
            textAlign: TextAlign.center,
            style: TextStyle(color: AppColors.ink2, fontWeight: FontWeight.w500),
          ),
        ],
      ),
    );
  }

  Widget _buildEmptyResult() {
    return const Padding(
      padding: EdgeInsets.symmetric(vertical: 12),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Icon(Icons.bluetooth_disabled_rounded, size: 36, color: AppColors.ink3),
          SizedBox(height: 12),
          Text('Nenhum dispositivo encontrado', style: TextStyle(fontWeight: FontWeight.w600)),
          SizedBox(height: 6),
          Text(
            'A busca via Bluetooth (BLE) ainda será implementada.',
            textAlign: TextAlign.center,
            style: TextStyle(color: AppColors.ink2, fontSize: 12, fontWeight: FontWeight.w500),
          ),
        ],
      ),
    );
  }
}
