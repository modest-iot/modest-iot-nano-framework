/**
 * @file MqttGatewayClient.cpp
 * @brief Implements MQTT broker session handling and telemetry publish operations.
 * @date 2026-06-06
 * @version 1.0
 */

#include "MqttGatewayClient.h"
#include <ArduinoJson.h>

MqttGatewayClient::MqttGatewayClient(ConnectivityDriver& transportDriver, Client& networkClient, 
                                     const char* brokerIpAddress, uint16_t brokerPort, 
                                     const char* topic, const char* deviceId)
    : GatewayClient(transportDriver, ""), // We don't use serviceEndpointUrl here, so pass ""
      mqttClient(networkClient),
      clientIdentifier(deviceId),
      publishTopic(topic) 
{
    mqttClient.setServer(brokerIpAddress, brokerPort);
}

void MqttGatewayClient::ensureBrokerSessionConnection() {
    // If the network interface state is broken, do not attempt a broker connection
    if (!communicationTransportDriver.canTransmit()) {
        return;
    }

    // Keep loop non-blocking inside the NetworkWorker thread context
    if (!mqttClient.connected()) {
        mqttClient.connect(clientIdentifier);
    }
}

bool MqttGatewayClient::sendTelemetryRecord(const TelemetryPackage& abstractTelemetryPayload) {
    // 1. Guard capability status check before spending memory processing power
    if (!communicationTransportDriver.canTransmit()) {
        communicationTransportDriver.connect(); // Asynchronously attempt a link recovery
        return false;
    }

    // 2. Assure our application layer MQTT socket session state is established
    ensureBrokerSessionConnection();
    if (!mqttClient.connected()) {
        return false;
    }

    // 3. Execute payload serialization polymorphically
    String serializedPayloadOutput = abstractTelemetryPayload.serializeAsString();

    // 4. Publish telemetry payload onto the broker topic ring (Executed on priority 1 thread)
    bool publishSuccessStatus = mqttClient.publish(publishTopic, serializedPayloadOutput.c_str());
    
    // Maintain internal background MQTT processing states (pings, heartbeats)
    mqttClient.loop();

    return publishSuccessStatus;
}