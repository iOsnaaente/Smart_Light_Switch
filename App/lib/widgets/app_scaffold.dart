import 'package:flutter/material.dart';

import '../core/theme/app_theme.dart';

/// Round, soft icon button used in app bars (mirrors the wireframe `IconBtn`).
class RoundIconButton extends StatelessWidget {
  final IconData? icon;
  final Widget? child;
  final VoidCallback? onTap;
  final Color? color;

  const RoundIconButton({super.key, this.icon, this.child, this.onTap, this.color})
      : assert(icon != null || child != null, 'provide either icon or child');

  @override
  Widget build(BuildContext context) {
    return Material(
      color: AppColors.surface,
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.circular(11),
        side: const BorderSide(color: AppColors.hair),
      ),
      child: InkWell(
        borderRadius: BorderRadius.circular(11),
        onTap: onTap,
        child: SizedBox(
          width: 34,
          height: 34,
          child: Center(child: child ?? Icon(icon, size: 17, color: color ?? AppColors.ink)),
        ),
      ),
    );
  }
}

/// Centered title + subtitle app bar with rounded icon buttons on either
/// side (mirrors the wireframe `AppBar`/`DeviceHead`).
class DeviceAppBar extends StatelessWidget implements PreferredSizeWidget {
  final String title;
  final String? subtitle;
  final Widget? leading;
  final Widget? trailing;

  const DeviceAppBar({super.key, required this.title, this.subtitle, this.leading, this.trailing});

  @override
  Size get preferredSize => const Size.fromHeight(56);

  @override
  Widget build(BuildContext context) {
    return SafeArea(
      child: Padding(
        padding: const EdgeInsets.fromLTRB(18, 8, 18, 6),
        child: Row(
          children: [
            SizedBox(width: 34, child: Align(alignment: Alignment.centerLeft, child: leading ?? const SizedBox.shrink())),
            Expanded(
              child: Column(
                mainAxisSize: MainAxisSize.min,
                children: [
                  Text(title, style: const TextStyle(fontSize: 17, fontWeight: FontWeight.w700)),
                  if (subtitle != null)
                    Padding(
                      padding: const EdgeInsets.only(top: 1),
                      child: Text(subtitle!, style: const TextStyle(fontSize: 11, color: AppColors.ink2, fontWeight: FontWeight.w500)),
                    ),
                ],
              ),
            ),
            SizedBox(width: 34, child: Align(alignment: Alignment.centerRight, child: trailing ?? const SizedBox.shrink())),
          ],
        ),
      ),
    );
  }
}

/// Bottom navigation bar styled to match the wireframe `TabBar`.
class SmartBottomNav extends StatelessWidget {
  final int currentIndex;
  final ValueChanged<int> onTap;

  const SmartBottomNav({super.key, required this.currentIndex, required this.onTap});

  static const _tabs = [
    (label: 'Controle', icon: Icons.wb_sunny_rounded),
    (label: 'Dispositivos', icon: Icons.grid_view_rounded),
    (label: 'Perfil', icon: Icons.person_outline_rounded),
  ];

  @override
  Widget build(BuildContext context) {
    return DecoratedBox(
      decoration: const BoxDecoration(
        color: AppColors.surface,
        border: Border(top: BorderSide(color: AppColors.hair)),
      ),
      child: SafeArea(
        top: false,
        child: Padding(
          padding: const EdgeInsets.symmetric(vertical: 9),
          child: Row(
            children: List.generate(_tabs.length, (i) {
              final tab = _tabs[i];
              final active = i == currentIndex;
              final color = active ? AppColors.accent : AppColors.ink3;
              final textColor = active ? AppColors.accentInk : AppColors.ink2;
              return Expanded(
                child: InkWell(
                  onTap: () => onTap(i),
                  child: Column(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      Icon(tab.icon, size: 19, color: color),
                      const SizedBox(height: 5),
                      Text(
                        tab.label,
                        style: TextStyle(fontSize: 9.5, fontWeight: active ? FontWeight.w700 : FontWeight.w500, color: textColor),
                      ),
                    ],
                  ),
                ),
              );
            }),
          ),
        ),
      ),
    );
  }
}

/// Section label with an optional trailing action (mirrors wireframe `Sub`).
class SectionLabel extends StatelessWidget {
  final String title;
  final String? action;
  final VoidCallback? onAction;

  const SectionLabel({super.key, required this.title, this.action, this.onAction});

  @override
  Widget build(BuildContext context) {
    return Row(
      mainAxisAlignment: MainAxisAlignment.spaceBetween,
      children: [
        Text(title.toUpperCase(), style: labelStyle()),
        if (action != null)
          GestureDetector(
            onTap: onAction,
            child: Text(action!, style: const TextStyle(fontSize: 13, color: AppColors.accentInk, fontWeight: FontWeight.w600)),
          ),
      ],
    );
  }
}

/// Settings row with title, optional value and chevron (mirrors `Row`).
class SettingsRow extends StatelessWidget {
  final String title;
  final String? value;
  final bool showChevron;
  final bool danger;
  final VoidCallback? onTap;

  const SettingsRow({
    super.key,
    required this.title,
    this.value,
    this.showChevron = true,
    this.danger = false,
    this.onTap,
  });

  @override
  Widget build(BuildContext context) {
    final color = danger ? AppColors.danger : AppColors.ink;
    return InkWell(
      onTap: onTap,
      child: Container(
        padding: const EdgeInsets.symmetric(vertical: 13, horizontal: 2),
        decoration: const BoxDecoration(border: Border(bottom: BorderSide(color: AppColors.hair2))),
        child: Row(
          children: [
            Container(
              width: 34,
              height: 34,
              decoration: BoxDecoration(
                color: danger ? AppColors.dangerSoft : AppColors.fill,
                borderRadius: BorderRadius.circular(11),
              ),
              child: Center(
                child: Container(
                  width: 7,
                  height: 7,
                  decoration: BoxDecoration(color: danger ? AppColors.danger : AppColors.ink2, shape: BoxShape.circle),
                ),
              ),
            ),
            const SizedBox(width: 13),
            Expanded(
              child: Text(title, style: TextStyle(fontSize: 14.5, fontWeight: FontWeight.w600, color: color)),
            ),
            if (value != null)
              Padding(
                padding: const EdgeInsets.only(right: 6),
                child: Text(value!, style: const TextStyle(fontSize: 12, color: AppColors.ink2, fontWeight: FontWeight.w500)),
              ),
            if (showChevron && !danger) const Icon(Icons.chevron_right_rounded, size: 18, color: AppColors.ink3),
          ],
        ),
      ),
    );
  }
}
