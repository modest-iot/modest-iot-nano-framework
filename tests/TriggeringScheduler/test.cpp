#include <time.h>
/**
 * @file test.cpp
 * @brief Host unit tests for the Triggering Scheduler (W13).
 *
 * Validates interrupt-driven sensor registration, trigger handling,
 * removal, hybrid sensor registration, and trigger table capacity.
 */

#include "Arduino.h"
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
    TestDevice() : Device(100, 0) {}
    void on(Event) override {}
    void processQueuedTelemetryData(const TelemetryPackage*) const override {}

    bool initializeEngine() { return Device::initializeAsynchronousEngine(); }
    bool appendSamplingSensor(EventHandler* sensor, int eventIdentifier) {
        return Device::appendSensorToScheduler(sensor, eventIdentifier);
    }

    // Forward protected Device methods for test access
    uint32_t appendSensorToTriggeringScheduler(Sensor* s, int e, int m = CHANGE) override {
        return Device::appendSensorToTriggeringScheduler(s, e, m);
    }
    void removeSensorFromTriggeringScheduler(uint32_t h) override {
        Device::removeSensorFromTriggeringScheduler(h);
    }
    bool appendToBothSchedulers(Sensor* s, int e, int m = CHANGE) override {
        return Device::appendToBothSchedulers(s, e, m);
    }
};

void resetInterruptFakes() {
    std::memset(fakeInterruptHandlers, 0, sizeof(fakeInterruptHandlers));
    std::memset(fakeInterruptHandlersArg, 0, sizeof(fakeInterruptHandlersArg));
    std::memset(fakeInterruptArgs, 0, sizeof(fakeInterruptArgs));
    std::memset(fakeInterruptModes, 0, sizeof(fakeInterruptModes));
}

void startDevice(TestDevice& device) {
    assert(device.initializeEngine());
}

void fireInterrupt(const int pin) {
    assert(pin >= 0 && pin < 64);
    assert(fakeInterruptHandlersArg[pin] != nullptr);
    fakeInterruptHandlersArg[pin](fakeInterruptArgs[pin]);
}

// =========================================================================
// Tests
// =========================================================================

} // anonymous namespace

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
    // Test 10: source-specific callbacks mark only their own trigger entry
    // =========================================================================
    {
        TestDevice device;
        TestTriggeredSensor sensor(4);

        device.appendSensorToTriggeringScheduler(
            &sensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, RISING);

        resetFakeFreeRtosState();
        resetInterruptFakes();
        startDevice(device);
        fireInterrupt(4);

        // Verify the trigger flag was set
        assert(device.triggerTable[0].wasTriggered);
        assert(fakeFreeRtosState.isrNotificationCalls == 1);
        device.removeSensorFromTriggeringScheduler(1);
        std::printf("  [PASS] source-specific callback marks its own trigger entry\n");
    }

    // =========================================================================
    // Test 11: processTriggeredSensors processes triggered sensors
    // =========================================================================
    {
        TestDevice device;
        TestTriggeredSensor sensor(4);

        device.appendSensorToTriggeringScheduler(
            &sensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, RISING);

        resetFakeFreeRtosState();
        resetInterruptFakes();
        startDevice(device);
        fireInterrupt(4);
        assert(device.triggerTable[0].wasTriggered);

        // Process triggered sensors
        device.processTriggeredSensors();

        // Verify the sensor's processEvent was called and flag was cleared
        assert(sensor.getEventCount() == 1);
        assert(!device.triggerTable[0].wasTriggered);
        device.removeSensorFromTriggeringScheduler(1);
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

        resetFakeFreeRtosState();
        resetInterruptFakes();
        startDevice(device);
        fireInterrupt(4);
        fireInterrupt(4);
        fireInterrupt(5);

        // Process triggered sensors
        device.processTriggeredSensors();

        // Verify both sensors were processed
        assert(sensor1.getEventCount() == 1);
        assert(sensor2.getEventCount() == 1);
        assert(!device.triggerTable[0].wasTriggered);
        assert(!device.triggerTable[1].wasTriggered);
        assert(fakeFreeRtosState.isrNotificationCalls == 3);
        device.removeSensorFromTriggeringScheduler(1);
        device.removeSensorFromTriggeringScheduler(2);
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
    // Test 15: attachInterruptArg is deferred until the sampler exists
    // =========================================================================
    {
        resetFakeFreeRtosState();
        resetInterruptFakes();

        TestDevice device;
        TestTriggeredSensor sensor(4);

        device.appendSensorToTriggeringScheduler(
            &sensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, RISING);

        assert(fakeInterruptHandlersArg[4] == nullptr);
        startDevice(device);
        assert(fakeInterruptModes[4] == RISING);
        assert(fakeInterruptHandlersArg[4] != nullptr);
        assert(fakeInterruptArgs[4] == &device.triggerTable[0]);
        device.removeSensorFromTriggeringScheduler(1);
        std::printf("  [PASS] attachInterruptArg is deferred until the sampler exists\n");
    }

    // =========================================================================
    // Test 16: detachInterrupt mock clears handler on removal
    // =========================================================================
    {
        TestDevice device;
        TestTriggeredSensor sensor(5);

        uint32_t handle = device.appendSensorToTriggeringScheduler(
            &sensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, FALLING);
        resetFakeFreeRtosState();
        resetInterruptFakes();
        startDevice(device);
        assert(fakeInterruptHandlersArg[5] != nullptr);
        const auto handler = fakeInterruptHandlersArg[5];
        void* const context = fakeInterruptArgs[5];
        const int notificationsBeforeRemoval = fakeFreeRtosState.isrNotificationCalls;

        device.removeSensorFromTriggeringScheduler(handle);
        assert(fakeInterruptHandlersArg[5] == nullptr);
        handler(context);
        assert(fakeFreeRtosState.isrNotificationCalls == notificationsBeforeRemoval);
        assert(fakeInterruptModes[5] == 0);
        std::printf("  [PASS] removal detaches and disables a stale callback context\n");
    }

    // =========================================================================
    // Test 17: callbacks route to separate devices and coalesce repeated edges
    // =========================================================================
    {
        resetFakeFreeRtosState();
        resetInterruptFakes();
        TestDevice firstDevice;
        TestDevice secondDevice;
        TestTriggeredSensor firstSensor(4);
        TestTriggeredSensor secondSensor(5);
        assert(firstDevice.appendSensorToTriggeringScheduler(
            &firstSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, RISING) != 0);
        assert(secondDevice.appendSensorToTriggeringScheduler(
            &secondSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, FALLING) != 0);

        startDevice(firstDevice);
        startDevice(secondDevice);
        fireInterrupt(4);
        fireInterrupt(4);
        fireInterrupt(5);

        assert(fakeFreeRtosState.isrNotificationCalls == 3);
        assert(fakeFreeRtosState.isrNotificationTargets[0] == &fakeTaskHandles[0]);
        assert(fakeFreeRtosState.isrNotificationTargets[1] == &fakeTaskHandles[0]);
        assert(fakeFreeRtosState.isrNotificationTargets[2] == &fakeTaskHandles[2]);
        firstDevice.processTriggeredSensors();
        secondDevice.processTriggeredSensors();
        assert(firstSensor.getEventCount() == 1);
        assert(secondSensor.getEventCount() == 1);
        firstDevice.removeSensorFromTriggeringScheduler(1);
        secondDevice.removeSensorFromTriggeringScheduler(1);
        std::printf("  [PASS] callbacks route per device and repeated edges coalesce\n");
    }

    // =========================================================================
    // Test 18: invalid pins and duplicate pin registrations are rejected
    // =========================================================================
    {
        TestDevice device;
        TestTriggeredSensor negativePin(-1);
        TestTriggeredSensor outOfRangePin(DeviceConstants::MAX_GPIO_INTERRUPTS);
        TestTriggeredSensor validPin(12);

        assert(device.appendSensorToTriggeringScheduler(
            &negativePin, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, RISING) == 0);
        assert(device.appendSensorToTriggeringScheduler(
            &outOfRangePin, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, RISING) == 0);
        assert(device.appendSensorToTriggeringScheduler(
            &validPin, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, RISING) != 0);
        TestTriggeredSensor duplicatePin(12);
        assert(device.appendSensorToTriggeringScheduler(
            &duplicatePin, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, RISING) == 0);
        std::printf("  [PASS] invalid and duplicate trigger registrations are rejected\n");
    }

    // =========================================================================
    // Test 19: failed combined registrations do not leave either scheduler half
    // =========================================================================
    {
        TestDevice device;
        TestTriggeredSensor triggerSensors[] = {
            TestTriggeredSensor(20), TestTriggeredSensor(21), TestTriggeredSensor(22),
            TestTriggeredSensor(23), TestTriggeredSensor(24)
        };
        for (int index = 0; index < 4; ++index) {
            assert(device.appendSensorToTriggeringScheduler(
                &triggerSensors[index], Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, RISING) != 0);
        }
        assert(!device.appendToBothSchedulers(
            &triggerSensors[4], Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, RISING));
        TestTriggeredSensor sampleSensors[9];
        for (int index = 0; index < 8; ++index) {
            assert(device.appendSamplingSensor(
                static_cast<EventHandler*>(&sampleSensors[index]),
                Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER));
        }
        assert(!device.appendSamplingSensor(
            static_cast<EventHandler*>(&sampleSensors[8]),
            Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER));
    }
    {
        TestDevice device;
        TestTriggeredSensor sampleSensors[8];
        for (auto& sampleSensor : sampleSensors) {
            assert(device.appendSamplingSensor(
                static_cast<EventHandler*>(&sampleSensor),
                Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER));
        }
        TestTriggeredSensor hybridSensor(25);
        assert(!device.appendToBothSchedulers(
            &hybridSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, RISING));
        for (const auto& entry : device.triggerTable) {
            assert(entry.sensor != &hybridSensor);
        }
    }
    {
        TestDevice device;
        TestTriggeredSensor invalidPin(-1);
        assert(!device.appendToBothSchedulers(
            &invalidPin, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, RISING));
        TestTriggeredSensor sampleSensors[9];
        for (int index = 0; index < 8; ++index) {
            assert(device.appendSamplingSensor(
                static_cast<EventHandler*>(&sampleSensors[index]),
                Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER));
        }
        assert(!device.appendSamplingSensor(
            static_cast<EventHandler*>(&sampleSensors[8]),
            Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER));
        std::printf("  [PASS] combined registration failures roll back partial state\n");
    }

    std::printf("All TriggeringScheduler tests passed successfully!\n");
    return 0;
}
