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
#include "Device.h"
#include <MQTT.h>   // MQTT Things (256dpi/arduino-mqtt), actively maintained replacement for PubSubClient

/**
 * @brief Gateway adapter that publishes `TelemetryPackage` payloads to an MQTT topic
 *        and subscribes to inbound command topics for cloud-to-device communication.
 * @ingroup gateway_module
 */
class MqttGatewayClient : public GatewayClient {
private:
    MQTTClient mqttClient; ///< Underlying MQTT client instance (MQTT Things).
    
    const char* clientIdentifier; ///< Unique client identifier.
    const char* publishTopic; ///< MQTT topic for publishing telemetry.
    const char* subscribeTopic; ///< MQTT topic for subscribing to inbound commands.

    unsigned long lastReconnectAttemptTimestamp; ///< Timestamp of the last broker connection attempt.
    unsigned long reconnectIntervalMilliseconds; ///< Minimum cooldown between reconnect attempts in milliseconds.

    uint32_t sendsSucceededCount; ///< Successful telemetry transmissions.
    uint32_t sendsFailedCount; ///< Failed telemetry transmissions.
    bool firstFailureLogged; ///< Tracks whether the first failure warning was emitted.

    static constexpr size_t MAX_TELEMETRY_PAYLOAD_SIZE = 512; ///< Maximum serialized payload size in bytes.
    char serializationBuffer[MAX_TELEMETRY_PAYLOAD_SIZE]; ///< Fixed-size buffer for JSON serialization.

    // Cloud-to-device command infrastructure
    Device* targetDevice; ///< Device that receives incoming MQTT commands via enqueueCommand().
    static constexpr size_t MAX_SUBSCRIBE_PAYLOAD_SIZE = 256; ///< Maximum inbound command payload size.
    char subscribePayloadBuffer[MAX_SUBSCRIBE_PAYLOAD_SIZE]; ///< Fixed-size buffer for incoming command JSON.

    /**
     * @brief Static callback invoked by MQTTClient when a subscribed message arrives.
     *
     * Parses the JSON payload to extract the command identifier and marshals it
     * to the sampler task via Device::enqueueCommand(). Runs in the network task context
     * when mqttClient.loop() is called.
     *
     * @param topic MQTT topic of the incoming message.
     * @param payload Message payload as a String.
     */
    static void mqttMessageCallback(String& topic, String& payload);

    static constexpr uint16_t DEFAULT_RECONNECT_INTERVAL_MILLISECONDS = 5000; ///< Default cooldown between broker reconnection attempts.
    static constexpr uint8_t DEFAULT_MQTT_QOS = 0; ///< Default Quality of Service for MQTT subscribe.

    void ensureBrokerSessionConnection(); ///< Ensures broker session is active with rate-limited backoff.
    void subscribeToTopic(); ///< Subscribes to the inbound command topic after successful connection.

public:
    /**
     * @param transportDriver Abstract reference handling physical Layer 1-4 link health.
     * @param networkClient The underlying TCP network client socket (e.g., WiFiClient).
     * @param brokerIpAddress The network IP or domain address of your MQTT Broker.
     * @param brokerPort The port of your broker instance (typically 1883 for unencrypted TCP).
     * @param publishTopic Target MQTT topic string destination for publishing telemetry records.
     * @param subscribeTopic MQTT topic to subscribe to for inbound cloud-to-device commands.
     *        Pass nullptr to disable subscribe (publish-only mode).
     * @param targetDevice Device that receives incoming commands via enqueueCommand().
     *        Commands are marshaled to the sampler task for cross-task safety.
     *        Required when subscribeTopic is non-null; ignored otherwise.
     * @param deviceId Unique identification literal used during broker handshakes.
     */
    MqttGatewayClient(ConnectivityDriver& transportDriver, Client& networkClient, 
                      const char* brokerIpAddress, uint16_t brokerPort,
                      const char* publishTopic, const char* subscribeTopic,
                      Device* targetDevice,
                      const char* deviceId = "ModestIotNode");

    /**
     * @brief Legacy constructor for publish-only mode (backward compatible).
     *
     * Equivalent to calling the full constructor with subscribeTopic = nullptr
     * and targetDevice = nullptr.
     *
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
     * Maintains broker connection and processes pending MQTT messages.
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
     * @brief Returns the last error code from the MQTT client.
     * @return MQTT client error code.
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

    /**
     * @brief Returns the subscribe topic, or nullptr if subscribe is disabled.
     * @return Subscribe topic string, or nullptr.
     */
    [[nodiscard]] const char* getSubscribeTopic() const;
};

#endif // MQTT_GATEWAY_CLIENT_H