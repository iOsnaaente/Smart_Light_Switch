import 'package:flutter/material.dart';

import '../core/theme/app_theme.dart';

enum ChipTone { plain, on, cool, ok, off }

class _ChipColors {
  final Color background;
  final Color border;
  final Color foreground;
  const _ChipColors(this.background, this.border, this.foreground);
}

const _toneColors = {
  ChipTone.plain: _ChipColors(AppColors.surface, AppColors.hair, AppColors.ink2),
  ChipTone.on: _ChipColors(AppColors.accentSoft, Colors.transparent, AppColors.accentInk),
  ChipTone.cool: _ChipColors(AppColors.coolSoft, Colors.transparent, AppColors.coolInk),
  ChipTone.ok: _ChipColors(AppColors.okSoft, Colors.transparent, Color(0xFF3F7256)),
  ChipTone.off: _ChipColors(AppColors.fill, Colors.transparent, AppColors.ink2),
};

const _dotColors = {
  ChipTone.ok: AppColors.ok,
  ChipTone.off: AppColors.off,
  ChipTone.on: AppColors.accent,
  ChipTone.cool: AppColors.cool,
  ChipTone.plain: AppColors.ink2,
};

/// Small pill badge used for status/labels (mirrors the wireframe `Chip`).
class StatusChip extends StatelessWidget {
  final String label;
  final ChipTone tone;
  final bool withDot;

  const StatusChip({super.key, required this.label, this.tone = ChipTone.plain, this.withDot = false});

  /// Convenience constructor for the online/offline device badge.
  factory StatusChip.connection({required bool online, String? subtitle}) {
    final text = online ? 'ONLINE' : 'OFFLINE${subtitle != null ? ' · $subtitle' : ''}';
    return StatusChip(label: text, tone: online ? ChipTone.ok : ChipTone.off, withDot: true);
  }

  @override
  Widget build(BuildContext context) {
    final colors = _toneColors[tone]!;
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 5),
      decoration: BoxDecoration(
        color: colors.background,
        border: Border.all(color: colors.border),
        borderRadius: BorderRadius.circular(20),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          if (withDot) ...[
            Container(
              width: 7,
              height: 7,
              decoration: BoxDecoration(color: _dotColors[tone], shape: BoxShape.circle),
            ),
            const SizedBox(width: 5),
          ],
          Text(
            label,
            style: TextStyle(
              fontSize: 10.5,
              fontWeight: FontWeight.w700,
              letterSpacing: 0.3,
              color: colors.foreground,
            ),
          ),
        ],
      ),
    );
  }
}

/// Tiny solid status dot (mirrors the wireframe `Dot`).
class StatusDot extends StatelessWidget {
  final ChipTone tone;
  final double size;
  const StatusDot({super.key, this.tone = ChipTone.ok, this.size = 7});

  @override
  Widget build(BuildContext context) {
    return Container(
      width: size,
      height: size,
      decoration: BoxDecoration(color: _dotColors[tone], shape: BoxShape.circle),
    );
  }
}
