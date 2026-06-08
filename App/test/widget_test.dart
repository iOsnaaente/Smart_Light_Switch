import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';

import 'package:smart_light_switch/api/api_client.dart';
import 'package:smart_light_switch/main.dart';

void main() {
  testWidgets('Login screen shows username and password fields', (WidgetTester tester) async {
    await tester.pumpWidget(SmartLightApp(apiClient: _RejectingApiClient()));
    await tester.pumpAndSettle();

    expect(find.text('SmartLight'), findsOneWidget);
    expect(find.text('USUÁRIO'), findsOneWidget);
    expect(find.text('SENHA'), findsOneWidget);
    expect(find.byType(TextField), findsNWidgets(2));
    expect(find.widgetWithText(ElevatedButton, 'Entrar'), findsOneWidget);
  });

  testWidgets('Login button shows error when credentials are rejected', (WidgetTester tester) async {
    await tester.pumpWidget(SmartLightApp(apiClient: _RejectingApiClient()));
    await tester.pumpAndSettle();

    await tester.tap(find.widgetWithText(ElevatedButton, 'Entrar'));
    await tester.pumpAndSettle();

    expect(find.text('Usuário ou senha inválidos'), findsOneWidget);
  });
}

class _RejectingApiClient extends ApiClient {
  @override
  Future<bool> login(String username, String password) async => false;
}
