import 'package:flutter_reactive_ble/flutter_reactive_ble.dart';

class BLEUtil {
  static final BLEUtil _bleUtil = BLEUtil._internal();

  final _ble = FlutterReactiveBle();

  factory BLEUtil() {
    return _bleUtil;
  }

  BLEUtil._internal();

  void scanAndConnect() {
    _ble.scanForDevices(withServices: []);
  }
}
