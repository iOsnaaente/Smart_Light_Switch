import 'package:flutter/material.dart';

class DevicesPage extends StatelessWidget {
  const DevicesPage({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Dispositivos'),
      ),
      body: ListView(
        children: [
          ListTile(
            title: const Text('Mesa de Trabalho'),
            subtitle: const Text('Sala'),
            onTap: () {
              Navigator.pushNamed(
                context,
                '/control',
              );
            },
          ),
        ],
      ),
    );
  }
}
