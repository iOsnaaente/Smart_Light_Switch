import 'package:flutter/material.dart';

import '../../core/routes/app_routes.dart';
import '../../core/theme/app_theme.dart';

class LoginPage extends StatelessWidget {
  const LoginPage({super.key});

  void _enterAsDemo(BuildContext context) {
    Navigator.pushReplacementNamed(context, AppRoutes.devices);
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: AppColors.paper,
      body: SafeArea(
        child: Padding(
          padding: const EdgeInsets.symmetric(horizontal: 24),
          child: Center(
            child: ConstrainedBox(
              constraints: const BoxConstraints(maxWidth: 360),
              child: Column(
                mainAxisSize: MainAxisSize.min,
                children: [
                  const _Logo(),
                  const SizedBox(height: 8),
                  const Text(
                    'controle seu interruptor inteligente',
                    textAlign: TextAlign.center,
                    style: TextStyle(fontSize: 13.5, color: AppColors.ink2, fontWeight: FontWeight.w500),
                  ),
                  const SizedBox(height: 30),
                  const _LoginField(label: 'E-MAIL', placeholder: 'voce@email.com'),
                  const SizedBox(height: 14),
                  const _LoginField(label: 'SENHA', placeholder: '••••••••', obscure: true),
                  const SizedBox(height: 8),
                  Align(
                    alignment: Alignment.centerRight,
                    child: Text('esqueci a senha', style: TextStyle(fontSize: 13, color: AppColors.accentInk, fontWeight: FontWeight.w600)),
                  ),
                  const SizedBox(height: 26),
                  SizedBox(
                    width: double.infinity,
                    child: ElevatedButton(
                      onPressed: () => _enterAsDemo(context),
                      child: const Text('Entrar'),
                    ),
                  ),
                  const SizedBox(height: 10),
                  SizedBox(
                    width: double.infinity,
                    child: OutlinedButton(
                      onPressed: () => _enterAsDemo(context),
                      child: const Text('Entrar como demonstração'),
                    ),
                  ),
                  const SizedBox(height: 22),
                  Text.rich(
                    TextSpan(
                      style: const TextStyle(fontSize: 13.5, color: AppColors.ink2, fontWeight: FontWeight.w500),
                      children: [
                        const TextSpan(text: 'novo aqui? '),
                        TextSpan(text: 'criar conta', style: TextStyle(color: AppColors.accentInk, fontWeight: FontWeight.w700)),
                      ],
                    ),
                  ),
                ],
              ),
            ),
          ),
        ),
      ),
    );
  }
}

class _Logo extends StatelessWidget {
  const _Logo();

  @override
  Widget build(BuildContext context) {
    const size = 56.0;
    return Column(
      children: [
        Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            Container(
              width: size,
              height: size,
              decoration: BoxDecoration(
                borderRadius: BorderRadius.circular(size * 0.3),
                gradient: const LinearGradient(
                  begin: Alignment.topLeft,
                  end: Alignment.bottomRight,
                  colors: [AppColors.accent, Color(0xFFC57F1E)],
                ),
                boxShadow: const [BoxShadow(color: Color(0x60D9942B), blurRadius: 18, offset: Offset(0, 6))],
              ),
              child: Center(
                child: Container(
                  width: size * 0.42,
                  height: size * 0.42,
                  decoration: BoxDecoration(
                    color: Colors.white.withValues(alpha: 0.95),
                    shape: BoxShape.circle,
                    border: Border.all(color: Colors.white.withValues(alpha: 0.3), width: 4),
                  ),
                ),
              ),
            ),
            const SizedBox(width: 10),
            const Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text('SmartLight', style: TextStyle(fontSize: 26, fontWeight: FontWeight.w800, letterSpacing: -0.5)),
                SizedBox(height: 2),
                Text('luz que se ajusta', style: TextStyle(fontSize: 11.5, color: AppColors.ink2, fontWeight: FontWeight.w500)),
              ],
            ),
          ],
        ),
      ],
    );
  }
}

class _LoginField extends StatelessWidget {
  final String label;
  final String placeholder;
  final bool obscure;

  const _LoginField({required this.label, required this.placeholder, this.obscure = false});

  @override
  Widget build(BuildContext context) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text(label, style: labelStyle(fontSize: 12)),
        const SizedBox(height: 7),
        TextField(
          obscureText: obscure,
          decoration: InputDecoration(hintText: placeholder),
        ),
      ],
    );
  }
}
