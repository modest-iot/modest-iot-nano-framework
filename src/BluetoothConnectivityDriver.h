#ifndef BLUETOOTH_CONNECTIVITY_DRIVER_H
#define BLUETOOTH_CONNECTIVITY_DRIVER_H

#include "ConnectivityDriver.h"

#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BLUEDROID_ENABLED)
#include <BluetoothSerial.h>

/**
 * @brief Bluetooth Classic SPP implementation of the ConnectivityDriver.
 * @ingroup connectivity_module
 * @deprecated BluetoothSerial is deprecated in ESP32 Core 3.0+ and unsupported on ESP32-S3/C3/C6/H2. Use BleConnectivityDriver instead.
 */
class [[deprecated("BluetoothSerial is deprecated in ESP32 Core 3.0+. Use BleConnectivityDriver.")]] BluetoothConnectivityDriver : public ConnectivityDriver {
private:
    BluetoothSerial bluetoothSerial;
    bool connected = false;

public:
    BluetoothConnectivityDriver() = default;
    ~BluetoothConnectivityDriver() override = default;

    void connect() override;
    bool isConnected() override;
    bool transmit(const char* target, const char* data) override;
    bool canTransmit() override;
};

#endif // CONFIG_BT_ENABLED && CONFIG_BLUEDROID_ENABLED

#endif // BLUETOOTH_CONNECTIVITY_DRIVER_H
