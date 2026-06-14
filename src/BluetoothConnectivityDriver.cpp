#include "BluetoothConnectivityDriver.h"

void BluetoothConnectivityDriver::connect() {
    // Start Bluetooth Serial with the device name
    serialBT.begin("Modest-IoT-Device");
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
    serialBT.println(data);
    return true;
}

bool BluetoothConnectivityDriver::canTransmit() {
    return connected;
}
