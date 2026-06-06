#ifndef MQTT_GATEWAY_CLIENT_H
#define MQTT_GATEWAY_CLIENT_H

/**
 * @file MqttGatewayClient.h
 * @brief Declares an MQTT publisher client for serialized telemetry payloads.
 */

#include "ConnectivityDriver.h"
#include "TelemetryPackage.h"
#include <Client.h>         // Standard Arduino Network Client reference interface
#include <PubSubClient.h>   // Extends standard PubSubClient compilation visibility

/**
 * @brief Gateway adapter that publishes `TelemetryPackage` payloads to an MQTT topic.
 */
class MqttGatewayClient {
private:
    ConnectivityDriver& communicationTransportDriver;
    PubSubClient mqttClient;
    
    const char* clientIdentifier;
    const char* publishTopic;

    void ensureBrokerSessionConnection();

public:
    /**
     * @param transportDriver Abstract reference handling physical Layer 1-4 link health.
     * @param networkClient The underlying TCP network client socket (e.g., WiFiClient).
     * @param brokerIpAddress The network IP or domain address of your MQTT Broker.
     * @param brokerPort The port of your broker instance (typically 1883 for unencrypted TCP).
     * @param topic Target MQTT topic string destination for publishing records.
     * @param deviceId Unique identification literal used during broker handshakes.
     */
    MqttGatewayClient(ConnectivityDriver& transportDriver, Client& networkClient, 
                      const char* brokerIpAddress, uint16_t brokerPort, 
                      const char* topic, const char* deviceId = "ModestIotNode");
    
    ~MqttGatewayClient() = default;

    /**
     * @brief Polymorphically serializes any payload inheriting from TelemetryPackage and publishes it.
     * @param abstractTelemetryPayload Polymorphic payload to serialize and publish.
     * @return True if the payload byte stream was successfully accepted and pushed by the broker.
     */
    bool sendTelemetryRecord(const TelemetryPackage& abstractTelemetryPayload);
};

#endif // MQTT_GATEWAY_CLIENT_H