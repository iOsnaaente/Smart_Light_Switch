import 'package:flutter/material.dart';

class StatusChip extends StatelessWidget {
  final bool online;

  const StatusChip({
    super.key,
    required this.online,
  });

  @override
  Widget build(BuildContext context) {
    return Chip(
      label: Text(
        online ? 'ONLINE' : 'OFFLINE',
      ),
    );
  }
}
