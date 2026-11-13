#include <time.h>
#include "TelemetryPackage.h"
#include <ArduinoJson.h>
#include <cassert>
#include <cstdio>
#include <string>

namespace {

// Concrete telemetry packages matching the examples
class SmartBandTelemetryPackage : public TelemetryPackage {
private:
    int heartBeatsPerMinute;
    float skinTemperatureInCelsius;
    float relativeHumidityInPercentage;

public:
    SmartBandTelemetryPackage(int heartRate, float temperature, float humidity)
        : heartBeatsPerMinute(heartRate),
          skinTemperatureInCelsius(temperature),
          relativeHumidityInPercentage(humidity) {}

    ~SmartBandTelemetryPackage() override = default;

    void serialize(JsonDocument& serializationDestination) const override {
        serializationDestination["heartBeatsPerMinute"] = this->heartBeatsPerMinute;
        serializationDestination["skinTemperatureInCelsius"] = this->skinTemperatureInCelsius;
        serializationDestination["relativeHumidityPercentage"] = this->relativeHumidityInPercentage;
    }
};

class AirQualityTelemetryPackage : public TelemetryPackage {
private:
    float gasConcentrationInPpm;
    bool gasDetected;
    int operatingMode;

public:
    AirQualityTelemetryPackage(float ppm, bool detected, int mode)
        : gasConcentrationInPpm(ppm), gasDetected(detected), operatingMode(mode) {}

    ~AirQualityTelemetryPackage() override = default;

    void serialize(JsonDocument& serializationDestination) const override {
        serializationDestination["gasPpm"] = this->gasConcentrationInPpm;
        serializationDestination["gasDetected"] = this->gasDetected;
        serializationDestination["sensorMode"] = this->operatingMode;
    }
};

} // anonymous namespace

[[nodiscard]] int main() {
    std::printf("Running Telemetry Serialization Golden Tests...\n");

    // =========================================================================
    // Test 1: SmartBandTelemetryPackage default envelope metadata
    // =========================================================================
    {
        SmartBandTelemetryPackage packet(72, 36.5f, 45.2f);
        packet.setEnvelopeMetadata(0, 0);
        String json = packet.toJsonString();

        // Keys are sorted alphabetically by std::map in the mock:
        // heartBeatsPerMinute, relativeHumidityPercentage, sequence,
        // skinTemperatureInCelsius, timestamp
        std::string expected = "{\"heartBeatsPerMinute\":72,\"relativeHumidityPercentage\":45.2,\"sequence\":0,\"skinTemperatureInCelsius\":36.5,\"timestamp\":0}";
        assert(json == expected);
        std::printf("  SmartBand default metadata: PASS\n");
    }

    // =========================================================================
    // Test 2: SmartBandTelemetryPackage with custom envelope metadata
    // =========================================================================
    {
        SmartBandTelemetryPackage packet(72, 36.5f, 45.2f);
        packet.setEnvelopeMetadata(1234567, 42);
        String json = packet.toJsonString();

        std::string expected = "{\"heartBeatsPerMinute\":72,\"relativeHumidityPercentage\":45.2,\"sequence\":42,\"skinTemperatureInCelsius\":36.5,\"timestamp\":1234567}";
        assert(json == expected);
        std::printf("  SmartBand custom metadata: PASS\n");
    }

    // =========================================================================
    // Test 3: AirQualityTelemetryPackage default envelope metadata
    // =========================================================================
    {
        AirQualityTelemetryPackage packet(250.5f, true, 0);
        packet.setEnvelopeMetadata(0, 0);
        String json = packet.toJsonString();

        // Keys sorted alphabetically: gasDetected, gasPpm, sensorMode,
        // sequence, timestamp
        std::string expected = "{\"gasDetected\":true,\"gasPpm\":250.5,\"sensorMode\":0,\"sequence\":0,\"timestamp\":0}";
        assert(json == expected);
        std::printf("  AirQuality default metadata: PASS\n");
    }

    // =========================================================================
    // Test 4: AirQualityTelemetryPackage with custom envelope metadata
    // =========================================================================
    {
        AirQualityTelemetryPackage packet(250.5f, true, 0);
        packet.setEnvelopeMetadata(9876543, 99);
        String json = packet.toJsonString();

        std::string expected = "{\"gasDetected\":true,\"gasPpm\":250.5,\"sensorMode\":0,\"sequence\":99,\"timestamp\":9876543}";
        assert(json == expected);
        std::printf("  AirQuality custom metadata: PASS\n");
    }

    // =========================================================================
    // Test 5: SmartBandTelemetryPackage with zero values
    // =========================================================================
    {
        SmartBandTelemetryPackage packet(0, 0.0f, 0.0f);
        packet.setEnvelopeMetadata(0, 0);
        String json = packet.toJsonString();

        std::string expected = "{\"heartBeatsPerMinute\":0,\"relativeHumidityPercentage\":0,\"sequence\":0,\"skinTemperatureInCelsius\":0,\"timestamp\":0}";
        assert(json == expected);
        std::printf("  SmartBand zero values: PASS\n");
    }

    // =========================================================================
    // Test 6: AirQualityTelemetryPackage with gas not detected
    // =========================================================================
    {
        AirQualityTelemetryPackage packet(0.0f, false, 2);
        packet.setEnvelopeMetadata(100, 1);
        String json = packet.toJsonString();

        std::string expected = "{\"gasDetected\":false,\"gasPpm\":0,\"sensorMode\":2,\"sequence\":1,\"timestamp\":100}";
        assert(json == expected);
        std::printf("  AirQuality gas not detected: PASS\n");
    }

    // =========================================================================
    // Test 7: Bounded buffer overflow detection
    // =========================================================================
    {
        SmartBandTelemetryPackage packet(72, 36.5f, 45.2f);
        packet.setEnvelopeMetadata(1234567, 42);

        // Serialize to a very small buffer — should return 0 (overflow)
        char smallBuffer[10];
        JsonDocument doc = packet.toDocument();
        size_t len = serializeJson(doc, smallBuffer, sizeof(smallBuffer));
        assert(len == 0);
        std::printf("  Bounded buffer overflow: PASS\n");
    }

    // =========================================================================
    // Test 8: Bounded buffer with sufficient size
    // =========================================================================
    {
        SmartBandTelemetryPackage packet(72, 36.5f, 45.2f);
        packet.setEnvelopeMetadata(1234567, 42);

        char buffer[512];
        JsonDocument doc = packet.toDocument();
        size_t len = serializeJson(doc, buffer, sizeof(buffer));
        assert(len > 0);
        assert(std::string(buffer).find("\"heartBeatsPerMinute\":72") != std::string::npos);
        assert(std::string(buffer).find("\"sequence\":42") != std::string::npos);
        assert(std::string(buffer).find("\"timestamp\":1234567") != std::string::npos);
        std::printf("  Bounded buffer sufficient size: PASS\n");
    }

    // =========================================================================
    // Test 9: toDocument() returns correct field count
    // =========================================================================
    {
        SmartBandTelemetryPackage packet(72, 36.5f, 45.2f);
        packet.setEnvelopeMetadata(100, 1);
        JsonDocument doc = packet.toDocument();
        const auto& fields = doc.getFields();
        assert(fields.size() == 5); // timestamp, sequence, heartBeatsPerMinute, skinTemperatureInCelsius, relativeHumidityPercentage
        std::printf("  SmartBand field count: PASS\n");
    }

    // =========================================================================
    // Test 10: AirQualityTelemetryPackage toDocument() field count
    // =========================================================================
    {
        AirQualityTelemetryPackage packet(250.5f, true, 0);
        packet.setEnvelopeMetadata(200, 2);
        JsonDocument doc = packet.toDocument();
        const auto& fields = doc.getFields();
        assert(fields.size() == 5); // timestamp, sequence, gasPpm, gasDetected, sensorMode
        std::printf("  AirQuality field count: PASS\n");
    }

    std::printf("All telemetry serialization tests passed.\n");
    return 0;
}
