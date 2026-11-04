#include "MqttGatewayClient.h"
#include "WiFiConnectivityDriver.h"
#include "TelemetryPackage.h"
#include <ArduinoJson.h>
#include <cassert>
#include <cstdio>
#include <string>

namespace {

// Mock Transport Driver for testing gateway behavior in isolation
class MockConnectivityDriver : public ConnectivityDriver {
private:
    bool linkConnected = true;

public:
    int connectCalls = 0;

    void setLinkConnected(bool connected) {
        linkConnected = connected;
    }

    void connect() override {
        connectCalls++;
    }

    [[nodiscard]] bool isConnected() override {
        return linkConnected;
    }

    [[nodiscard]] bool canTransmit() override {
        return linkConnected;
    }

    bool transmit(const char* target, const char* data) override {
        (void)target;
        (void)data;
        return linkConnected;
    }
};

// Mock concrete telemetry package for testing serialization and dispatch
class SampleTelemetryPackage : public TelemetryPackage {
private:
    int sensorId;
    float readingValue;

public:
    SampleTelemetryPackage(int id, float value)
        : sensorId(id), readingValue(value) {}

    ~SampleTelemetryPackage() override = default;

    void serialize(JsonDocument& destination) const override {
        destination["id"] = sensorId;
        destination["value"] = readingValue;
    }
};

} // anonymous namespace

int main() {
    std::printf("Running MqttGatewayClient & WiFi Reconnection Tests...\n");

    // =========================================================================
    // Test 1: Successful Connection, Publishing, and Keep-Alive Servicing
    // =========================================================================
    {
        PubSubClient::resetMockState();
        fakeMillis = 1000;

        MockConnectivityDriver mockDriver;
        Client fakeSocket;
        MqttGatewayClient gateway(mockDriver, fakeSocket, "192.168.1.100", 1883, "telemetry/vitals", "Node-01");

        assert(!gateway.isConnected());
        assert(gateway.getClientState() == MQTT_DISCONNECTED);

        SampleTelemetryPackage package(42, 23.5f);
        bool sent = gateway.sendTelemetryRecord(package);

        assert(sent);
        assert(gateway.isConnected());
        assert(PubSubClient::connectAttemptCount == 1);
        assert(PubSubClient::lastClientId == "Node-01");
        assert(PubSubClient::publishCount == 1);
        assert(PubSubClient::lastTopic == "telemetry/vitals");
        assert(PubSubClient::lastPayload.find("\"id\":42") != std::string::npos);
        assert(PubSubClient::lastPayload.find("\"value\":23.5") != std::string::npos);
        assert(PubSubClient::loopCount == 1);

        // Verify service() / loop() executes keep-alive without publishing new data
        gateway.service();
        assert(PubSubClient::loopCount == 2);
        assert(PubSubClient::publishCount == 1); // No new publish

        gateway.loop();
        assert(PubSubClient::loopCount == 3);
        assert(PubSubClient::publishCount == 1);
    }

    // =========================================================================
    // Test 2: MQTT Reconnect Rate-Limiting & Backoff during Broker Outage
    // =========================================================================
    {
        PubSubClient::resetMockState();
        fakeMillis = 10000;

        MockConnectivityDriver mockDriver;
        Client fakeSocket;
        MqttGatewayClient gateway(mockDriver, fakeSocket, "192.168.1.100", 1883, "telemetry/vitals", "Node-02");
        gateway.setReconnectInterval(5000);
        assert(gateway.getReconnectInterval() == 5000);

        // Simulate broker refusal / timeout
        PubSubClient::simulateConnectFailure = true;

        SampleTelemetryPackage package(1, 10.0f);

        // First attempt at t = 10000ms: Should attempt connect and fail
        bool sent1 = gateway.sendTelemetryRecord(package);
        assert(!sent1);
        assert(PubSubClient::connectAttemptCount == 1);
        assert(!gateway.isConnected());

        // Second attempt at t = 11000ms (+1000ms, cooldown not elapsed): Must NOT attempt connect
        fakeMillis = 11000;
        bool sent2 = gateway.sendTelemetryRecord(package);
        assert(!sent2);
        assert(PubSubClient::connectAttemptCount == 1); // Connect count remains 1

        // Third attempt at t = 14999ms (+4999ms, cooldown not elapsed): Must NOT attempt connect
        fakeMillis = 14999;
        gateway.service();
        assert(PubSubClient::connectAttemptCount == 1);

        // Fourth attempt at t = 15001ms (+5001ms, cooldown elapsed): Should attempt connect
        fakeMillis = 15001;
        gateway.service();
        assert(PubSubClient::connectAttemptCount == 2); // Connect attempted again

        // Broker recovers at t = 20002ms
        fakeMillis = 20002;
        PubSubClient::simulateConnectFailure = false;
        bool sent3 = gateway.sendTelemetryRecord(package);
        assert(sent3);
        assert(PubSubClient::connectAttemptCount == 3);
        assert(gateway.isConnected());
        assert(PubSubClient::publishCount == 1);
    }

    // =========================================================================
    // Test 3: Transport Link Down Gating & Recovery Trigger
    // =========================================================================
    {
        PubSubClient::resetMockState();
        fakeMillis = 30000;

        MockConnectivityDriver mockDriver;
        mockDriver.setLinkConnected(false); // Transport is down
        Client fakeSocket;
        MqttGatewayClient gateway(mockDriver, fakeSocket, "192.168.1.100", 1883, "telemetry/status", "Node-03");

        SampleTelemetryPackage package(99, 0.0f);
        bool sent = gateway.sendTelemetryRecord(package);
        assert(!sent);
        assert(mockDriver.connectCalls == 1); // Triggered link recovery
        assert(PubSubClient::connectAttemptCount == 0); // Did not attempt broker connection while transport down

        // Service while transport down
        gateway.service();
        assert(mockDriver.connectCalls == 2);
        assert(PubSubClient::connectAttemptCount == 0);

        // Transport comes back up
        mockDriver.setLinkConnected(true);
        sent = gateway.sendTelemetryRecord(package);
        assert(sent);
        assert(PubSubClient::connectAttemptCount == 1);
        assert(PubSubClient::publishCount == 1);
    }

    // =========================================================================
    // Test 4: WiFiConnectivityDriver Reconnect Backoff & Re-Arming
    // =========================================================================
    {
        WiFiClass::resetMockState();
        fakeMillis = 50000;

        WiFiConnectivityDriver wifiDriver("TestSSID", "TestPassword");
        wifiDriver.setReconnectInterval(10000);
        assert(wifiDriver.getReconnectInterval() == 10000);

        WiFi.setStatus(WL_DISCONNECTED);
        assert(!wifiDriver.isConnected());
        assert(!wifiDriver.canTransmit());

        // First connect attempt at t = 50000ms: Should invoke WiFi.begin()
        wifiDriver.connect();
        assert(WiFiClass::beginCallCount == 1);
        assert(WiFiClass::lastBeginSsid == "TestSSID");
        assert(WiFiClass::lastBeginPassword == "TestPassword");

        // Immediate subsequent connect calls while disconnected at t = 52000ms: Should be throttled
        fakeMillis = 52000;
        wifiDriver.connect();
        assert(WiFiClass::beginCallCount == 1);

        fakeMillis = 59999;
        wifiDriver.connect();
        assert(WiFiClass::beginCallCount == 1);

        // Cooldown expires at t = 60001ms: Should re-trigger WiFi.begin()
        fakeMillis = 60001;
        wifiDriver.connect();
        assert(WiFiClass::beginCallCount == 2);

        // Wi-Fi successfully connects
        WiFi.setStatus(WL_CONNECTED);
        assert(wifiDriver.isConnected());
        assert(wifiDriver.canTransmit());

        // Subsequent connect() when already connected: No-op
        wifiDriver.connect();
        assert(WiFiClass::beginCallCount == 2);

        // Wi-Fi disconnects again at t = 80000ms
        WiFi.setStatus(WL_DISCONNECTED);
        assert(!wifiDriver.isConnected());
        fakeMillis = 80000;
        wifiDriver.connect();
        assert(WiFiClass::beginCallCount == 3);
    }

    std::printf("All MqttGatewayClient and WiFi Reconnection tests passed successfully!\n");
    return 0;
}
