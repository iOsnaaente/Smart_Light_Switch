import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../../core/routes/app_routes.dart';
import '../../core/theme/app_theme.dart';
import '../../providers/auth_provider.dart';
import '../../providers/device_provider.dart';

class LoginPage extends StatefulWidget {
  const LoginPage({super.key});

  @override
  State<LoginPage> createState() => _LoginPageState();
}

class _LoginPageState extends State<LoginPage> {
  final _usernameController = TextEditingController(text: 'admin');
  final _passwordController = TextEditingController(text: 'admin');
  bool _submitting = false;
  String? _error;

  @override
  void dispose() {
    _usernameController.dispose();
    _passwordController.dispose();
    super.dispose();
  }

  Future<void> _enter() async {
    setState(() {
      _submitting = true;
      _error = null;
    });

    final auth = context.read<AuthProvider>();
    final ok = await auth.login(_usernameController.text.trim(), _passwordController.text);
    if (!mounted) return;

    if (ok) {
      await context.read<DeviceProvider>().loadDevices();
      if (!mounted) return;
      Navigator.pushReplacementNamed(context, AppRoutes.devices);
    } else {
      setState(() {
        _submitting = false;
        _error = auth.error ?? 'Não foi possível entrar';
      });
    }
  }

  void _enterAsDemo() {
    context.read<AuthProvider>().enterDemoMode();
    context.read<DeviceProvider>().enableDemoMode();
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
                  _LoginField(label: 'USUÁRIO', placeholder: 'admin', controller: _usernameController),
                  const SizedBox(height: 14),
                  _LoginField(label: 'SENHA', placeholder: '••••••••', obscure: true, controller: _passwordController),
                  const SizedBox(height: 8),
                  const Align(
                    alignment: Alignment.centerRight,
                    child: Text('esqueci a senha', style: TextStyle(fontSize: 13, color: AppColors.accentInk, fontWeight: FontWeight.w600)),
                  ),
                  if (_error != null) ...[
                    const SizedBox(height: 14),
                    Text(_error!, textAlign: TextAlign.center, style: const TextStyle(fontSize: 13, color: AppColors.danger, fontWeight: FontWeight.w600)),
                  ],
                  const SizedBox(height: 22),
                  SizedBox(
                    width: double.infinity,
                    child: ElevatedButton(
                      onPressed: _submitting ? null : _enter,
                      child: _submitting
                          ? const SizedBox(
                              width: 20,
                              height: 20,
                              child: CircularProgressIndicator(strokeWidth: 2.4, color: Colors.white),
                            )
                          : const Text('Entrar'),
                    ),
                  ),
                  const SizedBox(height: 10),
                  SizedBox(
                    width: double.infinity,
                    child: OutlinedButton(
                      onPressed: _submitting ? null : _enterAsDemo,
                      child: const Text('Entrar como demonstração'),
                    ),
                  ),
                  const SizedBox(height: 18),
                  Text.rich(
                    TextSpan(
                      style: const TextStyle(fontSize: 12.5, color: AppColors.ink2, fontWeight: FontWeight.w500),
                      children: [
                        const TextSpan(text: 'usuário de teste do Backend: '),
                        TextSpan(text: 'admin / admin', style: TextStyle(color: AppColors.accentInk, fontWeight: FontWeight.w700)),
                      ],
                    ),
                    textAlign: TextAlign.center,
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
  final TextEditingController controller;

  const _LoginField({required this.label, required this.placeholder, required this.controller, this.obscure = false});

  @override
  Widget build(BuildContext context) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text(label, style: labelStyle(fontSize: 12)),
        const SizedBox(height: 7),
        TextField(
          controller: controller,
          obscureText: obscure,
          decoration: InputDecoration(hintText: placeholder),
        ),
      ],
    );
  }
}
