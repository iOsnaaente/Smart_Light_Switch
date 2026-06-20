import 'package:flutter/material.dart';

import '../core/theme/app_theme.dart';
import '../models/device.dart';
import 'lux_gauge.dart';
import 'status_chip.dart';

/// Row used in the "Meus interruptores" list — mini gauge, name, status
/// chips and a toggle (mirrors the wireframe `SwitchRow`).
class DeviceListTile extends StatelessWidget {
  final Device device;
  final VoidCallback? onTap;
  final ValueChanged<bool>? onToggle;

  const DeviceListTile({super.key, required this.device, this.onTap, this.onToggle});

  @override
  Widget build(BuildContext context) {
    final lux = device.online ? device.lux.round().toString() : '—';
    return InkWell(
      onTap: onTap,
      child: Container(
        padding: const EdgeInsets.symmetric(vertical: 12, horizontal: 2),
        decoration: const BoxDecoration(border: Border(bottom: BorderSide(color: AppColors.hair2))),
        child: Row(
          children: [
            MiniLuxGauge(percent: device.online ? device.dimmer.toDouble() : 0, big: lux),
            const SizedBox(width: 12),
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(device.name, style: const TextStyle(fontSize: 15.5, fontWeight: FontWeight.w600)),
                  const SizedBox(height: 4),
                  Row(
                    children: [
                      StatusChip(
                        label: device.online ? 'online' : 'OFFLINE',
                        tone: device.online ? ChipTone.ok : ChipTone.off,
                        withDot: true,
                      ),
                      const SizedBox(width: 5),
                      StatusChip(
                        label: device.automaticMode ? 'Automático' : 'Manual',
                        tone: device.automaticMode ? ChipTone.on : ChipTone.cool,
                      ),
                    ],
                  ),
                ],
              ),
            ),
            const SizedBox(width: 8),
            SwitchToggle(value: device.online && device.relayOn, onChanged: device.online ? onToggle : null),
          ],
        ),
      ),
    );
  }
}

/// Card used in the grid layout (mirrors the wireframe `GridCard`).
class DeviceGridCard extends StatelessWidget {
  final Device device;
  final VoidCallback? onTap;
  final ValueChanged<bool>? onToggle;

  const DeviceGridCard({super.key, required this.device, this.onTap, this.onToggle});

  @override
  Widget build(BuildContext context) {
    final lux = device.online ? device.lux.round().toString() : '—';
    return Opacity(
      opacity: device.online ? 1 : 0.6,
      child: InkWell(
        onTap: onTap,
        borderRadius: BorderRadius.circular(18),
        child: Container(
          padding: const EdgeInsets.all(13),
          decoration: BoxDecoration(
            color: AppColors.surface,
            borderRadius: BorderRadius.circular(18),
            border: Border.all(color: AppColors.hair),
          ),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Row(
                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                children: [
                  StatusDot(tone: device.online ? ChipTone.ok : ChipTone.off),
                  SwitchToggle(value: device.online, onChanged: device.online ? onToggle : null),
                ],
              ),
              const SizedBox(height: 9),
              Center(child: MiniLuxGauge(percent: device.online ? device.dimmer.toDouble() : 0, big: lux)),
              const SizedBox(height: 9),
              Text(device.name, style: const TextStyle(fontSize: 14.5, fontWeight: FontWeight.w600)),
              const SizedBox(height: 1),
              Text(device.room, style: const TextStyle(fontSize: 10.5, color: AppColors.ink2, fontWeight: FontWeight.w500)),
              const SizedBox(height: 9),
              StatusChip(label: device.automaticMode ? 'Automático' : 'Manual', tone: device.automaticMode ? ChipTone.on : ChipTone.cool),
            ],
          ),
        ),
      ),
    );
  }
}

/// Small on/off switch styled after the wireframe `Toggle`.
class SwitchToggle extends StatelessWidget {
  final bool value;
  final ValueChanged<bool>? onChanged;

  const SwitchToggle({super.key, required this.value, this.onChanged});

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: onChanged == null ? null : () => onChanged!(!value),
      child: AnimatedContainer(
        duration: const Duration(milliseconds: 160),
        width: 42,
        height: 24,
        padding: const EdgeInsets.all(2.5),
        decoration: BoxDecoration(
          color: value ? AppColors.accent : AppColors.fill,
          borderRadius: BorderRadius.circular(14),
          border: Border.all(color: value ? const Color(0x2E9A6412) : AppColors.hair),
        ),
        child: AnimatedAlign(
          duration: const Duration(milliseconds: 160),
          curve: Curves.easeOut,
          alignment: value ? Alignment.centerRight : Alignment.centerLeft,
          child: Container(
            width: 19,
            height: 19,
            decoration: const BoxDecoration(
              color: Colors.white,
              shape: BoxShape.circle,
              boxShadow: [BoxShadow(color: Color(0x40231E16), blurRadius: 3, offset: Offset(0, 1))],
            ),
          ),
        ),
      ),
    );
  }
}
