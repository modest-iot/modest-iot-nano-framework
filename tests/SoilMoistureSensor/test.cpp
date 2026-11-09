/**
 * @file test.cpp
 * @brief Host unit tests for the SoilMoistureSensor component.
 *
 * Validates moisture reading, change detection, and event emission.
 */

#include "Arduino.h"
#include "Event.h"
#include "Sensor.h"
#include "SoilMoistureSensor.h"
#include <cassert>
#include <cstdio>

namespace {

// Test subclass that exposes protected members for testing
class TestableSoilMoistureSensor : public SoilMoistureSensor {
public:
    using SoilMoistureSensor::SoilMoistureSensor;
    using SoilMoistureSensor::cachedMoistureLevel;
    using SoilMoistureSensor::lastNotifiedMoistureLevel;
    using SoilMoistureSensor::processEvent;
    using SoilMoistureSensor::PROPAGATION_SUPPRESSION_IDENTIFIER;
    using SoilMoistureSensor::INITIAL_LAST_NOTIFIED_MOISTURE_LEVEL;
};

} // namespace

int main() {
    std::printf("Running SoilMoistureSensor tests...\n");

    // Test 1: Constructor initializes state correctly
    {
        SoilMoistureSensor sensor(34);
        assert(sensor.getMoistureLevel() == 0);
        assert(sensor.getSamplingIntervalInMilliseconds() == 1000);
        assert(sensor.getEffectiveMinimalSamplingIntervalInMilliseconds() == 100);
        std::printf("  [PASS] Constructor initializes state correctly\n");
    }

    // Test 2: First reading always emits DATA_READ_EVENT
    {
        TestableSoilMoistureSensor sensor(34);
        fakeAnalogRead[34] = 500;

        Event event(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
        sensor.processEvent(event);

        assert(event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER);
        assert(event.sourceId == 34);
        assert(sensor.getMoistureLevel() == 500);
        std::printf("  [PASS] First reading always emits DATA_READ_EVENT\n");
    }

    // Test 3: Unchanged moisture suppresses propagation
    {
        TestableSoilMoistureSensor sensor(34);
        fakeAnalogRead[34] = 500;

        Event event(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
        sensor.processEvent(event);
        assert(event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER);

        // Same value — should suppress
        event = Event(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
        sensor.processEvent(event);
        assert(event.identifier == sensor.PROPAGATION_SUPPRESSION_IDENTIFIER);
        std::printf("  [PASS] Unchanged moisture suppresses propagation\n");
    }

    // Test 4: Moisture change > epsilon emits DATA_READ_EVENT
    {
        TestableSoilMoistureSensor sensor(34);
        fakeAnalogRead[34] = 500;

        Event event(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
        sensor.processEvent(event);
        assert(event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER);

        // Change by 100 ADC units — well above epsilon of 5
        fakeAnalogRead[34] = 600;
        event = Event(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
        sensor.processEvent(event);
        assert(event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER);
        assert(sensor.getMoistureLevel() == 600);
        std::printf("  [PASS] Moisture change > epsilon emits DATA_READ_EVENT\n");
    }

    // Test 5: Moisture change of 1 ADC unit suppresses as noise
    {
        TestableSoilMoistureSensor sensor(34);
        fakeAnalogRead[34] = 500;

        Event event(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
        sensor.processEvent(event);
        assert(event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER);

        // Change by 1 ADC unit — below epsilon of 5
        fakeAnalogRead[34] = 501;
        event = Event(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
        sensor.processEvent(event);
        assert(event.identifier == sensor.PROPAGATION_SUPPRESSION_IDENTIFIER);
        std::printf("  [PASS] Moisture change of 1 ADC unit suppresses as noise\n");
    }

    // Test 6: Moisture change exactly equal to epsilon suppresses
    {
        TestableSoilMoistureSensor sensor(34);
        fakeAnalogRead[34] = 500;

        Event event(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
        sensor.processEvent(event);
        assert(event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER);

        // Change by exactly 5 — epsilon is strict greater-than, so suppresses
        fakeAnalogRead[34] = 505;
        event = Event(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
        sensor.processEvent(event);
        assert(event.identifier == sensor.PROPAGATION_SUPPRESSION_IDENTIFIER);
        std::printf("  [PASS] Moisture change exactly equal to epsilon suppresses\n");
    }

    // Test 7: Sampling interval defaults are correct
    {
        SoilMoistureSensor sensor(34);
        assert(sensor.getSamplingIntervalInMilliseconds() == 1000);
        assert(sensor.getEffectiveMinimalSamplingIntervalInMilliseconds() == 100);
        std::printf("  [PASS] Sampling interval defaults are correct\n");
    }

    std::printf("All SoilMoistureSensor tests passed successfully!\n");
    return 0;
}
