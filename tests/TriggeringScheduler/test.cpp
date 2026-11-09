/**
 * @file test.cpp
 * @brief Host unit tests for the Triggering Scheduler (W13).
 *
 * Validates interrupt-driven sensor registration, trigger handling,
 * removal, hybrid sensor registration, and trigger table capacity.
 */

#include "fakes/Arduino.h"
#include "Event.h"
#include "Sensor.h"
#include "Device.h"
#include <cassert>
#include <cstdio>
#include <cstring>

namespace {

// =========================================================================
// Test fixture: minimal Sensor subclass for triggering tests
// =========================================================================

class TestTriggeredSensor : public Sensor {
    mutable int eventCount = 0;

protected:
    void processEvent(Event& event) override {
        eventCount++;
        event.value = 42.0f;
        reportReadSuccess(event);
    }
    uint32_t getDefaultSamplingIntervalInMilliseconds() const override { return 1000; }
    uint32_t getMinimalSamplingIntervalInMilliseconds() const override { return 100; }

public:
    explicit TestTriggeredSensor(int gpioPin, EventHandler* parent = nullptr)
        : Sensor(gpioPin, parent) {}
    TestTriggeredSensor() : Sensor(0, nullptr) {}

    [[nodiscard]] int getEventCount() const { return eventCount; }
    void resetEventCount() { eventCount = 0; }
};

// =========================================================================
// Test fixture: minimal Device subclass for triggering tests
// =========================================================================

class TestDevice : public Device {
public:
    TestDevice() : Device(100) {
        // Initialize trigger table to known state
        std::memset(triggerTable, 0, sizeof(triggerTable));
    }
};

// =========================================================================
// Tests
// =========================================================================

[[nodiscard]] int main() {
    std::printf("Running TriggeringScheduler tests...\n");

    // =========================================================================
    // Test 1: Sensor getPin() returns assigned pin
    // =========================================================================
    {
        TestTriggeredSensor sensor(4);
        assert(sensor.getPin() == 4);
        TestTriggeredSensor sensor2(16);
        assert(sensor2.getPin() == 16);
        std::printf("  [PASS] Sensor getPin() returns assigned pin\n");
    }

    // =========================================================================
    // Test 2: TriggerEntry stores interrupt mode in Device, not Sensor
    // =========================================================================
    {
        TestDevice device;
        TestTriggeredSensor sensor(4);

        uint32_t handle = device.appendSensorToTriggeringScheduler(
            &sensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, RISING);

        assert(handle == 1);
        // Interrupt mode is stored in TriggerEntry, not on the sensor
        assert(device.triggerTable[0].interruptMode == RISING);
        std::printf("  [PASS] TriggerEntry stores interrupt mode in Device\n");
    }

    // =========================================================================
    // Test 3: appendSensorToTriggeringScheduler registers sensor
    // =========================================================================
    {
        TestDevice device;
        TestTriggeredSensor sensor(4);

        uint32_t handle = device.appendSensorToTriggeringScheduler(
            &sensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, RISING);

        assert(handle != 0);
        assert(handle == 1); // First slot, handle = index + 1
        assert(device.triggerTable[0].sensor == &sensor);
        assert(device.triggerTable[0].eventIdentifier == Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
        assert(device.triggerTable[0].interruptMode == RISING);
        std::printf("  [PASS] appendSensorToTriggeringScheduler registers sensor\n");
    }

    // =========================================================================
    // Test 4: Multiple sensors register in separate slots
    // =========================================================================
    {
        TestDevice device;
        TestTriggeredSensor sensor1(4);
        TestTriggeredSensor sensor2(5);
        TestTriggeredSensor sensor3(16);

        uint32_t handle1 = device.appendSensorToTriggeringScheduler(
            &sensor1, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, RISING);
        uint32_t handle2 = device.appendSensorToTriggeringScheduler(
            &sensor2, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, FALLING);
        uint32_t handle3 = device.appendSensorToTriggeringScheduler(
            &sensor3, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, CHANGE);

        assert(handle1 == 1);
        assert(handle2 == 2);
        assert(handle3 == 3);
        assert(device.triggerTable[0].interruptMode == RISING);
        assert(device.triggerTable[1].interruptMode == FALLING);
        assert(device.triggerTable[2].interruptMode == CHANGE);
        std::printf("  [PASS] Multiple sensors register in separate slots\n");
    }

    // =========================================================================
    // Test 5: Trigger table capacity limit (MAX_TRIGGERED_SENSORS = 4)
    // =========================================================================
    {
        TestDevice device;
        TestTriggeredSensor sensors[5];
        for (int i = 0; i < 5; i++) {
            sensors[i] = TestTriggeredSensor(i + 4);
        }

        uint32_t handles[5] = {};
        for (int i = 0; i < 5; i++) {
            handles[i] = device.appendSensorToTriggeringScheduler(
                &sensors[i], Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, CHANGE);
        }

        // First 4 should succeed, 5th should fail (table full)
        assert(handles[0] == 1);
        assert(handles[1] == 2);
        assert(handles[2] == 3);
        assert(handles[3] == 4);
        assert(handles[4] == 0); // Table full
        std::printf("  [PASS] Trigger table capacity limit enforced\n");
    }

    // =========================================================================
    // Test 6: removeSensorFromTriggeringScheduler clears registration
    // =========================================================================
    {
        TestDevice device;
        TestTriggeredSensor sensor(4);

        uint32_t handle = device.appendSensorToTriggeringScheduler(
            &sensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, RISING);
        assert(handle != 0);
        assert(device.triggerTable[0].sensor == &sensor);

        device.removeSensorFromTriggeringScheduler(handle);
        assert(device.triggerTable[0].sensor == nullptr);
        assert(device.triggerTable[0].eventIdentifier == -1);
        assert(device.triggerTable[0].interruptMode == CHANGE);
        std::printf("  [PASS] removeSensorFromTriggeringScheduler clears registration\n");
    }

    // =========================================================================
    // Test 7: removeSensorFromTriggeringScheduler with invalid handle is safe
    // =========================================================================
    {
        TestDevice device;
        // Should not crash with invalid handles
        device.removeSensorFromTriggeringScheduler(0);
        device.removeSensorFromTriggeringScheduler(99);
        std::printf("  [PASS] removeSensorFromTriggeringScheduler with invalid handle is safe\n");
    }

    // =========================================================================
    // Test 8: appendSensorToTriggeringScheduler rejects null sensor
    // =========================================================================
    {
        TestDevice device;
        uint32_t handle = device.appendSensorToTriggeringScheduler(
            nullptr, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, CHANGE);
        assert(handle == 0);
        std::printf("  [PASS] appendSensorToTriggeringScheduler rejects null sensor\n");
    }

    // =========================================================================
    // Test 9: appendSensorToTriggeringScheduler rejects invalid event ID
    // =========================================================================
    {
        TestDevice device;
        TestTriggeredSensor sensor(4);
        uint32_t handle = device.appendSensorToTriggeringScheduler(
            &sensor, -1, CHANGE);
        assert(handle == 0);
        std::printf("  [PASS] appendSensorToTriggeringScheduler rejects invalid event ID\n");
    }

    // =========================================================================
    // Test 10: onSensorTrigger marks sensor as triggered
    // =========================================================================
    {
        TestDevice device;
        TestTriggeredSensor sensor(4);

        device.appendSensorToTriggeringScheduler(
            &sensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, RISING);

        // Simulate the ISR being called
        Device::activeDevice = &device;
        Device::onSensorTrigger(1);

        // Verify the trigger flag was set
        assert(device.triggerTable[0].wasTriggered);
        std::printf("  [PASS] onSensorTrigger marks sensor as triggered\n");
    }

    // =========================================================================
    // Test 11: processTriggeredSensors processes triggered sensors
    // =========================================================================
    {
        TestDevice device;
        TestTriggeredSensor sensor(4);

        device.appendSensorToTriggeringScheduler(
            &sensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, RISING);

        // Simulate the ISR being called
        Device::activeDevice = &device;
        Device::onSensorTrigger(1);
        assert(device.triggerTable[0].wasTriggered);

        // Process triggered sensors
        device.processTriggeredSensors();

        // Verify the sensor's processEvent was called and flag was cleared
        assert(sensor.getEventCount() == 1);
        assert(!device.triggerTable[0].wasTriggered);
        std::printf("  [PASS] processTriggeredSensors processes triggered sensors\n");
    }

    // =========================================================================
    // Test 12: processTriggeredSensors handles multiple triggered sensors
    // =========================================================================
    {
        TestDevice device;
        TestTriggeredSensor sensor1(4);
        TestTriggeredSensor sensor2(5);

        device.appendSensorToTriggeringScheduler(
            &sensor1, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, RISING);
        device.appendSensorToTriggeringScheduler(
            &sensor2, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, FALLING);

        // Simulate both ISRs being called
        Device::activeDevice = &device;
        Device::onSensorTrigger(1);
        Device::onSensorTrigger(2);

        // Process triggered sensors
        device.processTriggeredSensors();

        // Verify both sensors were processed
        assert(sensor1.getEventCount() == 1);
        assert(sensor2.getEventCount() == 1);
        assert(!device.triggerTable[0].wasTriggered);
        assert(!device.triggerTable[1].wasTriggered);
        std::printf("  [PASS] processTriggeredSensors handles multiple triggered sensors\n");
    }

    // =========================================================================
    // Test 13: processTriggeredSensors is safe when no sensors triggered
    // =========================================================================
    {
        TestDevice device;
        // Should not crash when no sensors are registered
        device.processTriggeredSensors();
        std::printf("  [PASS] processTriggeredSensors is safe when no sensors triggered\n");
    }

    // =========================================================================
    // Test 14: appendToBothSchedulers registers with both schedulers
    // =========================================================================
    {
        TestDevice device;
        TestTriggeredSensor sensor(4);

        bool result = device.appendToBothSchedulers(
            &sensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, RISING);

        // appendToBothSchedulers calls appendSensorToScheduler and
        // appendSensorToTriggeringScheduler. Both should succeed.
        assert(result == true);
        assert(device.triggerTable[0].sensor == &sensor);
        assert(device.triggerTable[0].interruptMode == RISING);
        std::printf("  [PASS] appendToBothSchedulers registers with both schedulers\n");
    }

    // =========================================================================
    // Test 15: attachInterrupt mock stores handler and mode
    // =========================================================================
    {
        // Reset mock state
        std::memset(fakeInterruptHandlers, 0, sizeof(fakeInterruptHandlers));
        std::memset(fakeInterruptModes, 0, sizeof(fakeInterruptModes));

        TestDevice device;
        TestTriggeredSensor sensor(4);

        device.appendSensorToTriggeringScheduler(
            &sensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, RISING);

        // Verify the interrupt was attached with the correct mode
        assert(fakeInterruptModes[4] == RISING);
        assert(fakeInterruptHandlers[4] != nullptr);
        std::printf("  [PASS] attachInterrupt mock stores handler and mode\n");
    }

    // =========================================================================
    // Test 16: detachInterrupt mock clears handler on removal
    // =========================================================================
    {
        TestDevice device;
        TestTriggeredSensor sensor(5);

        uint32_t handle = device.appendSensorToTriggeringScheduler(
            &sensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, FALLING);
        assert(fakeInterruptHandlers[5] != nullptr);

        device.removeSensorFromTriggeringScheduler(handle);
        assert(fakeInterruptHandlers[5] == nullptr);
        assert(fakeInterruptModes[5] == 0);
        std::printf("  [PASS] detachInterrupt mock clears handler on removal\n");
    }

    std::printf("All TriggeringScheduler tests passed successfully!\n");
    return 0;
}

} // namespace
