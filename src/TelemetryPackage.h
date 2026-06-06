#ifndef TELEMETRY_PACKAGE_H
#define TELEMETRY_PACKAGE_H

/**
 * @file TelemetryPackage.h
 * @brief Declares the abstract telemetry payload contract used by gateway clients.
 */

#include <ArduinoJson.h>

/**
 * @brief Polymorphic payload contract for telemetry serialization.
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
    JsonDocument serialize() const {
        JsonDocument standaloneDocument;
        // Delegate directly to the virtual method implemented by subclasses
        this->serialize(standaloneDocument);
        return standaloneDocument;
    }
};

#endif // TELEMETRY_PACKAGE_H