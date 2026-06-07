import 'package:flutter/material.dart';

import '../core/theme/app_theme.dart';

/// Manual/Automático segmented toggle (mirrors the wireframe `Segmented`).
class ModeSegment extends StatelessWidget {
  final List<String> options;
  final int activeIndex;
  final ValueChanged<int>? onChanged;

  const ModeSegment({super.key, required this.options, required this.activeIndex, this.onChanged});

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.all(4),
      decoration: BoxDecoration(color: AppColors.fill, borderRadius: BorderRadius.circular(14)),
      child: Row(
        children: List.generate(options.length, (i) {
          final active = i == activeIndex;
          return Expanded(
            child: GestureDetector(
              onTap: onChanged == null ? null : () => onChanged!(i),
              child: AnimatedContainer(
                duration: const Duration(milliseconds: 160),
                padding: const EdgeInsets.symmetric(vertical: 9),
                decoration: BoxDecoration(
                  color: active ? AppColors.surface : Colors.transparent,
                  borderRadius: BorderRadius.circular(10),
                  boxShadow: active ? const [BoxShadow(color: Color(0x1A231E16), blurRadius: 2, offset: Offset(0, 1))] : null,
                ),
                child: Text(
                  options[i],
                  textAlign: TextAlign.center,
                  style: TextStyle(
                    fontSize: 14,
                    fontWeight: active ? FontWeight.w700 : FontWeight.w500,
                    color: active ? AppColors.ink : AppColors.ink2,
                  ),
                ),
              ),
            ),
          );
        }),
      ),
    );
  }
}

/// Small metric tile (mirrors the wireframe `Stat`).
class StatTile extends StatelessWidget {
  final String value;
  final String unit;
  final String label;
  final ChipToneAccent tone;

  const StatTile({super.key, required this.value, required this.unit, required this.label, this.tone = ChipToneAccent.plain});

  @override
  Widget build(BuildContext context) {
    final fg = switch (tone) {
      ChipToneAccent.accent => AppColors.accentInk,
      ChipToneAccent.cool => AppColors.coolInk,
      ChipToneAccent.plain => AppColors.ink,
    };
    return Expanded(
      child: Container(
        padding: const EdgeInsets.symmetric(vertical: 11, horizontal: 8),
        decoration: BoxDecoration(
          color: AppColors.surface,
          borderRadius: BorderRadius.circular(14),
          border: Border.all(color: AppColors.hair),
        ),
        child: Column(
          children: [
            RichText(
              text: TextSpan(
                children: [
                  TextSpan(text: value, style: TextStyle(fontSize: 18, fontWeight: FontWeight.w800, color: fg, letterSpacing: -0.3)),
                  TextSpan(text: ' $unit', style: const TextStyle(fontSize: 10.5, fontWeight: FontWeight.w600, color: AppColors.ink2)),
                ],
              ),
            ),
            const SizedBox(height: 5),
            Text(label.toUpperCase(), style: labelStyle(fontSize: 9.5)),
          ],
        ),
      ),
    );
  }
}

enum ChipToneAccent { plain, accent, cool }

/// Tiny line chart comparing natural daylight vs. lamp light through the day
/// (mirrors the wireframe `DayChart`).
class DayChart extends StatelessWidget {
  final double height;
  final String caption;

  const DayChart({super.key, this.height = 84, this.caption = 'hoje'});

  @override
  Widget build(BuildContext context) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        Container(
          height: height,
          decoration: BoxDecoration(
            color: AppColors.surface,
            borderRadius: BorderRadius.circular(12),
            border: Border.all(color: AppColors.hair),
          ),
          clipBehavior: Clip.antiAlias,
          child: CustomPaint(painter: _DayChartPainter(), size: Size.infinite),
        ),
        const SizedBox(height: 8),
        Row(
          children: [
            _legend(AppColors.cool, 'natural'),
            const SizedBox(width: 14),
            _legend(AppColors.accent, 'lâmpada'),
            const Spacer(),
            Text(caption.toUpperCase(), style: labelStyle(fontSize: 9.5)),
          ],
        ),
      ],
    );
  }

  Widget _legend(Color color, String label) {
    return Row(
      mainAxisSize: MainAxisSize.min,
      children: [
        Container(width: 13, height: 2.5, decoration: BoxDecoration(color: color, borderRadius: BorderRadius.circular(2))),
        const SizedBox(width: 5),
        Text(label, style: const TextStyle(fontSize: 10, color: AppColors.ink2, fontWeight: FontWeight.w500)),
      ],
    );
  }
}

class _DayChartPainter extends CustomPainter {
  static const _natural = [0.78, 0.55, 0.30, 0.16, 0.20, 0.34, 0.58, 0.80];
  static const _lamp = [0.30, 0.42, 0.62, 0.74, 0.71, 0.58, 0.40, 0.26];

  @override
  void paint(Canvas canvas, Size size) {
    _drawLine(canvas, size, _natural, AppColors.cool, 2.4);
    _drawLine(canvas, size, _lamp, AppColors.accent, 2.8);
  }

  void _drawLine(Canvas canvas, Size size, List<double> values, Color color, double strokeWidth) {
    final path = Path();
    for (var i = 0; i < values.length; i++) {
      final x = size.width * i / (values.length - 1);
      final y = size.height * values[i];
      if (i == 0) {
        path.moveTo(x, y);
      } else {
        path.lineTo(x, y);
      }
    }
    final paint = Paint()
      ..color = color
      ..style = PaintingStyle.stroke
      ..strokeWidth = strokeWidth
      ..strokeCap = StrokeCap.round
      ..strokeJoin = StrokeJoin.round;
    canvas.drawPath(path, paint);
  }

  @override
  bool shouldRepaint(covariant _DayChartPainter oldDelegate) => false;
}
