import 'package:flutter/material.dart';

import '../core/theme/app_theme.dart';

/// Horizontal "dimmer" slider styled after the wireframe `Slider` —
/// pill track, filled progress, and a ringed handle.
class DimmerSlider extends StatelessWidget {
  final double percent; // 0..100
  final String? label;
  final bool muted;
  final ValueChanged<double>? onChanged;

  const DimmerSlider({
    super.key,
    required this.percent,
    this.label,
    this.muted = false,
    this.onChanged,
  });

  @override
  Widget build(BuildContext context) {
    final color = muted ? AppColors.ink3 : AppColors.accent;
    final pct = percent.clamp(0, 100).toDouble();

    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        if (label != null)
          Padding(
            padding: const EdgeInsets.only(bottom: 9),
            child: Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                Text(
                  label!.toUpperCase(),
                  style: labelStyle(fontSize: 10, color: AppColors.ink2),
                ),
                Text(
                  '${pct.round()}%',
                  style: TextStyle(
                    fontSize: 10,
                    fontWeight: FontWeight.w700,
                    letterSpacing: 0.4,
                    color: muted ? AppColors.ink2 : AppColors.accentInk,
                  ),
                ),
              ],
            ),
          ),
        LayoutBuilder(
          builder: (context, constraints) {
            final width = constraints.maxWidth;
            void handleDrag(Offset local) {
              if (onChanged == null) return;
              final ratio = (local.dx / width).clamp(0.0, 1.0);
              onChanged!((ratio * 100));
            }

            return GestureDetector(
              behavior: HitTestBehavior.opaque,
              onHorizontalDragUpdate: onChanged == null ? null : (d) => handleDrag(d.localPosition),
              onTapDown: onChanged == null ? null : (d) => handleDrag(d.localPosition),
              child: SizedBox(
                height: 24,
                child: Stack(
                  alignment: Alignment.centerLeft,
                  children: [
                    Container(
                      height: 10,
                      decoration: BoxDecoration(color: AppColors.fill, borderRadius: BorderRadius.circular(8)),
                    ),
                    FractionallySizedBox(
                      widthFactor: pct / 100,
                      child: Container(
                        height: 10,
                        decoration: BoxDecoration(
                          color: muted ? AppColors.ink3.withValues(alpha: 0.5) : AppColors.accent,
                          borderRadius: BorderRadius.circular(8),
                        ),
                      ),
                    ),
                    Align(
                      alignment: Alignment(pct / 100 * 2 - 1, 0),
                      child: Container(
                        width: 24,
                        height: 24,
                        decoration: BoxDecoration(
                          color: AppColors.surface,
                          shape: BoxShape.circle,
                          border: Border.all(color: color, width: 2),
                          boxShadow: const [BoxShadow(color: Color(0x2E231E16), blurRadius: 6, offset: Offset(0, 2))],
                        ),
                      ),
                    ),
                  ],
                ),
              ),
            );
          },
        ),
      ],
    );
  }
}
