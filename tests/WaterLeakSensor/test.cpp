/**
 * @file test.cpp
 * @brief Host tests for WaterLeakSensor (water leak detection sensor).
 * @ingroup sensors_module
 */

#include "WaterLeakSensor.h"
#include "Event.h"
#include "Sensor.h"
#include <cassert>
#include <iostream>
#include "../fakes/Arduino.h"

class TestableWaterLeakSensor : public WaterLeakSensor {
public:
    using WaterLeakSensor::lastState;
    using WaterLeakSensor::processEvent;
    using WaterLeakSensor::getDefaultSamplingIntervalInMilliseconds;
    using WaterLeakSensor::getMinimalSamplingIntervalInMilliseconds;

    TestableWaterLeakSensor(int signalGpioPin, EventHandler* parentHandler = nullptr)
        : WaterLeakSensor(signalGpioPin, parentHandler) {}
};

void resetFakeState() {
    for (int i = 0; i < 64; i++) {
        fakePinModeCalls[i].pin = 0;
        fakePinModeCalls[i].mode = 0;
    }
    fakePinModeCallCount = 0;
    fakeDigitalReadValue = HIGH;
}

void testConstructorInitializesState() {
    std::cout << "Test: Constructor initializes state correctly... ";
    resetFakeState();
    
    TestableWaterLeakSensor sensor(23);
    
    assert(sensor.getPin() == 23);
    assert(sensor.lastState == false);
    assert(sensor.isWaterLeakDetected() == false);
    std::cout << "PASSED" << std::endl;
}

void testLeakDetectedEmitsEvent() {
    std::cout << "Test: Leak detected emits WATER_LEAK_DETECTED_EVENT... ";
    resetFakeState();
    fakeDigitalReadValue = LOW; // water detected
    
    TestableWaterLeakSensor sensor(23);
    
    Event event(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    sensor.processEvent(event);
    
    assert(event.identifier == WaterLeakSensor::WATER_LEAK_DETECTED_EVENT_IDENTIFIER);
    assert(event.sourceId == 23);
    assert(sensor.isWaterLeakDetected() == true);
    assert(sensor.lastState == true);
    std::cout << "PASSED" << std::endl;
}

void testLeakClearedEmitsEvent() {
    std::cout << "Test: Leak cleared emits WATER_LEAK_CLEARED_EVENT... ";
    resetFakeState();
    fakeDigitalReadValue = LOW; // start with leak
    
    TestableWaterLeakSensor sensor(23);
    
    // First reading: leak detected
    Event event1(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    sensor.processEvent(event1);
    assert(event1.identifier == WaterLeakSensor::WATER_LEAK_DETECTED_EVENT_IDENTIFIER);
    
    // Second reading: leak cleared
    fakeDigitalReadValue = HIGH;
    Event event2(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    sensor.processEvent(event2);
    
    assert(event2.identifier == WaterLeakSensor::WATER_LEAK_CLEARED_EVENT_IDENTIFIER);
    assert(event2.sourceId == 23);
    assert(sensor.isWaterLeakDetected() == false);
    assert(sensor.lastState == false);
    std::cout << "PASSED" << std::endl;
}

void testNoLeakSuppressesPropagation() {
    std::cout << "Test: No leak suppresses propagation... ";
    resetFakeState();
    
    TestableWaterLeakSensor sensor(23);
    
    Event event(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    sensor.processEvent(event);
    
    assert(event.identifier == WaterLeakSensor::PROPAGATION_SUPPRESSION_IDENTIFIER);
    assert(sensor.isWaterLeakDetected() == false);
    std::cout << "PASSED" << std::endl;
}

void testLeakStillPresentSuppressesPropagation() {
    std::cout << "Test: Leak still present suppresses propagation... ";
    resetFakeState();
    fakeDigitalReadValue = LOW; // leak
    
    TestableWaterLeakSensor sensor(23);
    
    // First reading: leak detected
    Event event1(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    sensor.processEvent(event1);
    assert(event1.identifier == WaterLeakSensor::WATER_LEAK_DETECTED_EVENT_IDENTIFIER);
    
    // Second reading: leak still present
    Event event2(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    sensor.processEvent(event2);
    
    assert(event2.identifier == WaterLeakSensor::PROPAGATION_SUPPRESSION_IDENTIFIER);
    assert(sensor.isWaterLeakDetected() == true);
    std::cout << "PASSED" << std::endl;
}

void testSamplingIntervalDefaults() {
    std::cout << "Test: Sampling interval defaults are correct... ";
    resetFakeState();
    
    TestableWaterLeakSensor sensor(23);
    
    assert(sensor.getDefaultSamplingIntervalInMilliseconds() == 100);
    assert(sensor.getMinimalSamplingIntervalInMilliseconds() == 20);
    std::cout << "PASSED" << std::endl;
}

int main() {
    std::cout << "=== WaterLeakSensor Test Suite ===" << std::endl;
    
    testConstructorInitializesState();
    testLeakDetectedEmitsEvent();
    testLeakClearedEmitsEvent();
    testNoLeakSuppressesPropagation();
    testLeakStillPresentSuppressesPropagation();
    testSamplingIntervalDefaults();
    
    std::cout << "=== All 6 WaterLeakSensor tests PASSED ===" << std::endl;
    return 0;
}
