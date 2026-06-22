import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:provider/provider.dart';

import '../core/theme/app_theme.dart';
import '../providers/auth_provider.dart';
import '../services/ble_provisioning_service.dart';

enum _Step { scanning, credentials, provisioning, success, failure, unsupported }

/// Popup for finding nearby SmartLight devices over BLE and provisioning
/// them with Wi-Fi credentials. See `lib/services/ble_provisioning_service.dart`
/// for the protocol details (kept in sync with `Firmware/communication/ble_setup.cpp`).
class DeviceScanDialog extends StatefulWidget {
  const DeviceScanDialog({super.key});

  /// Returns `true` if a device was successfully provisioned, so the caller
  /// can refresh the device list.
  static Future<bool> show(BuildContext context) {
    return showDialog<bool>(context: context, builder: (_) => const DeviceScanDialog())
        .then((value) => value ?? false);
  }

  @override
  State<DeviceScanDialog> createState() => _DeviceScanDialogState();
}

class _DeviceScanDialogState extends State<DeviceScanDialog> {
  _Step _step = _Step.scanning;
  List<SmartLightScanResult> _found = [];
  bool _scanning = true;
  StreamSubscription<List<SmartLightScanResult>>? _scanSub;
  StreamSubscription<bool>? _scanningSub;

  SmartLightScanResult? _selected;
  final _ssidCtrl = TextEditingController();
  final _passCtrl = TextEditingController();
  bool _obscurePassword = true;

  String _liveStatus = 'idle';
  String? _resultDeviceId;
  String? _error;

  @override
  void initState() {
    super.initState();
    _startScan();
  }

  Future<void> _startScan() async {
    bool supported;
    try {
      supported = await FlutterBluePlus.isSupported;
    } catch (_) {
      supported = false;
    }
    if (!supported) {
      if (mounted) setState(() => _step = _Step.unsupported);
      return;
    }

    setState(() {
      _step = _Step.scanning;
      _found = [];
      _scanning = true;
    });
    _scanningSub?.cancel();
    _scanningSub = FlutterBluePlus.isScanning.listen((scanning) {
      if (mounted) setState(() => _scanning = scanning);
    });
    _scanSub?.cancel();
    try {
      _scanSub = BleProvisioningService.scan().listen(
        (found) {
          if (mounted) setState(() => _found = found);
        },
        onError: (_) {
          if (mounted) setState(() => _step = _Step.unsupported);
        },
      );
    } catch (_) {
      if (mounted) setState(() => _step = _Step.unsupported);
    }
  }

  void _selectDevice(SmartLightScanResult result) {
    _scanSub?.cancel();
    BleProvisioningService.stopScan();
    setState(() {
      _selected = result;
      _step = _Step.credentials;
    });
  }

  Future<void> _submitCredentials() async {
    final ssid = _ssidCtrl.text.trim();
    if (ssid.isEmpty || _selected == null) return;

    final userId = context.read<AuthProvider>().userId;
    if (userId == null) {
      setState(() => _error = 'Não foi possível identificar seu usuário. Faça login novamente.');
      return;
    }

    setState(() {
      _step = _Step.provisioning;
      _liveStatus = 'connecting';
      _error = null;
    });

    final result = await BleProvisioningService.provision(
      device: _selected!.device,
      ssid: ssid,
      password: _passCtrl.text,
      userId: userId,
      onStatus: (status) {
        if (mounted) setState(() => _liveStatus = status.status);
      },
    );

    if (!mounted) return;
    if (result.success) {
      setState(() {
        _step = _Step.success;
        _resultDeviceId = result.deviceId;
      });
    } else {
      setState(() {
        _step = _Step.failure;
        _error = result.error;
      });
    }
  }

  @override
  void dispose() {
    _scanSub?.cancel();
    _scanningSub?.cancel();
    BleProvisioningService.stopScan();
    _ssidCtrl.dispose();
    _passCtrl.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return AlertDialog(
      title: const Text('Procurar dispositivos'),
      content: SizedBox(width: 300, child: _buildContent()),
      actions: _buildActions(),
    );
  }

  Widget _buildContent() {
    switch (_step) {
      case _Step.scanning:
        return _buildScanning();
      case _Step.credentials:
        return _buildCredentialsForm();
      case _Step.provisioning:
        return _buildProvisioning();
      case _Step.success:
        return _buildSuccess();
      case _Step.failure:
        return _buildFailure();
      case _Step.unsupported:
        return const Padding(
          padding: EdgeInsets.symmetric(vertical: 12),
          child: Text(
            'Bluetooth não é suportado nessa plataforma/dispositivo.',
            style: TextStyle(color: AppColors.ink2, fontWeight: FontWeight.w500),
          ),
        );
    }
  }

  Widget _buildScanning() {
    return Column(
      mainAxisSize: MainAxisSize.min,
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        if (_scanning)
          const Padding(
            padding: EdgeInsets.symmetric(vertical: 12),
            child: Column(
              children: [
                SizedBox(height: 22, width: 22, child: CircularProgressIndicator(strokeWidth: 2.5)),
                SizedBox(height: 12),
                Text(
                  'Procurando dispositivos SmartLight próximos via Bluetooth...',
                  textAlign: TextAlign.center,
                  style: TextStyle(color: AppColors.ink2, fontWeight: FontWeight.w500, fontSize: 13),
                ),
              ],
            ),
          )
        else if (_found.isEmpty)
          const Padding(
            padding: EdgeInsets.symmetric(vertical: 12),
            child: Text(
              'Nenhum dispositivo encontrado. Verifique se o interruptor está em modo de pareamento.',
              textAlign: TextAlign.center,
              style: TextStyle(color: AppColors.ink2, fontWeight: FontWeight.w500, fontSize: 13),
            ),
          ),
        if (_found.isNotEmpty)
          ConstrainedBox(
            constraints: const BoxConstraints(maxHeight: 220),
            child: ListView.separated(
              shrinkWrap: true,
              itemCount: _found.length,
              separatorBuilder: (_, _) => const Divider(height: 1, color: AppColors.hair2),
              itemBuilder: (context, i) {
                final result = _found[i];
                return ListTile(
                  contentPadding: EdgeInsets.zero,
                  leading: const Icon(Icons.bluetooth_rounded, color: AppColors.accentInk),
                  title: Text(result.name, style: const TextStyle(fontWeight: FontWeight.w600, fontSize: 14)),
                  subtitle: Text(result.deviceId ?? result.device.remoteId.str,
                      style: const TextStyle(fontSize: 11, color: AppColors.ink2)),
                  trailing: Text('${result.rssi} dBm', style: const TextStyle(fontSize: 11, color: AppColors.ink3)),
                  onTap: () => _selectDevice(result),
                );
              },
            ),
          ),
      ],
    );
  }

  Widget _buildCredentialsForm() {
    return Column(
      mainAxisSize: MainAxisSize.min,
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text('Conectar "${_selected!.name}" ao Wi-Fi', style: const TextStyle(fontWeight: FontWeight.w600)),
        const SizedBox(height: 12),
        TextField(
          controller: _ssidCtrl,
          autofocus: true,
          decoration: const InputDecoration(labelText: 'Nome da rede (SSID)'),
        ),
        const SizedBox(height: 10),
        TextField(
          controller: _passCtrl,
          obscureText: _obscurePassword,
          decoration: InputDecoration(
            labelText: 'Senha',
            suffixIcon: IconButton(
              icon: Icon(_obscurePassword ? Icons.visibility_rounded : Icons.visibility_off_rounded, size: 19),
              onPressed: () => setState(() => _obscurePassword = !_obscurePassword),
            ),
          ),
        ),
        if (_error != null) ...[
          const SizedBox(height: 10),
          Text(_error!, style: const TextStyle(color: AppColors.danger, fontSize: 12, fontWeight: FontWeight.w500)),
        ],
      ],
    );
  }

  Widget _buildProvisioning() {
    final label = switch (_liveStatus) {
      'connecting' => 'Enviando credenciais e aguardando o dispositivo conectar...',
      'failed' => 'Falha ao conectar...',
      _ => 'Aguardando o dispositivo...',
    };
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 12),
      child: Column(
        children: [
          const SizedBox(height: 22, width: 22, child: CircularProgressIndicator(strokeWidth: 2.5)),
          const SizedBox(height: 14),
          Text(label, textAlign: TextAlign.center, style: const TextStyle(color: AppColors.ink2, fontWeight: FontWeight.w500, fontSize: 13)),
        ],
      ),
    );
  }

  Widget _buildSuccess() {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 12),
      child: Column(
        children: [
          const Icon(Icons.check_circle_rounded, size: 36, color: AppColors.accentInk),
          const SizedBox(height: 12),
          const Text('Dispositivo conectado!', style: TextStyle(fontWeight: FontWeight.w700)),
          if (_resultDeviceId != null) ...[
            const SizedBox(height: 4),
            Text(_resultDeviceId!, style: const TextStyle(color: AppColors.ink2, fontSize: 12)),
          ],
          const SizedBox(height: 6),
          const Text(
            'Pode levar alguns segundos para ele aparecer na sua lista.',
            textAlign: TextAlign.center,
            style: TextStyle(color: AppColors.ink2, fontSize: 12, fontWeight: FontWeight.w500),
          ),
        ],
      ),
    );
  }

  Widget _buildFailure() {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 12),
      child: Column(
        children: [
          const Icon(Icons.error_outline_rounded, size: 36, color: AppColors.danger),
          const SizedBox(height: 12),
          Text(
            _error ?? 'Não foi possível provisionar o dispositivo.',
            textAlign: TextAlign.center,
            style: const TextStyle(color: AppColors.ink2, fontWeight: FontWeight.w500, fontSize: 13),
          ),
        ],
      ),
    );
  }

  List<Widget> _buildActions() {
    switch (_step) {
      case _Step.scanning:
        return [
          TextButton(onPressed: () => Navigator.pop(context, false), child: const Text('Cancelar')),
          if (!_scanning) TextButton(onPressed: _startScan, child: const Text('Buscar novamente')),
        ];
      case _Step.credentials:
        return [
          TextButton(onPressed: () => Navigator.pop(context, false), child: const Text('Cancelar')),
          TextButton(onPressed: _submitCredentials, child: const Text('Conectar')),
        ];
      case _Step.provisioning:
        return [
          TextButton(onPressed: () => Navigator.pop(context, false), child: const Text('Cancelar')),
        ];
      case _Step.success:
        return [
          TextButton(onPressed: () => Navigator.pop(context, true), child: const Text('Concluir')),
        ];
      case _Step.failure:
        return [
          TextButton(onPressed: () => Navigator.pop(context, false), child: const Text('Fechar')),
          TextButton(onPressed: () => setState(() => _step = _Step.credentials), child: const Text('Tentar novamente')),
        ];
      case _Step.unsupported:
        return [
          TextButton(onPressed: () => Navigator.pop(context, false), child: const Text('Fechar')),
        ];
    }
  }
}
