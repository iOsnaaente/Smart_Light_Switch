import 'package:flutter/foundation.dart';

import '../api/api_client.dart';

enum AuthStatus { signedOut, authenticated, demo }

/// Owns the session shared across the app: wraps [ApiClient] for real logins
/// against the Backend and also supports an offline "demo" mode (no Backend
/// required) so the system can always be presented end to end.
class AuthProvider extends ChangeNotifier {
  final ApiClient api;

  AuthStatus _status = AuthStatus.signedOut;
  bool _loading = false;
  String? _error;

  AuthProvider(this.api);

  AuthStatus get status => _status;
  bool get isAuthenticated => _status != AuthStatus.signedOut;
  bool get isDemo => _status == AuthStatus.demo;
  bool get loading => _loading;
  String? get error => _error;
  String? get username => isDemo ? 'demo' : api.username;

  Future<bool> login(String username, String password) async {
    _loading = true;
    _error = null;
    notifyListeners();
    try {
      final ok = await api.login(username, password);
      _status = ok ? AuthStatus.authenticated : AuthStatus.signedOut;
      _error = ok ? null : 'Usuário ou senha inválidos';
      return ok;
    } catch (e) {
      _status = AuthStatus.signedOut;
      _error = e.toString();
      return false;
    } finally {
      _loading = false;
      notifyListeners();
    }
  }

  /// Enters the app without talking to the Backend — [DeviceProvider] mirrors
  /// this by switching to its in-memory demo dataset.
  void enterDemoMode() {
    _status = AuthStatus.demo;
    _error = null;
    notifyListeners();
  }

  void logout() {
    api.logout();
    _status = AuthStatus.signedOut;
    _error = null;
    notifyListeners();
  }
}
