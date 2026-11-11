/**
 * @file BleConnectivityDriver.cpp
 * @brief Implements BLE Nordic UART Service (NUS) connectivity driver.
 * @date 2026-06-06
 * @version 1.0
 */

#include "BleConnectivityDriver.h"

#if defined(CONFIG_BT_ENABLED) || defined(ARDUINO_ARCH_ESP32) || defined(TEST_MOCK_BLE)

#include <BLEDevice.h>
#include <BLEServer.h>
// ReSharper disable once CppClangTidyCERTerr52cpp
#include <cstring>

namespace {

class InternalBleServerCallbacks : public BLEServerCallbacks {
private:
    BleConnectivityDriver* driver;

public:
    explicit InternalBleServerCallbacks(BleConnectivityDriver* driverInstance)  : driver(driverInstance) {}

    void onConnect(BLEServer* /*pServer*/) override {
        if (driver != nullptr)
            driver->handlePeerConnected();
    }

    void onDisconnect(BLEServer* /*pServer*/) override {
        if (driver != nullptr)
            driver->handlePeerDisconnected();
    }
};

class InternalBleRxCallbacks : public BLECharacteristicCallbacks {
private:
    BleConnectivityDriver* driver;

public:
    explicit InternalBleRxCallbacks(BleConnectivityDriver* driverInstance)  : driver(driverInstance) {}

    void onWrite(BLECharacteristic* pCharacteristic) override {
        if (driver != nullptr && pCharacteristic != nullptr) {
            const uint8_t* rxData = pCharacteristic->getData();
            if (const size_t rxLength = pCharacteristic->getLength(); rxData != nullptr && rxLength > 0)
                driver->handleDataReceived(rxData, rxLength);
        }
    }
};

} // anonymous namespace

BleConnectivityDriver::BleConnectivityDriver(const char* deviceName)
    : localDeviceName(deviceName != nullptr ? deviceName : "Modest-IoT-Device"),
      deviceConnected(false),
      advertisingActive(false),
      initialized(false),
      bleServer(nullptr),
      bleService(nullptr),
      txCharacteristic(nullptr),
      rxCharacteristic(nullptr),
      onDataReceivedHandler(nullptr),
      serverCallbacksHandler(std::make_unique<InternalBleServerCallbacks>(this)),
      rxCallbacksHandler(std::make_unique<InternalBleRxCallbacks>(this))
{
}

BleConnectivityDriver::~BleConnectivityDriver() {
    stopAdvertising();
}

void BleConnectivityDriver::connect() {
    if (!initialized) {
        BLEDevice::init(localDeviceName);

        bleServer = BLEDevice::createServer();
        bleServer->setCallbacks(serverCallbacksHandler.get());

        bleService = bleServer->createService(NUS_SERVICE_UUID);

        // TX Characteristic (Notify) - Peripheral to Central
        txCharacteristic = bleService->createCharacteristic(NUS_TX_CHARACTERISTIC_UUID,
            BLECharacteristic::PROPERTY_NOTIFY);

        // RX Characteristic (Write / Write without response) - Central to Peripheral
        rxCharacteristic = bleService->createCharacteristic(NUS_RX_CHARACTERISTIC_UUID,
            BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
        );
        rxCharacteristic->setCallbacks(rxCallbacksHandler.get());

        bleService->start();
        initialized = true;
    }

    startAdvertising();
}

bool BleConnectivityDriver::isConnected() {
    return deviceConnected;
}

bool BleConnectivityDriver::canTransmit() {
    return isConnected() && txCharacteristic != nullptr;
}

bool BleConnectivityDriver::transmit(const char* /*target*/, const char* data) {
    if (data == nullptr || !canTransmit())
        return false;

    return transmitBytes(reinterpret_cast<const uint8_t*>(data), std::strlen(data));
}

bool BleConnectivityDriver::transmitBytes(const uint8_t* payload, const size_t length) {
    if (payload == nullptr || length == 0 || !canTransmit())
        return false;

    txCharacteristic->setValue(const_cast<uint8_t*>(payload), length);
    txCharacteristic->notify();
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

    if (BLEAdvertising* pAdvertising = BLEDevice::getAdvertising(); pAdvertising != nullptr) {
        pAdvertising->addServiceUUID(NUS_SERVICE_UUID);
        pAdvertising->setScanResponse(true);
        pAdvertising->setMinPreferred(0x06);
        pAdvertising->setMinPreferred(0x12);
        BLEDevice::startAdvertising();
        advertisingActive = true;
    }
}

void BleConnectivityDriver::stopAdvertising() {
    if (advertisingActive) {
        if (BLEAdvertising* pAdvertising = BLEDevice::getAdvertising(); pAdvertising != nullptr)
            pAdvertising->stop();
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
