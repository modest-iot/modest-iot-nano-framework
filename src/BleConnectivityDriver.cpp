/**
 * @file BleConnectivityDriver.cpp
 * @brief Implements BLE Nordic UART Service (NUS) connectivity driver.
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-06-06
 * @version 1.0
 */

#include "BleConnectivityDriver.h"

#if defined(CONFIG_BT_ENABLED) || defined(ARDUINO_ARCH_ESP32) || defined(TEST_MOCK_BLE)

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
        bleHalBegin(localDeviceName);

        bleServer = bleHalCreateServer();
        bleServer.onConnect([this]() {
            handlePeerConnected();
        });
        bleServer.onDisconnect([this](uint8_t reason) {
            (void)reason;
            handlePeerDisconnected();
        });

        bleService = bleServer.createService(NUS_SERVICE_UUID);

        // TX Characteristic (Notify) - Peripheral to Central
        txCharacteristic = bleService.createCharacteristic(NUS_TX_CHARACTERISTIC_UUID,
            static_cast<uint32_t>(BleHalProperty::Notify),
            static_cast<uint16_t>(BleHalPermission::None));

        // RX Characteristic (Write / Write without response) - Central to Peripheral
        rxCharacteristic = bleService.createCharacteristic(NUS_RX_CHARACTERISTIC_UUID,
            static_cast<uint32_t>(BleHalProperty::Write | BleHalProperty::WriteWithoutRsp),
            static_cast<uint16_t>(BleHalPermission::WriteOpen)
        );
        rxCharacteristic.onWrite([this](const uint8_t* data, size_t length) {
            if (data != nullptr && length > 0) {
                handleDataReceived(data, length);
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

    BleHalAdvertising advertising = bleHalGetAdvertising();
    advertising.addServiceUUID(NUS_SERVICE_UUID);
    advertising.setScanResponse(true);
    advertising.start();
    advertisingActive = true;
}

void BleConnectivityDriver::stopAdvertising() {
    if (advertisingActive) {
        BleHalAdvertising advertising = bleHalGetAdvertising();
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
