import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import 'api/api_client.dart';
import 'core/routes/app_routes.dart';
import 'core/theme/app_theme.dart';
import 'providers/auth_provider.dart';
import 'providers/device_provider.dart';

void main() {
  runApp(const SmartLightApp());
}

class SmartLightApp extends StatelessWidget {
  /// Overrides the [ApiClient] used by the provider tree — lets widget tests
  /// substitute a fake client without making real HTTP calls.
  final ApiClient? apiClient;

  const SmartLightApp({super.key, this.apiClient});

  @override
  Widget build(BuildContext context) {
    return MultiProvider(
      providers: [
        Provider<ApiClient>(create: (_) => apiClient ?? ApiClient()),
        ChangeNotifierProvider<AuthProvider>(
          create: (context) => AuthProvider(context.read<ApiClient>()),
        ),
        ChangeNotifierProvider<DeviceProvider>(
          create: (context) => DeviceProvider(context.read<ApiClient>()),
        ),
      ],
      child: MaterialApp(
        title: 'Smart Light Switch',
        debugShowCheckedModeBanner: false,
        theme: AppTheme.lightTheme,
        initialRoute: AppRoutes.login,
        routes: AppRoutes.routes,
      ),
    );
  }
}
