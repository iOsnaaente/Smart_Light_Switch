import 'package:flutter/material.dart';

import 'core/routes/app_routes.dart';
import 'core/theme/app_theme.dart';

void main() {
  runApp(const SmartLightApp());
}

class SmartLightApp extends StatelessWidget {
  const SmartLightApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Smart Light Switch',
      debugShowCheckedModeBanner: false,
      theme: AppTheme.lightTheme,
      initialRoute: AppRoutes.login,
      routes: AppRoutes.routes,
    );
  }
}
