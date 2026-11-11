#ifndef BLE_GATEWAY_CLIENT_H
#define BLE_GATEWAY_CLIENT_H

/**
 * @file BleGatewayClient.h
 * @brief Declares a BLE NUS gateway client for serialized telemetry payloads.
 * @ingroup gateway_module
 * @date 2026-11-11
 * @version 1.0
 */

#include "GatewayClient.h"
#include "TelemetryPackage.h"

/**
 * @brief Gateway adapter that sends `TelemetryPackage` payloads over a BLE Nordic UART Service (NUS)
 *        connection using JSON serialization with a fixed-size buffer.
 *
 * Each telemetry record is serialized to JSON and sent as a GATT notification on the TX characteristic.
 * The message format is JSON lines: each notification contains one JSON object terminated by a newline.
 *
 * @ingroup gateway_module
 */
class BleGatewayClient : public GatewayClient {
private:
    uint32_t sendsSucceededCount; ///< Successful telemetry transmissions.
    uint32_t sendsFailedCount; ///< Failed telemetry transmissions.

    static constexpr size_t SERIALIZATION_BUFFER_SIZE = 512; ///< Maximum serialized payload size in bytes.
    char serializationBuffer[SERIALIZATION_BUFFER_SIZE]; ///< Fixed-size buffer for JSON serialization.

    /**
     * @brief Appends a newline terminator to the serialized JSON payload.
     *
     * BLE NUS is a raw byte stream; the newline delimiter allows the central peer
     * to parse individual JSON objects from the continuous stream.
     *
     * @param buffer Pointer to the serialization buffer.
     * @param currentLength Current length of the serialized JSON (excluding newline).
     */
    static void appendNewlineTerminator(char* buffer, size_t currentLength);

public:
    /**
     * @brief Constructs a BLE NUS gateway client.
     * @param transportDriver Reference to the BLE connectivity driver.
     * @param deviceName Optional device name override for BLE advertising.
     */
    explicit BleGatewayClient(ConnectivityDriver& transportDriver, const char* deviceName = nullptr);

    ~BleGatewayClient() override = default;

    /**
     * @brief Polymorphically serializes any payload inheriting from TelemetryPackage and sends it.
     *
     * Serializes the payload to JSON, appends a newline terminator, and sends the result
     * as a GATT notification on the BLE TX characteristic. Runs in the network task context.
     *
     * @param abstractTelemetryPayload Polymorphic payload to serialize and transmit.
     * @return True if the payload was successfully queued for GATT notification.
     * @runs_on network
     */
    bool sendTelemetryRecord(const TelemetryPackage& abstractTelemetryPayload) override;

    /**
     * @brief Periodic background upkeep routine for BLE connection maintenance.
     *
     * Checks whether the BLE peer is still connected and restarts advertising
     * if the connection was lost. Unlike MQTT, BLE does not require keep-alive pings.
     */
    void service() override;

    /**
     * @brief Ergonomic alias for service() matching standard Arduino loop conventions.
     */
    void loop() override;

    /**
     * @brief Returns send success/failure counters for diagnostics aggregation.
     * @return Statistics struct with cumulative send counts.
     */
    [[nodiscard]] GatewayStatistics getStatistics() const override;
};

#endif // BLE_GATEWAY_CLIENT_H
