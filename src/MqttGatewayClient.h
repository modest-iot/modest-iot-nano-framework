#ifndef MQTT_GATEWAY_CLIENT_H
#define MQTT_GATEWAY_CLIENT_H

/**
 * @file MqttGatewayClient.h
 * @brief Declares an MQTT publisher client for serialized telemetry payloads.
 * @ingroup gateway_module
 * @date 2026-06-06
 * @version 1.0
 */

#include "GatewayClient.h"
#include "TelemetryPackage.h"
#include <PubSubClient.h>   // Extends standard PubSubClient compilation visibility

/**
 * @name Gateway: Protocol-Specific Implementations
 * @ingroup gateway_module
 * @{
 */

/**
 * @brief Gateway adapter that publishes `TelemetryPackage` payloads to an MQTT topic.
 * @ingroup gateway_module
 */
class MqttGatewayClient : public GatewayClient {
private:
    PubSubClient mqttClient; ///< Underlying MQTT client instance.
    
    const char* clientIdentifier; ///< Unique client identifier.
    const char* publishTopic; ///< MQTT topic for publishing.

    void ensureBrokerSessionConnection(); ///< Ensures broker session is active.

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
    bool sendTelemetryRecord(const TelemetryPackage& abstractTelemetryPayload) override;
};

/** @} */ // End of Gateway: Protocol-Specific Implementations

#endif // MQTT_GATEWAY_CLIENT_H