import 'dart:async';
import 'dart:convert';

import 'package:flutter_blue_plus/flutter_blue_plus.dart';

/// Scans for and provisions "SmartLight" devices over BLE.
///
/// Mirrors `Firmware/communication/ble_setup.cpp` exactly — if the GATT
/// layout, advertising format, or JSON payloads change there, update this
/// file too:
///   - advertised name: "SmartLight-XXXX" (device_identity_get_ble_name)
///   - main adv packet carries a fixed manufacturer-data tag "BrunoSampaio"
///     with no company-id prefix (s_adv_mfg_data) — used here to identify
///     genuine SmartLight devices before connecting
///   - scan response carries manufacturer id 0xFFFF + the device_id ASCII
///     (s_scan_rsp_mfg_data), readable without connecting
///   - GATT service 0x1205, credentials char 0x010A (write JSON
///     {"ssid","password","user_id"}), status char 0x020B (read/notify JSON
///     {"status":"idle|connecting|connected|failed","device_id"})
class BleProvisioningService {
  BleProvisioningService._();

  static final Guid serviceUuid = Guid('1205');
  static final Guid credentialsCharUuid = Guid('010A');
  static final Guid statusCharUuid = Guid('020B');

  static const String _companyTag = 'BrunoSampaio';
  static final List<int> _companyTagBytes = utf8.encode(_companyTag);
  static const int _deviceIdManufacturerId = 0xFFFF;
  static const String namePrefix = 'SmartLight-';

  static const Duration scanTimeout = Duration(seconds: 8);
  static const Duration provisionTimeout = Duration(seconds: 25);

  /// Starts a scan and streams the de-duplicated, filtered list of nearby
  /// SmartLight devices found so far (sorted by signal strength).
  static Stream<List<SmartLightScanResult>> scan() {
    FlutterBluePlus.startScan(withKeywords: [namePrefix], timeout: scanTimeout).catchError((_) {});
    return FlutterBluePlus.scanResults.map((results) {
      final matches = <SmartLightScanResult>[];
      for (final r in results) {
        if (!_isSmartLightDevice(r)) continue;
        final name = r.advertisementData.advName.isNotEmpty ? r.advertisementData.advName : r.device.platformName;
        matches.add(SmartLightScanResult(
          device: r.device,
          name: name,
          deviceId: _extractDeviceId(r.advertisementData),
          rssi: r.rssi,
        ));
      }
      matches.sort((a, b) => b.rssi.compareTo(a.rssi));
      return matches;
    });
  }

  static Future<void> stopScan() => FlutterBluePlus.stopScan().catchError((_) {});

  static bool _isSmartLightDevice(ScanResult r) {
    final name = r.advertisementData.advName.isNotEmpty ? r.advertisementData.advName : r.device.platformName;
    if (!name.startsWith(namePrefix)) return false;
    for (final raw in r.advertisementData.msd) {
      if (_startsWith(raw, _companyTagBytes)) return true;
    }
    return false;
  }

  static bool _startsWith(List<int> data, List<int> prefix) {
    if (data.length < prefix.length) return false;
    for (var i = 0; i < prefix.length; i++) {
      if (data[i] != prefix[i]) return false;
    }
    return true;
  }

  static String? _extractDeviceId(AdvertisementData adv) {
    final bytes = adv.manufacturerData[_deviceIdManufacturerId];
    if (bytes == null || bytes.isEmpty) return null;
    return utf8.decode(bytes, allowMalformed: true);
  }

  /// Connects to [device], writes Wi-Fi credentials + [userId] to the
  /// credentials characteristic, and resolves once the firmware reports
  /// "connected" (with the final `device_id`) or "failed", or after
  /// [provisionTimeout] elapses.
  static Future<BleProvisionResult> provision({
    required BluetoothDevice device,
    required String ssid,
    required String password,
    required int userId,
    void Function(BleProvisionStatus status)? onStatus,
  }) async {
    final completer = Completer<BleProvisionResult>();
    StreamSubscription<List<int>>? statusSub;

    try {
      await device.connect(license: License.nonprofit, timeout: const Duration(seconds: 15));

      final services = await device.discoverServices();
      final service = services.firstWhere(
        (s) => s.uuid == serviceUuid,
        orElse: () => throw const BleProvisionException('Serviço de provisionamento não encontrado no dispositivo'),
      );
      final credChar = service.characteristics.firstWhere(
        (c) => c.uuid == credentialsCharUuid,
        orElse: () => throw const BleProvisionException('Característica de credenciais não encontrada'),
      );
      final statusChar = service.characteristics.firstWhere(
        (c) => c.uuid == statusCharUuid,
        orElse: () => throw const BleProvisionException('Característica de status não encontrada'),
      );

      await statusChar.setNotifyValue(true);
      statusSub = statusChar.lastValueStream.listen((value) {
        final parsed = _parseStatus(value);
        if (parsed == null || completer.isCompleted) return;
        onStatus?.call(parsed);
        if (parsed.status == 'connected') {
          completer.complete(BleProvisionResult.ok(parsed.deviceId));
        } else if (parsed.status == 'failed') {
          completer.complete(BleProvisionResult.error('O dispositivo não conseguiu se conectar a essa rede Wi-Fi'));
        }
      });

      final payload = jsonEncode({'ssid': ssid, 'password': password, 'user_id': userId});
      await credChar.write(utf8.encode(payload), withoutResponse: false);

      return await completer.future.timeout(
        provisionTimeout,
        onTimeout: () => BleProvisionResult.error('Tempo esgotado esperando o dispositivo conectar'),
      );
    } on BleProvisionException catch (e) {
      return BleProvisionResult.error(e.message);
    } catch (e) {
      return BleProvisionResult.error('Erro de comunicação BLE: $e');
    } finally {
      await statusSub?.cancel();
      try {
        await device.disconnect();
      } catch (_) {
        // já desconectado ou dispositivo saiu de alcance — sem ação necessária.
      }
    }
  }

  static BleProvisionStatus? _parseStatus(List<int> value) {
    if (value.isEmpty) return null;
    try {
      final json = jsonDecode(utf8.decode(value)) as Map<String, dynamic>;
      return BleProvisionStatus(
        status: json['status'] as String? ?? 'idle',
        deviceId: json['device_id'] as String?,
      );
    } catch (_) {
      return null;
    }
  }
}

class SmartLightScanResult {
  final BluetoothDevice device;
  final String name;
  final String? deviceId;
  final int rssi;

  SmartLightScanResult({required this.device, required this.name, required this.deviceId, required this.rssi});
}

class BleProvisionStatus {
  final String status;
  final String? deviceId;
  const BleProvisionStatus({required this.status, this.deviceId});
}

class BleProvisionException implements Exception {
  final String message;
  const BleProvisionException(this.message);
}

class BleProvisionResult {
  final bool success;
  final String? deviceId;
  final String? error;

  const BleProvisionResult._({required this.success, this.deviceId, this.error});

  factory BleProvisionResult.ok(String? deviceId) => BleProvisionResult._(success: true, deviceId: deviceId);
  factory BleProvisionResult.error(String message) => BleProvisionResult._(success: false, error: message);
}
