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
public:
    /** @brief Virtual destructor for safe polymorphic deletion. */
    virtual ~TelemetryPackage() = default;

    /**
     * @brief Serializes fields into an existing JSON document instance.
     * @param serializationDestination Destination document to populate.
     */
    virtual void serialize(JsonDocument& serializationDestination) const = 0;

    /**
     * @brief Convenience overload that creates and returns a standalone document.
     * @return A JSON document populated by `serialize(JsonDocument&)`.
     */
    [[nodiscard]] JsonDocument serialize() const {
        JsonDocument standaloneDocument;
        // Delegate directly to the virtual method implemented by subclasses
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
