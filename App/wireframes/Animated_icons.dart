// SmartLight — Animated Icons (Flutter / CustomPainter)
// Paleta do app: âmbar #F7B24B, azul #5C92DC, traço fino.
//
// Uso:
//   const SmartLightBulbIcon(on: true, size: 48)
//   const IntensityIcon(level: 0.62, size: 48)   // 0..1
//   const ConnectionIcon(active: true, size: 36)
//   const SettingsIcon(open: true, size: 28)
//
// Todos animam sozinhos ao mudar a propriedade (implicit animations
// via AnimationController). Cores expostas em SmartLightColors.

import 'dart:math' as math;
import 'package:flutter/material.dart';

class SmartLightColors {
  static const amber = Color(0xFFF7B24B);
  static const amberSoft = Color(0xFFFFD27E);
  static const blue = Color(0xFF5C92DC);
  static const ink = Color(0xFFFFF6E6);
  static const off = Color(0xFF5A5040);
}

// ───────────────────────── Lâmpada on/off ─────────────────────────
class SmartLightBulbIcon extends StatefulWidget {
  final bool on;
  final double size;
  const SmartLightBulbIcon({super.key, required this.on, this.size = 48});

  @override
  State<SmartLightBulbIcon> createState() => _SmartLightBulbIconState();
}

class _SmartLightBulbIconState extends State<SmartLightBulbIcon>
    with SingleTickerProviderStateMixin {
  late final AnimationController _c;

  @override
  void initState() {
    super.initState();
    _c = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 420),
      value: widget.on ? 1 : 0,
    );
  }

  @override
  void didUpdateWidget(covariant SmartLightBulbIcon old) {
    super.didUpdateWidget(old);
    if (old.on != widget.on) {
      widget.on ? _c.forward() : _c.reverse();
    }
  }

  @override
  void dispose() {
    _c.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return AnimatedBuilder(
      animation: _c,
      builder: (_, __) => CustomPaint(
        size: Size.square(widget.size),
        painter: _BulbPainter(CurvedAnimation(parent: _c, curve: Curves.easeOutBack).value.clamp(0, 1)),
      ),
    );
  }
}

class _BulbPainter extends CustomPainter {
  final double t; // 0 off → 1 on
  _BulbPainter(this.t);

  @override
  void paint(Canvas canvas, Size size) {
    final w = size.width, h = size.height;
    final stroke = w * 0.07;
    final cx = w / 2;
    final r = w * 0.30;
    final cy = h * 0.40;

    final glassColor = Color.lerp(SmartLightColors.off, SmartLightColors.amber, t)!;

    // Halo de luz cresce com t
    if (t > 0) {
      final halo = Paint()
        ..shader = RadialGradient(
          colors: [
            SmartLightColors.amberSoft.withOpacity(0.55 * t),
            SmartLightColors.amber.withOpacity(0),
          ],
        ).createShader(Rect.fromCircle(center: Offset(cx, cy), radius: r * 2.4));
      canvas.drawCircle(Offset(cx, cy), r * 2.4, halo);
    }

    final line = Paint()
      ..style = PaintingStyle.stroke
      ..strokeWidth = stroke
      ..strokeCap = StrokeCap.round
      ..strokeJoin = StrokeJoin.round
      ..color = glassColor;

    // Bulbo (preenchido quando ligado)
    canvas.drawCircle(
      Offset(cx, cy),
      r,
      Paint()..color = SmartLightColors.amber.withOpacity(0.16 * t),
    );
    canvas.drawCircle(Offset(cx, cy), r, line);

    // Base / rosca
    final baseTop = cy + r * 0.95;
    for (int i = 0; i < 3; i++) {
      final y = baseTop + i * stroke * 1.7;
      canvas.drawLine(Offset(cx - r * 0.45, y), Offset(cx + r * 0.45, y), line);
    }

    // Filamento: raios que "acendem" com t
    final rayPaint = Paint()
      ..style = PaintingStyle.stroke
      ..strokeWidth = stroke * 0.8
      ..strokeCap = StrokeCap.round
      ..color = Color.lerp(SmartLightColors.off, SmartLightColors.amberSoft, t)!;
    if (t > 0.05) {
      for (int i = 0; i < 6; i++) {
        final a = (math.pi / 3) * i - math.pi / 2;
        final inner = r * 0.18;
        final outer = r * (0.18 + 0.42 * t);
        canvas.drawLine(
          Offset(cx + math.cos(a) * inner, cy + math.sin(a) * inner),
          Offset(cx + math.cos(a) * outer, cy + math.sin(a) * outer),
          rayPaint,
        );
      }
    }
  }

  @override
  bool shouldRepaint(covariant _BulbPainter old) => old.t != t;
}

// ──────────────────── Sol / Intensidade (0..1) ────────────────────
class IntensityIcon extends StatefulWidget {
  final double level; // 0..1
  final double size;
  const IntensityIcon({super.key, required this.level, this.size = 48});

  @override
  State<IntensityIcon> createState() => _IntensityIconState();
}

class _IntensityIconState extends State<IntensityIcon>
    with SingleTickerProviderStateMixin {
  late final AnimationController _c;
  late double _from, _to;

  @override
  void initState() {
    super.initState();
    _from = _to = widget.level.clamp(0, 1);
    _c = AnimationController(
        vsync: this, duration: const Duration(milliseconds: 500))
      ..value = 1;
  }

  @override
  void didUpdateWidget(covariant IntensityIcon old) {
    super.didUpdateWidget(old);
    if (old.level != widget.level) {
      _from = _lerpNow();
      _to = widget.level.clamp(0, 1);
      _c.forward(from: 0);
    }
  }

  double _lerpNow() => _from + (_to - _from) *
      Curves.easeOut.transform(_c.value);

  @override
  void dispose() {
    _c.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return AnimatedBuilder(
      animation: _c,
      builder: (_, __) => CustomPaint(
        size: Size.square(widget.size),
        painter: _SunPainter(_lerpNow()),
      ),
    );
  }
}

class _SunPainter extends CustomPainter {
  final double v; // 0..1
  _SunPainter(this.v);

  @override
  void paint(Canvas canvas, Size size) {
    final w = size.width;
    final cx = w / 2, cy = w / 2;
    final core = w * 0.18;
    final stroke = w * 0.065;

    // Cor varia de azul (frio/baixo) para âmbar (quente/alto)
    final c = Color.lerp(SmartLightColors.blue, SmartLightColors.amber, v)!;

    final core1 = Paint()..color = c;
    canvas.drawCircle(Offset(cx, cy), core, core1);

    final ray = Paint()
      ..style = PaintingStyle.stroke
      ..strokeWidth = stroke
      ..strokeCap = StrokeCap.round
      ..color = c;

    // 8 raios cujo comprimento cresce com a intensidade
    for (int i = 0; i < 8; i++) {
      final a = (math.pi / 4) * i;
      final inner = core * 1.5;
      final outer = core * (1.5 + 1.4 * v);
      canvas.drawLine(
        Offset(cx + math.cos(a) * inner, cy + math.sin(a) * inner),
        Offset(cx + math.cos(a) * outer, cy + math.sin(a) * outer),
        ray,
      );
    }
  }

  @override
  bool shouldRepaint(covariant _SunPainter old) => old.v != v;
}

// ───────────────────── Conexão (barras pulsando) ─────────────────────
class ConnectionIcon extends StatefulWidget {
  final bool active;
  final double size;
  const ConnectionIcon({super.key, required this.active, this.size = 36});

  @override
  State<ConnectionIcon> createState() => _ConnectionIconState();
}

class _ConnectionIconState extends State<ConnectionIcon>
    with SingleTickerProviderStateMixin {
  late final AnimationController _c;

  @override
  void initState() {
    super.initState();
    _c = AnimationController(
        vsync: this, duration: const Duration(milliseconds: 1100))
      ..repeat();
  }

  @override
  void dispose() {
    _c.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return AnimatedBuilder(
      animation: _c,
      builder: (_, __) => CustomPaint(
        size: Size.square(widget.size),
        painter: _BarsPainter(widget.active ? _c.value : -1),
      ),
    );
  }
}

class _BarsPainter extends CustomPainter {
  final double phase; // -1 = inativo
  _BarsPainter(this.phase);

  @override
  void paint(Canvas canvas, Size size) {
    final w = size.width, h = size.height;
    const n = 4;
    final bw = w * 0.13;
    final gap = (w - bw * n) / (n - 1);
    final heights = [0.32, 0.55, 0.78, 1.0];

    for (int i = 0; i < n; i++) {
      double lit = 1;
      if (phase < 0) {
        lit = 0.25; // inativo: tudo apagado
      } else {
        // onda que percorre as barras
        final t = (phase * n - i).clamp(0.0, 1.0);
        lit = 0.35 + 0.65 * (math.sin(t * math.pi)).clamp(0.0, 1.0);
      }
      final bh = h * heights[i];
      final x = i * (bw + gap);
      final rect = RRect.fromRectAndRadius(
        Rect.fromLTWH(x, h - bh, bw, bh),
        Radius.circular(bw * 0.5),
      );
      canvas.drawRRect(
        rect,
        Paint()..color = SmartLightColors.amber.withOpacity(lit),
      );
    }
  }

  @override
  bool shouldRepaint(covariant _BarsPainter old) => old.phase != phase;
}

// ─────────────────── Settings (sliders deslizam) ───────────────────
class SettingsIcon extends StatefulWidget {
  final bool open; // controla a posição dos knobs
  final double size;
  const SettingsIcon({super.key, required this.open, this.size = 28});

  @override
  State<SettingsIcon> createState() => _SettingsIconState();
}

class _SettingsIconState extends State<SettingsIcon>
    with SingleTickerProviderStateMixin {
  late final AnimationController _c;

  @override
  void initState() {
    super.initState();
    _c = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 380),
      value: widget.open ? 1 : 0,
    );
  }

  @override
  void didUpdateWidget(covariant SettingsIcon old) {
    super.didUpdateWidget(old);
    if (old.open != widget.open) {
      widget.open ? _c.forward() : _c.reverse();
    }
  }

  @override
  void dispose() {
    _c.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return AnimatedBuilder(
      animation: _c,
      builder: (_, __) => CustomPaint(
        size: Size.square(widget.size),
        painter: _SlidersPainter(
            Curves.easeInOut.transform(_c.value)),
      ),
    );
  }
}

class _SlidersPainter extends CustomPainter {
  final double t;
  _SlidersPainter(this.t);

  @override
  void paint(Canvas canvas, Size size) {
    final w = size.width, h = size.height;
    final stroke = w * 0.085;
    final knobR = w * 0.13;

    final line = Paint()
      ..style = PaintingStyle.stroke
      ..strokeWidth = stroke
      ..strokeCap = StrokeCap.round
      ..color = SmartLightColors.ink;

    // duas trilhas
    final y1 = h * 0.32, y2 = h * 0.68;
    canvas.drawLine(Offset(w * 0.12, y1), Offset(w * 0.88, y1), line);
    canvas.drawLine(Offset(w * 0.12, y2), Offset(w * 0.88, y2), line);

    // knobs deslizam em direções opostas
    final knob = Paint()..color = SmartLightColors.amber;
    final x1 = lerpDouble(w * 0.32, w * 0.66, t);
    final x2 = lerpDouble(w * 0.66, w * 0.34, t);
    canvas.drawCircle(Offset(x1, y1), knobR,
        Paint()..color = SmartLightColors.ink); // borda
    canvas.drawCircle(Offset(x1, y1), knobR * 0.72, knob);
    canvas.drawCircle(Offset(x2, y2), knobR,
        Paint()..color = SmartLightColors.ink);
    canvas.drawCircle(Offset(x2, y2), knobR * 0.72, knob);
  }

  static double lerpDouble(double a, double b, double t) => a + (b - a) * t;

  @override
  bool shouldRepaint(covariant _SlidersPainter old) => old.t != t;
}
