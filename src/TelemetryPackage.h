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
    void setEnvelopeMetadata(const uint32_t timestamp, const uint32_t sequence) {
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
     *   the `toDocument()` and `toJsonString()` convenience methods.
     */
    virtual void serialize(JsonDocument& serializationDestination) const = 0;

    /**
     * @brief Creates and returns a standalone JSON document with all fields.
     * @return A JSON document with envelope metadata followed by subclass fields.
     * @note Renamed from `serialize()` to avoid name hiding when subclasses
     *   override the pure virtual `serialize(JsonDocument&)` method.
     */
    [[nodiscard]] JsonDocument toDocument() const {
        JsonDocument standaloneDocument;
        this->addEnvelopeMetadata(standaloneDocument);
        this->serialize(standaloneDocument);
        return standaloneDocument;
    }

    /**
     * @brief Serializes telemetry to a JSON string.
     * @return A String containing the serialized JSON representation.
     * @note Renamed from `serializeAsString()` to avoid name hiding when
     *   subclasses override the pure virtual `serialize(JsonDocument&)` method.
     */
    [[nodiscard]] String toJsonString() const {
        const JsonDocument sourceDocument = this->toDocument();
        String serializedJsonString;
        serializeJson(sourceDocument, serializedJsonString);
        return serializedJsonString;
    }
};

#endif // TELEMETRY_PACKAGE_H
