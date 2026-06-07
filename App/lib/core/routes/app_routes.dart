import 'package:flutter/material.dart';

import '../../screens/login/login_page.dart';
import '../../screens/dashboard/devices_page.dart';
import '../../screens/control/control_page.dart';
import '../../screens/settings/device_settings_page.dart';

class AppRoutes {
  static const login = '/';
  static const devices = '/devices';
  static const control = '/control';
  static const settings = '/settings';

  static Map<String, WidgetBuilder> get routes => {
        login: (_) => const LoginPage(),
        devices: (_) => const DevicesPage(),
        control: (_) => const ControlPage(),
        settings: (_) => const DeviceSettingsPage(),
      };
}
