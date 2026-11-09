/**
 * @file MqttGatewayClient.cpp
 * @brief Implements MQTT broker session handling and telemetry publish operations.
 * @date 2026-06-06
 * @version 1.0
 */

#include "MqttGatewayClient.h"
#include "ModestLog.h"
#include <ArduinoJson.h>   // W15: JSON parsing for incoming command payloads

// W15: Static pointer for callback access (set by constructor)
static MqttGatewayClient* gActiveMqttClient = nullptr;

MqttGatewayClient::MqttGatewayClient(ConnectivityDriver& transportDriver, Client& networkClient, 
                                     const char* brokerIpAddress, const uint16_t brokerPort,
                                     const char* publishT, const char* subscribeT,
                                     Device* targetDev,
                                     const char* deviceId)
    : GatewayClient(transportDriver, ""), // We don't use serviceEndpointUrl here, so pass ""
      mqttClient(networkClient),
      clientIdentifier(deviceId),
      publishTopic(publishT),
      subscribeTopic(subscribeT),
      lastReconnectAttemptTimestamp(0),
      reconnectIntervalMilliseconds(DEFAULT_RECONNECT_INTERVAL_MILLISECONDS),
      sendsSucceededCount(0),
      sendsFailedCount(0),
      firstFailureLogged(false),
      serializationBuffer{},
      targetDevice(targetDev),
      subscribePayloadBuffer{}
{
    mqttClient.setServer(brokerIpAddress, brokerPort);
    
    // Register the static callback if subscribe is enabled
    if (subscribeTopic != nullptr) {
        gActiveMqttClient = this;
        mqttClient.setCallback(mqttMessageCallback);
    }
}

MqttGatewayClient::MqttGatewayClient(ConnectivityDriver& transportDriver, Client& networkClient, 
                                     const char* brokerIpAddress, const uint16_t brokerPort,
                                     const char* topic, const char* deviceId)
    : GatewayClient(transportDriver, ""), // We don't use serviceEndpointUrl here, so pass ""
      mqttClient(networkClient),
      clientIdentifier(deviceId),
      publishTopic(topic),
      subscribeTopic(nullptr),
      lastReconnectAttemptTimestamp(0),
      reconnectIntervalMilliseconds(DEFAULT_RECONNECT_INTERVAL_MILLISECONDS),
      sendsSucceededCount(0),
      sendsFailedCount(0),
      firstFailureLogged(false),
      serializationBuffer{},
      targetDevice(nullptr),
      subscribePayloadBuffer{}
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
    if (const unsigned long currentTimestamp = millis(); lastReconnectAttemptTimestamp == 0 ||
                                                   (currentTimestamp - lastReconnectAttemptTimestamp >= reconnectIntervalMilliseconds)) {
        lastReconnectAttemptTimestamp = currentTimestamp;
        MODEST_LOG_I("MQTT", "Attempting connection to broker for client ID: %s", clientIdentifier);

        if (mqttClient.connect(clientIdentifier)) {
            MODEST_LOG_I("MQTT", "Connected to MQTT broker successfully.");
            // W15: Subscribe to the inbound command topic after successful connection
            subscribeToTopic();
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

void MqttGatewayClient::setReconnectInterval(unsigned long intervalMilliseconds) {
    reconnectIntervalMilliseconds = intervalMilliseconds;
}

unsigned long MqttGatewayClient::getReconnectInterval() const {
    return reconnectIntervalMilliseconds;
}

bool MqttGatewayClient::sendTelemetryRecord(const TelemetryPackage& abstractTelemetryPayload) {
    // 1. Guard capability status check before spending memory processing power
    if (!communicationTransportDriver.canTransmit()) {
        communicationTransportDriver.connect(); // Asynchronously attempt a link recovery
        sendsFailedCount++;
        if (!firstFailureLogged) {
            MODEST_LOG_W("MQTT", "First send failure: transport not ready");
            firstFailureLogged = true;
        }
        return false;
    }

    // 2. Assure our application layer MQTT socket session state is established
    ensureBrokerSessionConnection();
    if (!mqttClient.connected()) {
        sendsFailedCount++;
        if (!firstFailureLogged) {
            MODEST_LOG_W("MQTT", "First send failure: broker not connected");
            firstFailureLogged = true;
        }
        return false;
    }

    // 3. Execute payload serialization polymorphically into bounded buffer
    JsonDocument payload;
    abstractTelemetryPayload.addEnvelopeMetadata(payload);
    abstractTelemetryPayload.serialize(payload);
    if (const size_t serializedLength = serializeJson(payload, serializationBuffer, sizeof(serializationBuffer)); serializedLength == 0) {
        MODEST_LOG_W("MQTT", "Payload too large for serialization buffer (%u bytes)", MAX_TELEMETRY_PAYLOAD_SIZE);
        sendsFailedCount++;
        if (!firstFailureLogged) {
            MODEST_LOG_W("MQTT", "First send failure: payload exceeds buffer size");
            firstFailureLogged = true;
        }
        return false;
    }

    // 4. Publish telemetry payload onto the broker topic ring (Executed on priority 1 thread)
    const bool publishSuccessStatus = mqttClient.publish(publishTopic, serializationBuffer);
    if (!publishSuccessStatus) {
        sendsFailedCount++;
        if (!firstFailureLogged) {
            MODEST_LOG_W("MQTT", "First send failure: publish to topic %s failed", publishTopic);
            firstFailureLogged = true;
        }
    } else {
        sendsSucceededCount++;
        MODEST_LOG_D("MQTT", "Published telemetry payload to topic: %s", publishTopic);
    }
    
    // Maintain internal background MQTT processing states (pings, heartbeats)
    mqttClient.loop();

    return publishSuccessStatus;
}

GatewayStatistics MqttGatewayClient::getStatistics() const {
    GatewayStatistics stats;
    stats.sendsSucceeded = sendsSucceededCount;
    stats.sendsFailed = sendsFailedCount;
    return stats;
}

const char* MqttGatewayClient::getSubscribeTopic() const {
    return subscribeTopic;
}

// ============================================================================
// W15: Cloud-to-device MQTT subscribe
// ============================================================================

void MqttGatewayClient::subscribeToTopic() {
    if (subscribeTopic == nullptr) {
        return;
    }
    
    if (mqttClient.subscribe(subscribeTopic, DEFAULT_MQTT_QOS)) {
        MODEST_LOG_I("MQTT", "Subscribed to topic: %s", subscribeTopic);
    } else {
        MODEST_LOG_W("MQTT", "Failed to subscribe to topic: %s", subscribeTopic);
    }
}

void MqttGatewayClient::mqttMessageCallback(char* topic, const byte* payload, unsigned int length) {
    if (gActiveMqttClient == nullptr || gActiveMqttClient->targetDevice == nullptr) {
        return;
    }

    // Guard: ignore messages that don't match the subscribe topic
    if (gActiveMqttClient->subscribeTopic == nullptr) {
        return;
    }

    // Safely copy payload into fixed buffer
    if (length >= MAX_SUBSCRIBE_PAYLOAD_SIZE) {
        MODEST_LOG_W("MQTT", "Incoming command payload too large (%u bytes)", length);
        return;
    }
    
    // Null-terminate the payload for JSON parsing
    memcpy(gActiveMqttClient->subscribePayloadBuffer, payload, length);
    gActiveMqttClient->subscribePayloadBuffer[length] = '\0';

    // Parse JSON to extract command identifier
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, gActiveMqttClient->subscribePayloadBuffer);
    if (error != DeserializationError::Ok) {
        MODEST_LOG_W("MQTT", "Failed to parse command JSON (error code: %d)", static_cast<int>(error));
        return;
    }

    // Extract command identifier from JSON
    if (!doc["commandId"].is<int>()) {
        MODEST_LOG_W("MQTT", "Incoming command missing 'commandId' field");
        return;
    }

    const int commandId = doc["commandId"].as<int>();
    MODEST_LOG_D("MQTT", "Received command %d on topic: %s", commandId, topic);

    // Marshal the command to the sampler task via Device::enqueueCommand()
    // The Device is both the target handler and the marshaler, ensuring cross-task safety
    Command command(commandId);
    gActiveMqttClient->targetDevice->enqueueCommand(gActiveMqttClient->targetDevice, command);
}