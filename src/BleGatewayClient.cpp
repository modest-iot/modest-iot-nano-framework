/**
 * @file BleGatewayClient.cpp
 * @brief Implements a BLE NUS gateway client for serialized telemetry payloads.
 * @ingroup gateway_module
 * @copyright (c) 2025-2026 Angel Velasquez
 * @license Apache-2.0
 * @date 2026-11-11
 * @version 1.0
 */

#include "BleGatewayClient.h"
#include "BleConnectivityDriver.h"
#include "ModestLog.h"
#include <ArduinoJson.h>
// ReSharper disable once CppUnusedIncludeDirective
#include <cstring>

/** @name BLE Gateway Client Implementation */
//@{

BleGatewayClient::BleGatewayClient(ConnectivityDriver& transportDriver, const char* deviceName)
    : GatewayClient(transportDriver, nullptr),
      sendsSucceededCount(0),
      sendsFailedCount(0) {
    (void)deviceName; // Device name is configured in BleConnectivityDriver constructor
    memset(serializationBuffer, 0, SERIALIZATION_BUFFER_SIZE);
}

bool BleGatewayClient::sendTelemetryRecord(const TelemetryPackage& abstractTelemetryPayload) {
    if (!communicationTransportDriver.canTransmit()) {
        sendsFailedCount++;
        MODEST_LOG_W("BleGateway", "BLE peer not connected; telemetry dropped");
        return false;
    }

    // Serialize telemetry to fixed-size buffer using the same pattern as MqttGatewayClient
    JsonDocument payload;
    abstractTelemetryPayload.addEnvelopeMetadata(payload);
    abstractTelemetryPayload.serialize(payload);
    const size_t serializedLength = serializeJson(payload, serializationBuffer, sizeof(serializationBuffer));
    if (serializedLength == 0) {
        sendsFailedCount++;
        MODEST_LOG_W("BleGateway", "Payload too large for serialization buffer (%u bytes)", SERIALIZATION_BUFFER_SIZE);
        return false;
    }

    // Append newline terminator for JSON lines framing
    appendNewlineTerminator(serializationBuffer, serializedLength);

    // Send as GATT notification
    const bool success = communicationTransportDriver.transmit(nullptr, serializationBuffer);
    if (success) {
        sendsSucceededCount++;
    } else {
        sendsFailedCount++;
        MODEST_LOG_W("BleGateway", "BLE transmit failed; telemetry dropped");
    }

    return success;
}

void BleGatewayClient::service() {
    // BLE does not require keep-alive pings like MQTT.
    // Check if the peer disconnected and restart advertising if needed.
    BleConnectivityDriver& bleDriver = const_cast<BleConnectivityDriver&>(
        static_cast<const BleConnectivityDriver&>(communicationTransportDriver));
    if (!bleDriver.isConnected() && !bleDriver.isAdvertising()) {
        MODEST_LOG_I("BleGateway", "BLE peer disconnected; restarting advertising");
        bleDriver.startAdvertising();
    }
}

void BleGatewayClient::loop() {
    service();
}

GatewayStatistics BleGatewayClient::getStatistics() const {
    return {sendsSucceededCount, sendsFailedCount};
}

void BleGatewayClient::appendNewlineTerminator(char* buffer, const size_t currentLength) {
    if (currentLength < SERIALIZATION_BUFFER_SIZE - 1) {
        buffer[currentLength] = '\n';
        buffer[currentLength + 1] = '\0';
    }
}

//@} // BLE Gateway Client Implementation
