/**
 * @file MqttGatewayClient.cpp
 * @brief Implements MQTT broker session handling and telemetry publish operations.
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-06-06
 * @version 1.0
 */

#include "MqttGatewayClient.h"
#include "ModestLog.h"
#include <ArduinoJson.h>   // JSON parsing for incoming command payloads

// Static pointer for callback access (set by constructor)
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
    mqttClient.begin(brokerIpAddress, brokerPort, networkClient);
    
    // Register the static callback if subscribe is enabled
    if (subscribeTopic != nullptr) {
        gActiveMqttClient = this;
        mqttClient.onMessage(mqttMessageCallback);
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
    mqttClient.begin(brokerIpAddress, brokerPort, networkClient);
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
                                                   currentTimestamp - lastReconnectAttemptTimestamp >= reconnectIntervalMilliseconds) {
        lastReconnectAttemptTimestamp = currentTimestamp;
        MODEST_LOG_I("MQTT", "Attempting connection to broker for client ID: %s", clientIdentifier);

        if (mqttClient.connect(clientIdentifier)) {
            MODEST_LOG_I("MQTT", "Connected to MQTT broker successfully.");
            // Subscribe to the inbound command topic after successful connection
            subscribeToTopic();
        } else {
            MODEST_LOG_W("MQTT", "Failed to connect to MQTT broker, error: %d", mqttClient.lastError());
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
    return mqttClient.lastError();
}

void MqttGatewayClient::setReconnectInterval(const unsigned long intervalMilliseconds) {
    reconnectIntervalMilliseconds = intervalMilliseconds;
}

unsigned long MqttGatewayClient::getReconnectInterval() const {
    return reconnectIntervalMilliseconds;
}

bool MqttGatewayClient::sendTelemetryRecord(const TelemetryPackage& abstractTelemetryPayload) {
    // Check transport capability before processing
    if (!communicationTransportDriver.canTransmit()) {
        communicationTransportDriver.connect(); // Asynchronously attempt a link recovery
        sendsFailedCount++;
        if (!firstFailureLogged) {
            MODEST_LOG_W("MQTT", "First send failure: transport not ready");
            firstFailureLogged = true;
        }
        return false;
    }

    // Ensure MQTT broker session is established
    ensureBrokerSessionConnection();
    if (!mqttClient.connected()) {
        sendsFailedCount++;
        if (!firstFailureLogged) {
            MODEST_LOG_W("MQTT", "First send failure: broker not connected");
            firstFailureLogged = true;
        }
        return false;
    }

    // Serialize telemetry payload into bounded buffer
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

    // Publish telemetry payload to the broker topic
    const bool publishSuccessStatus = mqttClient.publish(publishTopic, serializationBuffer, strlen(serializationBuffer));
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

/**
 * @name Cloud-to-device MQTT subscribe
 * @{
 *
 * @brief Subscribes to an MQTT topic for inbound commands and marshals
 * parsed JSON payloads to the Device via enqueueCommand().
 */

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

void MqttGatewayClient::mqttMessageCallback(String& topic, String& payload) {
    (void)topic; // Used only in MODEST_LOG_D, which may be compiled out
    if (gActiveMqttClient == nullptr || gActiveMqttClient->targetDevice == nullptr) {
        return;
    }

    // Guard: ignore messages that don't match the subscribe topic
    if (gActiveMqttClient->subscribeTopic == nullptr) {
        return;
    }

    // Safely copy payload into fixed buffer
    const size_t length = payload.length();
    if (length >= static_cast<size_t>(MAX_SUBSCRIBE_PAYLOAD_SIZE)) {
        MODEST_LOG_W("MQTT", "Incoming command payload too large (%d bytes)", length);
        return;
    }
    
    // Null-terminate the payload for JSON parsing
    memcpy(gActiveMqttClient->subscribePayloadBuffer, payload.c_str(), length);
    gActiveMqttClient->subscribePayloadBuffer[length] = '\0';

    // Parse JSON to extract command identifier
    JsonDocument doc;
    if (const DeserializationError error = deserializeJson(doc, gActiveMqttClient->subscribePayloadBuffer); error != DeserializationError::Ok) {
        MODEST_LOG_W("MQTT", "Failed to parse command JSON (error code: %d)", static_cast<int>(error));
        return;
    }

    // Extract command identifier from JSON
    if (!doc[JSON_KEY_COMMAND_IDENTIFIER].is<int>()) {
        MODEST_LOG_W("MQTT", "Incoming command missing command identifier field");
        return;
    }

    const int commandId = doc[JSON_KEY_COMMAND_IDENTIFIER].as<int>();
    const int32_t commandPayload = doc[JSON_KEY_COMMAND_PAYLOAD] | 0;
    MODEST_LOG_D("MQTT", "Received command %d (payload %ld) on topic: %s", commandId, commandPayload, topic.c_str());

    // Marshal the command to the sampler task via Device::enqueueCommand()
    // The Device is both the target handler and the marshaler, ensuring cross-task safety
    const Command command(commandId, commandPayload);
    gActiveMqttClient->targetDevice->enqueueCommand(gActiveMqttClient->targetDevice, command);
}
/** @} */