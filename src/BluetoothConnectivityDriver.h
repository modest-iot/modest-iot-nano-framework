#ifndef BLUETOOTH_CONNECTIVITY_DRIVER_H
#define BLUETOOTH_CONNECTIVITY_DRIVER_H

#include "ConnectivityDriver.h"
#include <BluetoothSerial.h>

/**
 * @brief Bluetooth implementation of the ConnectivityDriver.
 * @ingroup connectivity_module
 */
class BluetoothConnectivityDriver : public ConnectivityDriver {
private:
    BluetoothSerial serialBT;
    bool connected = false;

public:
    BluetoothConnectivityDriver() = default;
    ~BluetoothConnectivityDriver() override = default;

    void connect() override;
    bool isConnected() override;
    bool transmit(const char* target, const char* data) override;
    bool canTransmit() override;
};

#endif // BLUETOOTH_CONNECTIVITY_DRIVER_H
