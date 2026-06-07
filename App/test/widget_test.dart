import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';

import 'package:smart_light_switch/api/api_client.dart';
import 'package:smart_light_switch/main.dart';

void main() {
  testWidgets('Login screen shows username and password fields', (WidgetTester tester) async {
    await tester.pumpWidget(SmartLightSwitchApp(apiClient: ApiClient()));

    expect(find.text('Smart Light Switch'), findsOneWidget);
    expect(find.widgetWithText(TextFormField, 'Usuário'), findsOneWidget);
    expect(find.widgetWithText(TextFormField, 'Senha'), findsOneWidget);
    expect(find.widgetWithText(FilledButton, 'Entrar'), findsOneWidget);
  });

  testWidgets('Login button shows error when credentials are rejected', (WidgetTester tester) async {
    await tester.pumpWidget(SmartLightSwitchApp(apiClient: _RejectingApiClient()));

    await tester.tap(find.widgetWithText(FilledButton, 'Entrar'));
    await tester.pumpAndSettle();

    expect(find.text('Usuário ou senha inválidos'), findsOneWidget);
  });
}

class _RejectingApiClient extends ApiClient {
  @override
  Future<bool> login(String username, String password) async => false;
}
