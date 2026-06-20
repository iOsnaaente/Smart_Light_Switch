import 'dart:math' as math;

import 'package:flutter/material.dart';

import '../core/theme/app_theme.dart';

/// Duration/curve shared by the gauges below so a backend reading never just
/// "pops" into place — it glides from the previous value to the new one,
/// which is what actually reads as fluid when polling refreshes every ~500ms.
const _gaugeAnimDuration = Duration(milliseconds: 450);
const _gaugeAnimCurve = Curves.easeOut;

/// Circular gauge showing the measured lux against the setpoint
/// (mirrors the wireframe `Dial`). Sweeps 270° starting at the
/// bottom-left, like a dashboard gauge.
class LuxGauge extends StatelessWidget {
  final double size;
  final double percent; // 0..100
  final String big;
  final String unit;
  final String caption;

  const LuxGauge({
    super.key,
    this.size = 184,
    required this.percent,
    required this.big,
    this.unit = 'lux',
    this.caption = '',
  });

  @override
  Widget build(BuildContext context) {
    final target = percent.clamp(0, 100).toDouble();
    final numericBig = double.tryParse(big);
    final bigStyle = TextStyle(
      fontSize: size * 0.21,
      fontWeight: FontWeight.w800,
      color: AppColors.ink,
      height: 1,
      letterSpacing: -0.5,
    );

    return SizedBox(
      width: size,
      height: size,
      child: Stack(
        alignment: Alignment.center,
        children: [
          if (target > 0)
            Container(
              width: size * 0.68,
              height: size * 0.68,
              decoration: BoxDecoration(
                shape: BoxShape.circle,
                gradient: RadialGradient(
                  colors: [AppColors.accentSoft2, AppColors.accentSoft2.withValues(alpha: 0)],
                ),
              ),
            ),
          TweenAnimationBuilder<double>(
            tween: Tween<double>(begin: target, end: target),
            duration: _gaugeAnimDuration,
            curve: _gaugeAnimCurve,
            builder: (context, animatedPercent, _) => CustomPaint(
              size: Size(size, size),
              painter: _GaugePainter(percent: animatedPercent),
            ),
          ),
          Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              if (numericBig != null)
                TweenAnimationBuilder<double>(
                  tween: Tween<double>(begin: numericBig, end: numericBig),
                  duration: _gaugeAnimDuration,
                  curve: _gaugeAnimCurve,
                  builder: (context, animatedValue, _) => Text(animatedValue.round().toString(), style: bigStyle),
                )
              else
                Text(big, style: bigStyle),
              const SizedBox(height: 2),
              Text(unit, style: const TextStyle(fontSize: 11.5, color: AppColors.ink2, fontWeight: FontWeight.w500)),
              if (caption.isNotEmpty) ...[
                const SizedBox(height: 5),
                Text(
                  caption.toUpperCase(),
                  textAlign: TextAlign.center,
                  style: labelStyle(color: AppColors.accentInk, fontSize: 10, weight: FontWeight.w700),
                ),
              ],
            ],
          ),
        ],
      ),
    );
  }
}

class _GaugePainter extends CustomPainter {
  final double percent;
  final double strokeWidth;
  final double inset;
  _GaugePainter({required this.percent, this.strokeWidth = 10, this.inset = 13});

  static const _sweep = 0.75; // 270° of the circle
  static const _startAngle = math.pi * 0.75; // matches `rotate(135deg)` in the wireframe

  @override
  void paint(Canvas canvas, Size size) {
    final center = Offset(size.width / 2, size.height / 2);
    final radius = size.width / 2 - inset;
    final rect = Rect.fromCircle(center: center, radius: radius);

    final track = Paint()
      ..color = AppColors.hair
      ..style = PaintingStyle.stroke
      ..strokeWidth = strokeWidth
      ..strokeCap = StrokeCap.round;
    canvas.drawArc(rect, _startAngle, 2 * math.pi * _sweep, false, track);

    if (percent > 0) {
      final fill = Paint()
        ..color = AppColors.accent
        ..style = PaintingStyle.stroke
        ..strokeWidth = strokeWidth
        ..strokeCap = StrokeCap.round;
      canvas.drawArc(rect, _startAngle, 2 * math.pi * _sweep * (percent.clamp(0, 100) / 100), false, fill);
    }
  }

  @override
  bool shouldRepaint(covariant _GaugePainter oldDelegate) =>
      oldDelegate.percent != percent || oldDelegate.strokeWidth != strokeWidth || oldDelegate.inset != inset;
}

/// Compact gauge used in list/grid rows (mirrors the wireframe `MiniDial`).
class MiniLuxGauge extends StatelessWidget {
  final double percent;
  final String big;
  final double size;

  const MiniLuxGauge({super.key, required this.percent, required this.big, this.size = 64});

  @override
  Widget build(BuildContext context) {
    final target = percent.clamp(0, 100).toDouble();
    final numericBig = double.tryParse(big);
    const bigStyle = TextStyle(fontSize: 13, fontWeight: FontWeight.w800, letterSpacing: -0.3);

    return SizedBox(
      width: size,
      height: size,
      child: Stack(
        alignment: Alignment.center,
        children: [
          TweenAnimationBuilder<double>(
            tween: Tween<double>(begin: target, end: target),
            duration: _gaugeAnimDuration,
            curve: _gaugeAnimCurve,
            builder: (context, animatedPercent, _) => CustomPaint(
              size: Size(size, size),
              painter: _GaugePainter(percent: animatedPercent, strokeWidth: 6, inset: 8),
            ),
          ),
          if (numericBig != null)
            TweenAnimationBuilder<double>(
              tween: Tween<double>(begin: numericBig, end: numericBig),
              duration: _gaugeAnimDuration,
              curve: _gaugeAnimCurve,
              builder: (context, animatedValue, _) => Text(animatedValue.round().toString(), style: bigStyle),
            )
          else
            Text(big, style: bigStyle),
        ],
      ),
    );
  }
}
