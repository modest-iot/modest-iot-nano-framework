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
 * @brief Gateway adapter that publishes `TelemetryPackage` payloads to an MQTT topic.
 * @ingroup gateway_module
 */
class MqttGatewayClient : public GatewayClient {
private:
    PubSubClient mqttClient; ///< Underlying MQTT client instance.
    
    const char* clientIdentifier; ///< Unique client identifier.
    const char* publishTopic; ///< MQTT topic for publishing.

    unsigned long lastReconnectAttemptTimestamp; ///< Timestamp of the last broker connection attempt.
    unsigned long reconnectIntervalMilliseconds; ///< Minimum cooldown between reconnect attempts in milliseconds.

    uint32_t sendsSucceededCount; ///< Successful telemetry transmissions.
    uint32_t sendsFailedCount; ///< Failed telemetry transmissions.
    bool firstFailureLogged; ///< Tracks whether the first failure warning was emitted.

    static constexpr size_t MAX_TELEMETRY_PAYLOAD_SIZE = 512; ///< Maximum serialized payload size in bytes.
    char serializationBuffer[MAX_TELEMETRY_PAYLOAD_SIZE]; ///< Fixed-size buffer for JSON serialization.

    static constexpr uint16_t DEFAULT_RECONNECT_INTERVAL_MILLISECONDS = 5000; ///< Default cooldown between broker reconnection attempts.
    void ensureBrokerSessionConnection(); ///< Ensures broker session is active with rate-limited backoff.

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
    
    ~MqttGatewayClient() override = default;

    /**
     * @brief Polymorphically serializes any payload inheriting from TelemetryPackage and publishes it.
     * @param abstractTelemetryPayload Polymorphic payload to serialize and publish.
     * @return True if the payload byte stream was successfully accepted and pushed by the broker.
     */
    bool sendTelemetryRecord(const TelemetryPackage& abstractTelemetryPayload) override;

    /**
     * @brief Periodic background upkeep routine for MQTT ping packets and keep-alive handling.
     * Maintains broker connection and executes PubSubClient loop processing.
     */
    void service() override;

    /**
     * @brief Ergonomic alias for service() matching standard Arduino loop conventions.
     */
    void loop() override;

    /**
     * @brief Inquires whether both the transport driver and the MQTT broker session are connected.
     * @return `true` if connected to broker and ready for publishing.
     */
    [[nodiscard]] bool isConnected() override;

    /**
     * @brief Returns the internal state integer reported by PubSubClient.
     * @return PubSubClient state integer (e.g. 0 = MQTT_CONNECTED).
     */
    [[nodiscard]] int getClientState();

    /**
     * @brief Sets the cooldown interval between broker reconnection attempts.
     * @param intervalMilliseconds Cooldown period in milliseconds (default: 5000ms).
     */
    void setReconnectInterval(unsigned long intervalMilliseconds);

    /**
     * @brief Gets the configured cooldown interval between broker reconnection attempts.
     * @return Cooldown period in milliseconds.
     */
    [[nodiscard]] unsigned long getReconnectInterval() const;

    /**
     * @brief Returns send success/failure counters for diagnostics aggregation.
     * @return Statistics struct with cumulative send counts.
     */
    [[nodiscard]] GatewayStatistics getStatistics() const override;
};

#endif // MQTT_GATEWAY_CLIENT_H