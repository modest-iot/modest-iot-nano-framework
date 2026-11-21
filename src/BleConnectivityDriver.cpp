/**
 * @file BleConnectivityDriver.cpp
 * @brief Implements BLE Nordic UART Service (NUS) connectivity driver.
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-06-06
 * @version 1.0
 */

#include "BleConnectivityDriver.h"

#if defined(CONFIG_BT_ENABLED) || defined(ARDUINO_ARCH_ESP32) || defined(TEST_MOCK_BLE)

#if defined(TEST_MOCK_BLE)
#include "BLE.h"
#else
#include <BLE.h>
#endif
// ReSharper disable once CppClangTidyCERTerr52cpp
#include <cstring>

BleConnectivityDriver::BleConnectivityDriver(const char* deviceName)
    : localDeviceName(deviceName != nullptr ? deviceName : "Modest-IoT-Device"),
      deviceConnected(false),
      advertisingActive(false),
      initialized(false),
      onDataReceivedHandler(nullptr)
{
}

BleConnectivityDriver::~BleConnectivityDriver() {
    stopAdvertising();
}

void BleConnectivityDriver::connect() {
    if (!initialized) {
        BLE.begin(localDeviceName);

        // ESP32 4.0.0 BLE API returns value types wrapping std::shared_ptr<Impl>.
        // Storing by value is cheap and safe — the shared_ptr ensures lifetime.
        bleServer = BLE.createServer();
        bleServer.onConnect([this](BLEServer &, const BLEConnInfo &) {
            handlePeerConnected();
        });
        bleServer.onDisconnect([this](BLEServer &, const BLEConnInfo &, uint8_t) {
            handlePeerDisconnected();
        });

        bleService = bleServer.createService(NUS_SERVICE_UUID);

        // TX Characteristic (Notify) - Peripheral to Central
        txCharacteristic = bleService.createCharacteristic(NUS_TX_CHARACTERISTIC_UUID,
            BLEProperty::Notify, BLEPermission::None);

        // RX Characteristic (Write / Write without response) - Central to Peripheral
        rxCharacteristic = bleService.createCharacteristic(NUS_RX_CHARACTERISTIC_UUID,
            BLEProperty::Write | BLEProperty::WriteNR, BLEPermission::WriteOpen
        );
        rxCharacteristic.onWrite([this](BLECharacteristic &chr, const BLEConnInfo &) {
            size_t rxLength = 0;
            const uint8_t* rxData = chr.getValue(&rxLength);
            if (rxData != nullptr && rxLength > 0) {
                handleDataReceived(rxData, rxLength);
            }
        });

        bleServer.start();
        initialized = true;
    }

    startAdvertising();
}

bool BleConnectivityDriver::isConnected() {
    return deviceConnected;
}

bool BleConnectivityDriver::canTransmit() {
    return isConnected();
}

bool BleConnectivityDriver::transmit(const char* /*target*/, const char* data) {
    if (data == nullptr || !canTransmit())
        return false;

    return transmitBytes(reinterpret_cast<const uint8_t*>(data), std::strlen(data));
}

bool BleConnectivityDriver::transmitBytes(const uint8_t* payload, const size_t length) {
    if (payload == nullptr || length == 0 || !canTransmit())
        return false;

    txCharacteristic.setValue(payload, length);
    txCharacteristic.notify();
    return true;
}

const char* BleConnectivityDriver::getDeviceName() const {
    return localDeviceName;
}

bool BleConnectivityDriver::isAdvertising() const {
    return advertisingActive;
}

void BleConnectivityDriver::startAdvertising() {
    if (!initialized)
        return;

    BLEAdvertising advertising = BLE.getAdvertising();
    advertising.addServiceUUID(NUS_SERVICE_UUID);
    advertising.setScanResponse(true);
    advertising.start();
    advertisingActive = true;
}

void BleConnectivityDriver::stopAdvertising() {
    if (advertisingActive) {
        BLEAdvertising advertising = BLE.getAdvertising();
        advertising.stop();
        advertisingActive = false;
    }
}

void BleConnectivityDriver::setReceivedDataCallback(const DataReceivedCallback callback) {
    onDataReceivedHandler = callback;
}

void BleConnectivityDriver::handlePeerConnected() {
    deviceConnected = true;
    advertisingActive = false;
}

void BleConnectivityDriver::handlePeerDisconnected() {
    deviceConnected = false;
    // Automatically restart advertising upon disconnection so device remains discoverable
    startAdvertising();
}

void BleConnectivityDriver::disconnect() {
    stopAdvertising();
    deviceConnected = false;
}

// ReSharper disable once CppMemberFunctionMayBeStatic
int32_t BleConnectivityDriver::getSignalStrengthInDbm() const {
    // BLE server mode does not expose RSSI via the Arduino ESP32 BLE API.
    // RSSI is only available during scanning (BLEAdvertisedDevice::getRSSI())
    // or from a BLE client perspective (BLEClient::getRssi()).
    // Return 0 to indicate unavailable.
    return 0;
}

void BleConnectivityDriver::handleDataReceived(const uint8_t* data, const size_t length) const {
    if (onDataReceivedHandler != nullptr && data != nullptr && length > 0)
        onDataReceivedHandler(reinterpret_cast<const char*>(data), length);
}

#endif // CONFIG_BT_ENABLED || ARDUINO_ARCH_ESP32 || TEST_MOCK_BLE
