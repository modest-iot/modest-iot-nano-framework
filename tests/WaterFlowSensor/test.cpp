#include <time.h>
/**
 * @file test.cpp
 * @brief Host unit tests for the WaterFlowSensor component.
 *
 * Validates flow rate calculation, pulse counting, and reset behavior.
 */

#include "Arduino.h"
#include "Event.h"
#include "Sensor.h"
#include "WaterFlowSensor.h"
#include <cassert>
#include <cstdio>

namespace {

// Test subclass that exposes protected members for testing
class TestableWaterFlowSensor : public WaterFlowSensor {
public:
    using WaterFlowSensor::WaterFlowSensor;
    using WaterFlowSensor::pulseCount;
    using WaterFlowSensor::cachedFlowRateInLitersPerMinute;
    using WaterFlowSensor::lastNotifiedFlowRateInLitersPerMinute;
    using WaterFlowSensor::processEvent;
    using WaterFlowSensor::PROPAGATION_SUPPRESSION_IDENTIFIER;
    using WaterFlowSensor::INITIAL_LAST_NOTIFIED_FLOW_RATE_IN_LITERS_PER_MINUTE;

    /** @brief Test-only reset that clears all internal state. */
    void resetPulseCount() {
        pulseCount = 0;
        cachedFlowRateInLitersPerMinute = 0.0f;
        lastNotifiedFlowRateInLitersPerMinute = INITIAL_LAST_NOTIFIED_FLOW_RATE_IN_LITERS_PER_MINUTE;
    }
};

} // namespace

int main() {
    std::printf("Running WaterFlowSensor tests...\n");

    // Test 1: Constructor initializes state correctly
    {
        WaterFlowSensor sensor(4);
        assert(sensor.getFlowRateInLitersPerMinute() == 0.0f);
        assert(sensor.getSamplingIntervalInMilliseconds() == 1000);
        assert(sensor.getEffectiveMinimalSamplingIntervalInMilliseconds() == 200);
        std::printf("  [PASS] Constructor initializes state correctly\n");
    }

    // Test 2: Flow rate calculation from pulse count
    {
        TestableWaterFlowSensor sensor(4);

        // Simulate 70 pulses in 1 second (70 pulses / 7 pulses/ml = 10 ml/s = 0.6 L/min)
        sensor.pulseCount = 70;

        Event event(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
        sensor.processEvent(event);

        assert(event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER);
        assert(event.sourceId == 4); // sourceId should be the pin number
        // 70 pulses / 7 pulses/ml / 1000 ml/L / 1s * 60s/min = 0.6 L/min
        float expected = 0.6f;
        float actual = sensor.getFlowRateInLitersPerMinute();
        assert(actual > expected - 0.01f && actual < expected + 0.01f);
        std::printf("  [PASS] Flow rate calculation from pulse count\n");
    }

    // Test 3: Zero flow rate when no pulses
    {
        TestableWaterFlowSensor sensor(4);
        sensor.pulseCount = 0;

        Event event(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
        sensor.processEvent(event);

        assert(event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER);
        assert(sensor.getFlowRateInLitersPerMinute() == 0.0f);
        std::printf("  [PASS] Zero flow rate when no pulses\n");
    }

    // Test 4: Delta pulse calculation
    {
        TestableWaterFlowSensor sensor(4);
        sensor.pulseCount = 100;  // Initial count

        Event event(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
        sensor.processEvent(event);
        // First call: delta = 100 - 0 = 100 pulses -> non-zero flow rate, emits event
        assert(event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER);
        assert(sensor.getFlowRateInLitersPerMinute() > 0.0f);

        // Now add 70 more pulses (170 - 100 = 70 delta)
        sensor.pulseCount = 170;

        event = Event(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
        sensor.processEvent(event);

        assert(event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER);
        float expected = 0.6f;
        float actual = sensor.getFlowRateInLitersPerMinute();
        assert(actual > expected - 0.01f && actual < expected + 0.01f);
        std::printf("  [PASS] Delta pulse calculation\n");
    }

    // Test 5: Reset pulse count
    {
        TestableWaterFlowSensor sensor(4);
        sensor.pulseCount = 50;

        sensor.resetPulseCount();
        assert(sensor.pulseCount == 0);
        assert(sensor.cachedFlowRateInLitersPerMinute == 0.0f);
        assert(sensor.lastNotifiedFlowRateInLitersPerMinute == sensor.INITIAL_LAST_NOTIFIED_FLOW_RATE_IN_LITERS_PER_MINUTE);
        std::printf("  [PASS] Reset pulse count\n");
    }

    // Test 6: Emits standard DATA_READ_EVENT like other data-reading sensors
    {
        TestableWaterFlowSensor sensor(4);
        sensor.pulseCount = 70;

        Event event(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
        sensor.processEvent(event);

        assert(event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER);
        assert(event.sourceId == 4); // sourceId is the pin, not the measured value
        std::printf("  [PASS] Emits standard DATA_READ_EVENT like other data-reading sensors\n");
    }

    // Test 7: Suppresses propagation when flow rate hasn't changed
    {
        TestableWaterFlowSensor sensor(4);
        sensor.pulseCount = 70;

        Event event(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
        sensor.processEvent(event);
        // First read: value changes from -1.0 to 0.6 → emits event
        assert(event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER);

        // Second read with same pulse count → delta is 0, flow rate is 0
        // 0 is different from 0.6 → emits event
        event = Event(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
        sensor.processEvent(event);
        assert(event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER);
        assert(sensor.getFlowRateInLitersPerMinute() == 0.0f);
        // 0 == 0 (within epsilon) → suppresses propagation
        event = Event(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
        sensor.processEvent(event);
        assert(event.identifier == sensor.PROPAGATION_SUPPRESSION_IDENTIFIER);
        std::printf("  [PASS] Suppresses propagation when flow rate hasn't changed\n");
    }

    // Test 8: Reset clears lastNotifiedFlowRateInLitersPerMinute
    {
        TestableWaterFlowSensor sensor(4);
        sensor.pulseCount = 70;

        Event event(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
        sensor.processEvent(event);
        assert(event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER);

        sensor.resetPulseCount();
        assert(sensor.cachedFlowRateInLitersPerMinute == 0.0f);
        assert(sensor.lastNotifiedFlowRateInLitersPerMinute == sensor.INITIAL_LAST_NOTIFIED_FLOW_RATE_IN_LITERS_PER_MINUTE);

        // After reset, a read of 0 should emit event again (lastNotifiedFlowRateInLitersPerMinute is INITIAL_LAST_NOTIFIED_FLOW_RATE_IN_LITERS_PER_MINUTE)
        event = Event(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
        sensor.processEvent(event);
        assert(event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER);
        std::printf("  [PASS] Reset clears lastNotifiedFlowRateInLitersPerMinute\n");
    }

    std::printf("All WaterFlowSensor tests passed successfully!\n");
    return 0;
}
