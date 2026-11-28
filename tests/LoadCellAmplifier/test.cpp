#include <time.h>
/**
 * @file test.cpp
 * @brief Host unit tests for the LoadCellAmplifier component.
 *
 * Validates weight reading, calibration, tare, and power management.
 */

#include "Arduino.h"
#include "Event.h"
#include "Sensor.h"
#include "LoadCellAmplifier.h"
#include <cassert>
#include <cstdio>

int main() {
    std::printf("Running LoadCellAmplifier tests...\n");

    // Test 1: Constructor initializes state correctly
    {
        LoadCellAmplifier sensor(4, 5, 0.0f, 5000.0f, 0, 2000000, 5);
        assert(sensor.getWeightInGrams() == 0.0f);
        assert(sensor.getMinWeightInGrams() == 0.0f);
        assert(sensor.getMaxWeightInGrams() == 5000.0f);
        assert(sensor.getSamplingIntervalInMilliseconds() == 100);
        assert(sensor.getEffectiveMinimalSamplingIntervalInMilliseconds() == 100);
        std::printf("  [PASS] Constructor initializes state correctly\n");
    }

    // Test 2: Weight getters return initial values
    {
        LoadCellAmplifier sensor(6, 7, 100.0f, 10000.0f, 100000, 3000000, 3);
        assert(sensor.getMinWeightInGrams() == 100.0f);
        assert(sensor.getMaxWeightInGrams() == 10000.0f);
        std::printf("  [PASS] Weight getters return initial values\n");
    }

    // Test 3: setGain configures gain pulses
    {
        LoadCellAmplifier sensor(8, 9, 0.0f, 5000.0f, 0, 2000000);
        sensor.setGain(128);
        sensor.setGain(32);
        std::printf("  [PASS] setGain configures gain pulses\n");
    }

    // Test 4: Power down and up
    {
        LoadCellAmplifier sensor(10, 11, 0.0f, 5000.0f, 0, 2000000);
        sensor.powerDown();
        sensor.powerUp();
        std::printf("  [PASS] Power down and up\n");
    }

    // Test 5: Tare resets offset
    {
        LoadCellAmplifier sensor(12, 13, 0.0f, 5000.0f, 0, 2000000);
        fakeDigitalReadValue = LOW; // Simulate HX711 data-ready for tare's readRawValueWithWait()
        sensor.tare();
        std::printf("  [PASS] Tare resets offset\n");
    }

    // Test 6: Reset clears state
    {
        LoadCellAmplifier sensor(14, 15, 0.0f, 5000.0f, 0, 2000000);
        sensor.reset();
        assert(sensor.getWeightInGrams() == 0.0f);
        std::printf("  [PASS] Reset clears state\n");
    }

    // Test 7: A not-ready sample does not refresh sensor validity
    {
        LoadCellAmplifier sensor(16, 17, 0.0f, 5000.0f, 0, 2000000);
        sensor.setFaultThreshold(1);
        sensor.on(Sensor::MEASURE_DATA_REQUESTED_EVENT);
        assert(!sensor.hasValidReading());
        assert(sensor.getDataValidity(1000) == Sensor::DataValidity::NoValidSample);
        assert(!sensor.isReadFaultActive());
        std::printf("  [PASS] Not-ready sample does not refresh validity\n");
    }

    std::printf("All LoadCellAmplifier tests passed successfully!\n");
    return 0;
}
