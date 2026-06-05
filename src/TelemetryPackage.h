#ifndef TELEMETRY_PACKAGE_H
#define TELEMETRY_PACKAGE_H

#include <ArduinoJson.h>

class TelemetryPackage {
public:
    virtual ~TelemetryPackage() = default;

    /**
     * @brief EXPLICIT TARGET MUTATOR: Serializes fields directly into an existing document context.
     * Essential for composing nested objects or appending metadata out of sight.
     */
    virtual void serialize(JsonDocument& serializationDestination) const = 0;

    /**
     * @brief STANDALONE GENERATOR OVERLOAD: Creates, populates, and returns a new document instance.
     * Leverages Return Value Optimization (RVO) to maximize memory efficiency.
     */
    JsonDocument serialize() const {
        JsonDocument standaloneDocument;
        // Delegate directly to the virtual method implemented by subclasses
        this->serialize(standaloneDocument);
        return standaloneDocument;
    }
};

#endif // TELEMETRY_PACKAGE_H