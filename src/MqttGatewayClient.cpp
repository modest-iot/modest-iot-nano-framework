/**
 * @file MqttGatewayClient.cpp
 * @brief Implements MQTT broker session handling and telemetry publish operations.
 * @date 2026-06-06
 * @version 1.0
 */

#include "MqttGatewayClient.h"
#include "ModestLog.h"

MqttGatewayClient::MqttGatewayClient(ConnectivityDriver& transportDriver, Client& networkClient, 
                                     const char* brokerIpAddress, uint16_t brokerPort, 
                                     const char* topic, const char* deviceId)
    : GatewayClient(transportDriver, ""), // We don't use serviceEndpointUrl here, so pass ""
      mqttClient(networkClient),
      clientIdentifier(deviceId),
      publishTopic(topic),
      lastReconnectAttemptTimestamp(0),
      reconnectIntervalMilliseconds(5000)
{
    mqttClient.setServer(brokerIpAddress, brokerPort);
}

void MqttGatewayClient::ensureBrokerSessionConnection() {
    // If the network interface state is broken, do not attempt a broker connection
    if (!communicationTransportDriver.canTransmit()) {
        return;
    }

    if (mqttClient.connected()) {
        return;
    }

    // Rate-limit connection attempts to avoid blocking the network task during outages
    unsigned long currentTimestamp = millis();
    if (lastReconnectAttemptTimestamp == 0 || 
        (currentTimestamp - lastReconnectAttemptTimestamp >= reconnectIntervalMilliseconds)) {
        lastReconnectAttemptTimestamp = currentTimestamp;
        MODEST_LOG_I("MQTT", "Attempting connection to broker for client ID: %s", clientIdentifier);

        if (mqttClient.connect(clientIdentifier)) {
            MODEST_LOG_I("MQTT", "Connected to MQTT broker successfully.");
        } else {
            MODEST_LOG_W("MQTT", "Failed to connect to MQTT broker, state: %d", mqttClient.state());
        }
    }
}

void MqttGatewayClient::service() {
    if (!communicationTransportDriver.canTransmit()) {
        communicationTransportDriver.connect();
        return;
    }

    ensureBrokerSessionConnection();
    if (mqttClient.connected()) {
        mqttClient.loop();
    }
}

void MqttGatewayClient::loop() {
    service();
}

bool MqttGatewayClient::isConnected() {
    return communicationTransportDriver.isConnected() && mqttClient.connected();
}

int MqttGatewayClient::getClientState() {
    return mqttClient.state();
}

void MqttGatewayClient::setReconnectInterval(unsigned long intervalMs) {
    reconnectIntervalMilliseconds = intervalMs;
}

unsigned long MqttGatewayClient::getReconnectInterval() const {
    return reconnectIntervalMilliseconds;
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
    if (!publishSuccessStatus) {
        MODEST_LOG_W("MQTT", "Failed to publish telemetry to topic: %s", publishTopic);
    } else {
        MODEST_LOG_D("MQTT", "Published telemetry payload to topic: %s", publishTopic);
    }
    
    // Maintain internal background MQTT processing states (pings, heartbeats)
    mqttClient.loop();

    return publishSuccessStatus;
}