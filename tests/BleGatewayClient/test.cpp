#include <time.h>
/**
 * @file test.cpp
 * @brief Host tests for BleGatewayClient BLE NUS telemetry gateway.
 * @ingroup gateway_module
 * @date 2026-11-11
 * @version 1.0
 */

#include "BleGatewayClient.h"
#include "BleConnectivityDriver.h"
#include "TelemetryPackage.h"
#include <Arduino.h>
#include <BLE.h>
#include "JsonTelemetryWriter.h"
#include <cassert>
#include <cstring>
#include <string>

namespace {

/**
 * @brief Minimal telemetry payload for testing serialization.
 */
class TestTelemetryPayload : public TelemetryPackage {
protected:
    void write(TelemetryWriter& writer) const override {
        writer.field("temperature", 24.5f);
        writer.field("humidity", 65.0f);
    }
};

} // namespace

[[nodiscard]] int main() {
    // 1. Constructor initializes statistics to zero
    BLE::reset();
    {
        BleConnectivityDriver driver("TestBleDevice");
        BleGatewayClient gatewayClient(driver);

        auto stats = gatewayClient.getStatistics();
        assert(stats.sendsSucceeded == 0);
        assert(stats.sendsFailed == 0);
        assert(!gatewayClient.isConnected());
    }

    // 2. sendTelemetryRecord fails when BLE peer is not connected
    BLE::reset();
    {
        BleConnectivityDriver driver("TestBleDevice");
        driver.connect();
        BleGatewayClient gatewayClient(driver);

        TestTelemetryPayload payload;
        assert(!gatewayClient.sendTelemetryRecord(payload));

        auto stats = gatewayClient.getStatistics();
        assert(stats.sendsSucceeded == 0);
        assert(stats.sendsFailed == 1);
    }

    // 3. sendTelemetryRecord succeeds when BLE peer is connected
    BLE::reset();
    {
        BleConnectivityDriver driver("TestBleDevice");
        driver.connect();
        BleGatewayClient gatewayClient(driver);

        // Simulate central peer connection
        BLEServer& server = BLE::serverInstance;
        server.onConnectCallback(server, BLEConnInfo("test-conn"));
        assert(gatewayClient.isConnected());

        TestTelemetryPayload payload;
        assert(gatewayClient.sendTelemetryRecord(payload));

        auto stats = gatewayClient.getStatistics();
        assert(stats.sendsSucceeded == 1);
        assert(stats.sendsFailed == 0);

        // Verify the TX characteristic received the JSON payload with newline
        BLEService& service = server.services[0];
        BLECharacteristic& txChar = service.characteristics[0];
        std::string value = txChar.getStringValue();
        assert(value.find("\"temperature\"") != std::string::npos);
        assert(value.find("\"humidity\"") != std::string::npos);
        assert(value.back() == '\n'); // JSON lines framing
        assert(txChar.notifyCount == 1);
    }

    // 4. Multiple telemetry records increment statistics correctly
    BLE::reset();
    {
        BleConnectivityDriver driver("TestBleDevice");
        driver.connect();
        BleGatewayClient gatewayClient(driver);

        BLEServer& server = BLE::serverInstance;
        server.onConnectCallback(server, BLEConnInfo("test-conn"));

        TestTelemetryPayload payload;
        assert(gatewayClient.sendTelemetryRecord(payload));
        assert(gatewayClient.sendTelemetryRecord(payload));
        assert(gatewayClient.sendTelemetryRecord(payload));

        auto stats = gatewayClient.getStatistics();
        assert(stats.sendsSucceeded == 3);
        assert(stats.sendsFailed == 0);
    }

    // 5. service() restarts advertising when peer disconnected and not advertising
    BLE::reset();
    {
        BleConnectivityDriver driver("TestBleDevice");
        driver.connect();
        BleGatewayClient gatewayClient(driver);

        BLEServer& server = BLE::serverInstance;
        server.onConnectCallback(server, BLEConnInfo("test-conn"));

        // Simulate disconnection
        server.onDisconnectCallback(server, BLEConnInfo("test-conn"), 0x01);
        assert(driver.isAdvertising()); // Auto-restart by BleConnectivityDriver

        // Stop advertising manually to test service() behavior
        driver.stopAdvertising();
        assert(!driver.isAdvertising());

        // service() should restart advertising
        gatewayClient.service();
        assert(driver.isAdvertising());
    }

    // 6. loop() delegates to service()
    BLE::reset();
    {
        BleConnectivityDriver driver("TestBleDevice");
        driver.connect();
        BleGatewayClient gatewayClient(driver);

        BLEServer& server = BLE::serverInstance;
        server.onConnectCallback(server, BLEConnInfo("test-conn"));
        server.onDisconnectCallback(server, BLEConnInfo("test-conn"), 0x01);

        driver.stopAdvertising();
        gatewayClient.loop();
        assert(driver.isAdvertising());
    }

    // 7. isConnected() reflects BLE driver connection state
    BLE::reset();
    {
        BleConnectivityDriver driver("TestBleDevice");
        driver.connect();
        BleGatewayClient gatewayClient(driver);

        assert(!gatewayClient.isConnected());

        BLEServer& server = BLE::serverInstance;
        server.onConnectCallback(server, BLEConnInfo("test-conn"));
        assert(gatewayClient.isConnected());

        server.onDisconnectCallback(server, BLEConnInfo("test-conn"), 0x01);
        assert(!gatewayClient.isConnected());
    }

    // 8. getStatistics() returns accurate counters after mixed success/failure
    BLE::reset();
    {
        BleConnectivityDriver driver("TestBleDevice");
        driver.connect();
        BleGatewayClient gatewayClient(driver);

        TestTelemetryPayload payload;

        // First send fails (not connected)
        assert(!gatewayClient.sendTelemetryRecord(payload));

        // Connect peer
        BLEServer& server = BLE::serverInstance;
        server.onConnectCallback(server, BLEConnInfo("test-conn"));

        // Next two sends succeed
        assert(gatewayClient.sendTelemetryRecord(payload));
        assert(gatewayClient.sendTelemetryRecord(payload));

        // Disconnect peer
        server.onDisconnectCallback(server, BLEConnInfo("test-conn"), 0x01);

        // Next send fails (not connected)
        assert(!gatewayClient.sendTelemetryRecord(payload));

        auto stats = gatewayClient.getStatistics();
        assert(stats.sendsSucceeded == 2);
        assert(stats.sendsFailed == 2);
    }

    return 0;
}
