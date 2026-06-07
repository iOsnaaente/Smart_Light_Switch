import 'package:flutter/material.dart';

/// Color tokens lifted from the wireframe kit (warm neutrals · amber lamp
/// light · cool blue daylight). Keeping these named the same way the
/// wireframes do makes it easy to keep the screens visually in sync.
class AppColors {
  AppColors._();

  static const paper = Color(0xFFFAF8F4);
  static const surface = Color(0xFFFFFFFF);
  static const ink = Color(0xFF23211D);
  static const ink2 = Color(0xFF938D82);
  static const ink3 = Color(0xFFC2BCB1);
  static const hair = Color(0xFFECE7DD);
  static const hair2 = Color(0xFFF2EEE6);
  static const fill = Color(0xFFF4F0E8);

  static const accent = Color(0xFFD9942B); // warm lamp light
  static const accentSoft = Color(0xFFFAEDD2);
  static const accentSoft2 = Color(0xFFFCF5E8);
  static const accentInk = Color(0xFF9A6412);

  static const cool = Color(0xFF6E89A6); // natural daylight
  static const coolSoft = Color(0xFFE9EEF3);
  static const coolInk = Color(0xFF3F5871);

  static const ok = Color(0xFF5E9B79);
  static const okSoft = Color(0xFFE5EFE8);
  static const off = Color(0xFFBCB6AB);

  static const danger = Color(0xFFB4543A);
  static const dangerSoft = Color(0xFFF6E7E2);
}

class AppTheme {
  static ThemeData get lightTheme {
    final base = ThemeData(
      useMaterial3: true,
      brightness: Brightness.light,
      scaffoldBackgroundColor: AppColors.paper,
      colorScheme: ColorScheme.fromSeed(
        seedColor: AppColors.accent,
        brightness: Brightness.light,
        primary: AppColors.accent,
        onPrimary: Colors.white,
        surface: AppColors.surface,
        onSurface: AppColors.ink,
      ),
      fontFamily: 'Plus Jakarta Sans',
      textTheme: const TextTheme(
        bodyMedium: TextStyle(color: AppColors.ink, letterSpacing: -0.1),
      ),
    );

    return base.copyWith(
      appBarTheme: const AppBarTheme(
        backgroundColor: AppColors.paper,
        foregroundColor: AppColors.ink,
        elevation: 0,
        centerTitle: true,
        surfaceTintColor: Colors.transparent,
        titleTextStyle: TextStyle(
          color: AppColors.ink,
          fontSize: 17,
          fontWeight: FontWeight.w700,
        ),
      ),
      cardTheme: CardThemeData(
        color: AppColors.surface,
        elevation: 0,
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(18),
          side: const BorderSide(color: AppColors.hair),
        ),
      ),
      inputDecorationTheme: InputDecorationTheme(
        filled: true,
        fillColor: AppColors.surface,
        contentPadding: const EdgeInsets.symmetric(horizontal: 14, vertical: 14),
        border: OutlineInputBorder(
          borderRadius: BorderRadius.circular(13),
          borderSide: const BorderSide(color: AppColors.hair),
        ),
        enabledBorder: OutlineInputBorder(
          borderRadius: BorderRadius.circular(13),
          borderSide: const BorderSide(color: AppColors.hair),
        ),
        focusedBorder: OutlineInputBorder(
          borderRadius: BorderRadius.circular(13),
          borderSide: const BorderSide(color: AppColors.accent, width: 1.4),
        ),
        hintStyle: const TextStyle(color: AppColors.ink2, fontWeight: FontWeight.w400),
      ),
      elevatedButtonTheme: ElevatedButtonThemeData(
        style: ElevatedButton.styleFrom(
          backgroundColor: AppColors.accent,
          foregroundColor: Colors.white,
          elevation: 0,
          minimumSize: const Size.fromHeight(50),
          shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(14)),
          textStyle: const TextStyle(fontSize: 15.5, fontWeight: FontWeight.w700),
        ),
      ),
      outlinedButtonTheme: OutlinedButtonThemeData(
        style: OutlinedButton.styleFrom(
          foregroundColor: AppColors.ink,
          side: const BorderSide(color: AppColors.hair),
          minimumSize: const Size.fromHeight(50),
          shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(14)),
          textStyle: const TextStyle(fontSize: 15.5, fontWeight: FontWeight.w700),
        ),
      ),
      dividerTheme: const DividerThemeData(color: AppColors.hair2, thickness: 1, space: 1),
    );
  }
}

/// Shared text style helpers (mirrors the wireframe's NUM/LBL helpers).
const numericStyle = TextStyle(
  fontFeatures: [FontFeature.tabularFigures()],
  letterSpacing: -0.4,
);

TextStyle labelStyle({Color color = AppColors.ink2, double fontSize = 11, FontWeight weight = FontWeight.w700}) {
  return TextStyle(
    color: color,
    fontSize: fontSize,
    fontWeight: weight,
    letterSpacing: 1.2,
  );
}
