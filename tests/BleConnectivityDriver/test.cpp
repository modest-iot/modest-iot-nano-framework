#include <time.h>
#include "BleConnectivityDriver.h"
#include <Arduino.h>
#include <BLEDevice.h>
#include <cassert>
#include <cstring>
#include <string>

namespace {
std::string lastReceivedPayload;
size_t lastReceivedLen = 0;

void testDataReceivedCallback(const char* data, const size_t length) {
    lastReceivedPayload = std::string(data, length);
    lastReceivedLen = length;
}
} // namespace

[[nodiscard]] int main() {
    // 1. Initial State verification with default device name
    BLEDevice::reset();
    lastReceivedPayload.clear();
    lastReceivedLen = 0;

    {
        BleConnectivityDriver defaultDriver;
        assert(std::strcmp(defaultDriver.getDeviceName(), "Modest-IoT-Device") == 0);
        assert(!defaultDriver.isConnected());
        assert(!defaultDriver.canTransmit());
        assert(!defaultDriver.isAdvertising());

        // Transmit before connect() or connection should fail safely
        assert(!defaultDriver.transmit(nullptr, "test payload"));
        assert(!defaultDriver.transmitBytes(reinterpret_cast<const uint8_t*>("bytes"), 5));
    }

    // 2. Custom device name and connect() initialization sequence
    BLEDevice::reset();
    BleConnectivityDriver driver("Custom-ESP32-Node");
    assert(std::strcmp(driver.getDeviceName(), "Custom-ESP32-Node") == 0);

    driver.setReceivedDataCallback(testDataReceivedCallback);

    // Call connect() to initialize BLE stack and start advertising
    driver.connect();
    assert(BLEDevice::getInitName() == "Custom-ESP32-Node");
    assert(driver.isAdvertising());
    assert(BLEDevice::getAdvertising()->running);
    assert(!BLEDevice::getAdvertising()->serviceUuids.empty());
    assert(BLEDevice::getAdvertising()->serviceUuids[0] == BleConnectivityDriver::NUS_SERVICE_UUID);
    assert(!driver.isConnected());
    assert(!driver.canTransmit());

    BLEServer* server = BLEDevice::getServer();
    assert(server != nullptr);
    assert(!server->services.empty());
    BLEService* service = server->services[0];
    assert(service->uuid == BleConnectivityDriver::NUS_SERVICE_UUID);
    assert(service->started);
    assert(service->characteristics.size() == 2);

    BLECharacteristic* txChar = service->characteristics[0];
    BLECharacteristic* rxChar = service->characteristics[1];
    assert(txChar->uuid == BleConnectivityDriver::NUS_TX_CHARACTERISTIC_UUID);
    assert((txChar->properties & BLECharacteristic::PROPERTY_NOTIFY) != 0);

    assert(rxChar->uuid == BleConnectivityDriver::NUS_RX_CHARACTERISTIC_UUID);
    assert((rxChar->properties & BLECharacteristic::PROPERTY_WRITE) != 0);

    // 3. Simulate Central Peer Connection
    assert(server->callbacks != nullptr);
    server->callbacks->onConnect(server);
    assert(driver.isConnected());
    assert(driver.canTransmit());
    assert(!driver.isAdvertising());

    // 4. Test outbound transmission over TX characteristic
    assert(driver.transmit("any_target", "Hello BLE World!"));
    assert(txChar->getValue() == "Hello BLE World!");
    assert(txChar->notifyCount == 1);

    constexpr uint8_t rawBytes[] = {0x01, 0x02, 0xAA, 0xBB};
    assert(driver.transmitBytes(rawBytes, sizeof(rawBytes)));
    assert(txChar->getValue().size() == 4);
    assert(txChar->notifyCount == 2);

    // Transmit null or 0 length should fail gracefully
    assert(!driver.transmit("any_target", nullptr));
    assert(!driver.transmitBytes(nullptr, 10));
    assert(!driver.transmitBytes(rawBytes, 0));

    // 5. Test inbound data reception via RX characteristic
    assert(rxChar->callbacks != nullptr);
    rxChar->setValue("Command: TURN_ON");
    rxChar->callbacks->onWrite(rxChar);
    assert(lastReceivedPayload == "Command: TURN_ON");
    assert(lastReceivedLen == 16);

    // 6. Simulate Central Peer Disconnection
    server->callbacks->onDisconnect(server);
    assert(!driver.isConnected());
    assert(!driver.canTransmit());
    // Disconnection should automatically restart advertising
    assert(driver.isAdvertising());
    assert(BLEDevice::getAdvertising()->running);

    // Outbound transmit after disconnection must fail safely
    assert(!driver.transmit("any_target", "Data after disconnect"));

    // 7. Test manual advertising control
    driver.stopAdvertising();
    assert(!driver.isAdvertising());
    assert(!BLEDevice::getAdvertising()->running);

    driver.startAdvertising();
    assert(driver.isAdvertising());
    assert(BLEDevice::getAdvertising()->running);

    // 8. Test re-connect idempotency
    driver.connect();
    assert(driver.isAdvertising());

    return 0;
}
