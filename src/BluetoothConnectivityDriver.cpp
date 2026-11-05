#include "BluetoothConnectivityDriver.h"

#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BLUEDROID_ENABLED) // NOLINT(readability-identifier-naming)

void BluetoothConnectivityDriver::connect() {
    // Start Bluetooth Serial with the device name
    bluetoothSerial.begin("Modest-IoT-Device");
    connected = true;
}

bool BluetoothConnectivityDriver::isConnected() {
    // BluetoothSerial status check (simplified)
    return connected;
}

bool BluetoothConnectivityDriver::transmit(const char* target, const char* data) {
    if (!connected) {
        return false;
    }
    // BluetoothSerial can act as a Print stream;
    // target is currently unused for Bluetooth (assumed single peer connection),
    // but the signature requires it.
    bluetoothSerial.println(data);
    return true;
}

bool BluetoothConnectivityDriver::canTransmit() {
    return connected;
}

#endif // CONFIG_BT_ENABLED && CONFIG_BLUEDROID_ENABLED // NOLINT(readability-identifier-naming)
