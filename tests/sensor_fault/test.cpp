/**
 * @file test.cpp
 * @brief Host unit tests for Sensor fault and recovery events.
 *
 * Tests the debounced threshold state machine: fault detection after N consecutive failures,
 * latching behavior (fault event emitted exactly once), and recovery event on successful read.
 */

#include <cstdio>
#include <cassert>
#include <cstring>

#include "../src/Event.h"
#include "../src/EventHandler.h"
#include "../src/Sensor.h"

// Anonymous namespace for internal linkage
namespace {

// Concrete test sensor that exposes fault helpers
class TestSensor : public Sensor {
public:
    using Sensor::Sensor; // Inherit constructors

    void processEvent(Event& event) override {
        // No-op for testing
        (void)event;
    }

    // Expose protected methods for testing
    using Sensor::reportReadSuccess;
    using Sensor::reportReadFailure;
    using Sensor::setFaultThreshold;
};

// Capturing handler to verify event propagation
class CapturingHandler : public EventHandler {
public:
    int lastEventId = -999;
    int lastSourceId = -999;
    int eventCount = 0;

    void on(Event event) override {
        lastEventId = event.identifier;
        lastSourceId = event.sourceId;
        eventCount++;
    }
};

// Test default state: fault detection disabled
void test_default_fault_threshold_disabled() {
    TestSensor sensor(4);
    CapturingHandler handler;
    sensor.setHandler(&handler);

    Event event(20);
    // With threshold 0 (default), fault event should never be emitted
    sensor.reportReadFailure(event);
    assert(event.identifier == 20); // Original event unchanged
    assert(handler.eventCount == 0); // No propagation because processEvent is no-op

    sensor.reportReadFailure(event);
    assert(event.identifier == 20);
}

// Test fault event emitted after reaching threshold
void test_fault_event_after_threshold() {
    TestSensor sensor(4);
    sensor.setFaultThreshold(3);

    Event event(20);
    // First failure - no fault yet
    sensor.reportReadFailure(event);
    assert(event.identifier == 20);

    // Second failure - no fault yet
    sensor.reportReadFailure(event);
    assert(event.identifier == 20);

    // Third failure - threshold reached, fault emitted
    sensor.reportReadFailure(event);
    assert(event.identifier == Sensor::SENSOR_FAULT_EVENT_IDENTIFIER);
    assert(event.sourceId == 4);
}

// Test fault event emitted exactly once (latching)
void test_fault_event_latching() {
    TestSensor sensor(5);
    sensor.setFaultThreshold(2);

    Event event(20);
    sensor.reportReadFailure(event);
    assert(event.identifier == 20);

    // Second failure triggers fault
    sensor.reportReadFailure(event);
    assert(event.identifier == Sensor::SENSOR_FAULT_EVENT_IDENTIFIER);

    // Third failure - fault already latched, no new fault event
    Event event2(20);
    sensor.reportReadFailure(event2);
    assert(event2.identifier == 20); // Original event unchanged, no re-emission
}

// Test recovery event clears fault state
void test_recovery_event_on_success() {
    TestSensor sensor(6);
    sensor.setFaultThreshold(2);

    // Trigger fault
    Event faultEvent(20);
    sensor.reportReadFailure(faultEvent);
    sensor.reportReadFailure(faultEvent);
    assert(faultEvent.identifier == Sensor::SENSOR_FAULT_EVENT_IDENTIFIER);

    // Successful read triggers recovery
    Event recoveryEvent(20);
    sensor.reportReadSuccess(recoveryEvent);
    assert(recoveryEvent.identifier == Sensor::SENSOR_RECOVERED_EVENT_IDENTIFIER);
    assert(recoveryEvent.sourceId == 6);
}

// Test recovery event only emitted once per fault cycle
void test_recovery_event_once_per_cycle() {
    TestSensor sensor(7);
    sensor.setFaultThreshold(2);

    // Trigger fault
    Event faultEvent(20);
    sensor.reportReadFailure(faultEvent);
    sensor.reportReadFailure(faultEvent);
    assert(faultEvent.identifier == Sensor::SENSOR_FAULT_EVENT_IDENTIFIER);

    // First success triggers recovery
    Event recoveryEvent(20);
    sensor.reportReadSuccess(recoveryEvent);
    assert(recoveryEvent.identifier == Sensor::SENSOR_RECOVERED_EVENT_IDENTIFIER);

    // Second success - no recovery event (already recovered)
    Event normalEvent(20);
    sensor.reportReadSuccess(normalEvent);
    assert(normalEvent.identifier == 20);
}

// Test failure counter resets on success
void test_failure_counter_resets_on_success() {
    TestSensor sensor(8);
    sensor.setFaultThreshold(3);

    // Two failures
    Event e1(20);
    sensor.reportReadFailure(e1);
    sensor.reportReadFailure(e1);

    // Success resets counter
    Event e2(20);
    sensor.reportReadSuccess(e2);
    assert(e2.identifier == 20); // No recovery (not faulted)

    // Need 3 failures again to trigger fault
    Event e3(20);
    sensor.reportReadFailure(e3);
    assert(e3.identifier == 20);

    Event e4(20);
    sensor.reportReadFailure(e4);
    assert(e4.identifier == 20);

    Event e5(20);
    sensor.reportReadFailure(e5);
    assert(e5.identifier == Sensor::SENSOR_FAULT_EVENT_IDENTIFIER);
}

// Test fault and recovery cycle can repeat
void test_fault_recovery_cycle_repeat() {
    TestSensor sensor(9);
    sensor.setFaultThreshold(2);

    // First cycle: fault -> recovery
    Event e1(20);
    sensor.reportReadFailure(e1);
    sensor.reportReadFailure(e1);
    assert(e1.identifier == Sensor::SENSOR_FAULT_EVENT_IDENTIFIER);

    Event e2(20);
    sensor.reportReadSuccess(e2);
    assert(e2.identifier == Sensor::SENSOR_RECOVERED_EVENT_IDENTIFIER);

    // Second cycle: fault -> recovery
    Event e3(20);
    sensor.reportReadFailure(e3);
    sensor.reportReadFailure(e3);
    assert(e3.identifier == Sensor::SENSOR_FAULT_EVENT_IDENTIFIER);

    Event e4(20);
    sensor.reportReadSuccess(e4);
    assert(e4.identifier == Sensor::SENSOR_RECOVERED_EVENT_IDENTIFIER);
}

// Test event identifiers are correct values
void test_event_identifier_values() {
    assert(Sensor::DATA_READ_EVENT_IDENTIFIER == 20);
    assert(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER == 21);
    assert(Sensor::SENSOR_FAULT_EVENT_IDENTIFIER == 22);
    assert(Sensor::SENSOR_RECOVERED_EVENT_IDENTIFIER == 23);
}

// Test sourceId contains sensor pin
void test_source_id_is_sensor_pin() {
    TestSensor sensor(12);
    sensor.setFaultThreshold(1);

    Event faultEvent(20);
    sensor.reportReadFailure(faultEvent);
    assert(faultEvent.sourceId == 12);

    Event recoveryEvent(20);
    sensor.reportReadSuccess(recoveryEvent);
    assert(recoveryEvent.sourceId == 12);
}

} // anonymous namespace

int main() {
    printf("Running Sensor fault and recovery tests...\n\n");

    test_default_fault_threshold_disabled();
    printf("[PASS] Default fault threshold is disabled\n");

    test_fault_event_after_threshold();
    printf("[PASS] Fault event emitted after threshold\n");

    test_fault_event_latching();
    printf("[PASS] Fault event latching works\n");

    test_recovery_event_on_success();
    printf("[PASS] Recovery event clears fault state\n");

    test_recovery_event_once_per_cycle();
    printf("[PASS] Recovery event emitted once per cycle\n");

    test_failure_counter_resets_on_success();
    printf("[PASS] Failure counter resets on success\n");

    test_fault_recovery_cycle_repeat();
    printf("[PASS] Fault/recovery cycle can repeat\n");

    test_event_identifier_values();
    printf("[PASS] Event identifier values are correct\n");

    test_source_id_is_sensor_pin();
    printf("[PASS] SourceId contains sensor pin\n");

    printf("\nAll sensor fault tests passed!\n");
    return 0;
}
