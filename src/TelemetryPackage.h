#ifndef TELEMETRY_PACKAGE_H
#define TELEMETRY_PACKAGE_H

/**
 * @file TelemetryPackage.h
 * @brief Declares the abstract telemetry payload contract used by gateway clients.
 * @ingroup core_module
 * @date 2026-06-06
 * @version 1.0
 */

#include <Arduino.h>
#include <ArduinoJson.h>

/**
 * @name Core: Telemetry Contracts
 * @ingroup core_module
 * @{
 */

/**
 * @brief Polymorphic payload contract for telemetry serialization.
 * @ingroup core_module
 */
class TelemetryPackage {
protected:
    uint32_t captureTimestamp = 0; ///< Capture time in milliseconds from device boot.
    uint32_t sequenceNumber = 0; ///< Monotonically increasing sequence assigned before enqueue.

public:
    /**
     * @brief Sets the envelope metadata values.
     * @param timestamp Capture time in milliseconds from device boot.
     * @param sequence Monotonically increasing sequence number.
     * @note Called by Device before enqueueing to assign metadata.
     */
    void setEnvelopeMetadata(uint32_t timestamp, uint32_t sequence) {
        captureTimestamp = timestamp;
        sequenceNumber = sequence;
    }

    /**
     * @brief Adds envelope metadata fields to the JSON document.
     * @param doc Document to populate with timestamp and sequence.
     */
    void addEnvelopeMetadata(JsonDocument& doc) const {
        doc["timestamp"] = static_cast<int>(captureTimestamp);
        doc["sequence"] = static_cast<int>(sequenceNumber);
    }

    /** @brief Virtual destructor for safe polymorphic deletion. */
    virtual ~TelemetryPackage() = default;

    /**
     * @brief Serializes fields into an existing JSON document instance.
     * @param serializationDestination Destination document to populate.
     * @note Envelope metadata (timestamp, sequence) is added automatically by
     *   the `serialize()` convenience overload before calling this method.
     */
    virtual void serialize(JsonDocument& serializationDestination) const = 0;

    /**
     * @brief Convenience overload that creates and returns a standalone document.
     * @return A JSON document with envelope metadata followed by subclass fields.
     */
    [[nodiscard]] JsonDocument serialize() const {
        JsonDocument standaloneDocument;
        this->addEnvelopeMetadata(standaloneDocument);
        this->serialize(standaloneDocument);
        return standaloneDocument;
    }

    /**
     * @brief Convenience method to serialize telemetry to a JSON string.
     * @return A String containing the serialized JSON representation.
     */
    [[nodiscard]] String serializeAsString() const {
        JsonDocument doc = this->serialize();
        String output;
        serializeJson(doc, output);
        return output;
    }
};

/** @} */ // End of Core: Telemetry Contracts

#endif // TELEMETRY_PACKAGE_H
