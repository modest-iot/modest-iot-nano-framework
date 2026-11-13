#include <time.h>
/**
 * @file test.cpp
 * @brief Host tests for DoorWindowSensor (door/window open/close detection).
 * @ingroup sensors_module
 */

#include "DoorWindowSensor.h"
#include "Event.h"
#include "Sensor.h"
#include <cassert>
#include <iostream>
#include "../fakes/Arduino.h"

class TestableDoorWindowSensor : public DoorWindowSensor {
public:
    using DoorWindowSensor::lastState;
    using DoorWindowSensor::processEvent;
    using DoorWindowSensor::getDefaultSamplingIntervalInMilliseconds;
    using DoorWindowSensor::getMinimalSamplingIntervalInMilliseconds;

    TestableDoorWindowSensor(int signalGpioPin, EventHandler* parentHandler = nullptr)
        : DoorWindowSensor(signalGpioPin, parentHandler) {}
};

void resetFakeState() {
    fakePinModeCallCount = 0;
    fakeDigitalReadValue = HIGH;
}

void testConstructorInitializesState() {
    std::cout << "Test: Constructor initializes state correctly... ";
    resetFakeState();
    
    TestableDoorWindowSensor sensor(23);
    
    assert(sensor.getPin() == 23);
    assert(sensor.lastState == false);
    assert(sensor.isDoorOpen() == false);
    std::cout << "PASSED" << std::endl;
}

void testDoorOpenedEmitsEvent() {
    std::cout << "Test: Door opened emits DOOR_OPENED_EVENT... ";
    resetFakeState();
    fakeDigitalReadValue = HIGH; // door open
    
    TestableDoorWindowSensor sensor(23);
    
    Event event(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    sensor.processEvent(event);
    
    assert(event.identifier == DoorWindowSensor::DOOR_OPENED_EVENT_IDENTIFIER);
    assert(event.sourceId == 23);
    assert(sensor.isDoorOpen() == true);
    assert(sensor.lastState == true);
    std::cout << "PASSED" << std::endl;
}

void testDoorClosedEmitsEvent() {
    std::cout << "Test: Door closed emits DOOR_CLOSED_EVENT... ";
    resetFakeState();
    fakeDigitalReadValue = HIGH; // start with door open
    
    TestableDoorWindowSensor sensor(23);
    
    // First reading: door opened
    Event event1(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    sensor.processEvent(event1);
    assert(event1.identifier == DoorWindowSensor::DOOR_OPENED_EVENT_IDENTIFIER);
    
    // Second reading: door closed
    fakeDigitalReadValue = LOW;
    Event event2(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    sensor.processEvent(event2);
    
    assert(event2.identifier == DoorWindowSensor::DOOR_CLOSED_EVENT_IDENTIFIER);
    assert(event2.sourceId == 23);
    assert(sensor.isDoorOpen() == false);
    assert(sensor.lastState == false);
    std::cout << "PASSED" << std::endl;
}

void testDoorClosedSuppressesPropagation() {
    std::cout << "Test: Door closed suppresses propagation... ";
    resetFakeState();
    fakeDigitalReadValue = LOW; // door closed
    
    TestableDoorWindowSensor sensor(23);
    
    Event event(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    sensor.processEvent(event);
    
    assert(event.identifier == DoorWindowSensor::PROPAGATION_SUPPRESSION_IDENTIFIER);
    assert(sensor.isDoorOpen() == false);
    std::cout << "PASSED" << std::endl;
}

void testDoorStillOpenSuppressesPropagation() {
    std::cout << "Test: Door still open suppresses propagation... ";
    resetFakeState();
    fakeDigitalReadValue = HIGH; // door open
    
    TestableDoorWindowSensor sensor(23);
    
    // First reading: door opened
    Event event1(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    sensor.processEvent(event1);
    assert(event1.identifier == DoorWindowSensor::DOOR_OPENED_EVENT_IDENTIFIER);
    
    // Second reading: door still open
    Event event2(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    sensor.processEvent(event2);
    
    assert(event2.identifier == DoorWindowSensor::PROPAGATION_SUPPRESSION_IDENTIFIER);
    assert(sensor.isDoorOpen() == true);
    std::cout << "PASSED" << std::endl;
}

void testSamplingIntervalDefaults() {
    std::cout << "Test: Sampling interval defaults are correct... ";
    resetFakeState();
    
    TestableDoorWindowSensor sensor(23);
    
    assert(sensor.getDefaultSamplingIntervalInMilliseconds() == 100);
    assert(sensor.getMinimalSamplingIntervalInMilliseconds() == 20);
    std::cout << "PASSED" << std::endl;
}

int main() {
    std::cout << "=== DoorWindowSensor Test Suite ===" << std::endl;
    
    testConstructorInitializesState();
    testDoorOpenedEmitsEvent();
    testDoorClosedEmitsEvent();
    testDoorClosedSuppressesPropagation();
    testDoorStillOpenSuppressesPropagation();
    testSamplingIntervalDefaults();
    
    std::cout << "=== All 6 DoorWindowSensor tests PASSED ===" << std::endl;
    return 0;
}
