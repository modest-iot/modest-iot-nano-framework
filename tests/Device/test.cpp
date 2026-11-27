#include <time.h>
/**
 * @file test.cpp
 * @brief Host unit tests for the Device component.
 *
 * Tests static methods and constants that don't require FreeRTOS.
 * Device requires FreeRTOS tasks/queues, so we test only the static API.
 */

#include "Arduino.h"
#include "Device.h"
#include "FakeI2cBus.h"
#include <cassert>
#include <cstdio>
#include <vector>

namespace {

class CompositionSensor final : public Sensor {
public:
    CompositionSensor() : Sensor(1, nullptr) {}

protected:
    void processEvent(Event&) override {}
    uint32_t getDefaultSamplingIntervalInMilliseconds() const override { return 1000; }
    uint32_t getMinimalSamplingIntervalInMilliseconds() const override { return 250; }
};

class CompositionPeripheral {
public:
    CompositionPeripheral(I2cBus& bus, std::vector<int>& startupOrder, const int order)
        : bus(bus), startupOrder(startupOrder), order(order) {}

    bool start() {
        startupOrder.push_back(order);
        return bus.isReady() && startResult;
    }

    I2cBus& bus;
    std::vector<int>& startupOrder;
    int order;
    bool startResult = true;
};

class CompositionDevice final : public Device {
public:
    explicit CompositionDevice(std::vector<int>& startupOrder)
        : Device(100),
          sensorPeripheral(bus, startupOrder, 1),
          displayPeripheral(bus, startupOrder, 2),
          startupOrder(startupOrder) {}

    void on(Event) override {}
    void handle(Command) override {}

    bool start() {
        if (bus.start({-1, -1, 100000}) != I2cStatus::Ok) {
            return false;
        }
        if (!sensorPeripheral.start() || !displayPeripheral.start()) {
            return false;
        }
        if (!appendSensorToScheduler(&sensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER,
                                     1000, 250)) {
            return false;
        }
        startupOrder.push_back(3);
        initializeAsynchronousEngine();
        startupOrder.push_back(4);
        return true;
    }

    bool canRegisterSensor() {
        return appendSensorToScheduler(&sensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER,
                                       1000, 250);
    }

    FakeI2cBus bus;
    CompositionPeripheral sensorPeripheral;
    CompositionPeripheral displayPeripheral;
    CompositionSensor sensor;

private:
    std::vector<int>& startupOrder;
};

void testComposedDeviceStartupOrderAndPeripheralFailure() {
    std::vector<int> startupOrder;
    {
        CompositionDevice device(startupOrder);
        device.displayPeripheral.startResult = false;

        assert(!device.start());
        assert(device.bus.startCount == 1);
        assert(device.sensorPeripheral.bus.isReady());
        assert(startupOrder == std::vector<int>({1, 2}));
        assert(device.canRegisterSensor());
    }

    startupOrder.clear();
    {
        CompositionDevice device(startupOrder);

        assert(device.start());
        assert(device.bus.startCount == 1);
        assert(&device.sensorPeripheral.bus == &device.bus);
        assert(&device.displayPeripheral.bus == &device.bus);
        assert(startupOrder == std::vector<int>({1, 2, 3, 4}));
        assert(!device.canRegisterSensor());
    }
}

} // namespace

int main() {
    std::printf("Running Device tests...\n");
    testComposedDeviceStartupOrderAndPeripheralFailure();

    // Test 1: Default telemetry schema version is nullptr
    {
        const char* version = Device::getTelemetrySchemaVersion();
        assert(version == nullptr);
        std::printf("  [PASS] Default telemetry schema version is nullptr\n");
    }

    // Test 2: Set and get telemetry schema version
    {
        Device::setTelemetrySchemaVersion("2.0");
        assert(Device::getTelemetrySchemaVersion() != nullptr);
        std::printf("  [PASS] Set and get telemetry schema version\n");
    }

    // Test 3: Reset telemetry schema version to nullptr
    {
        Device::setTelemetrySchemaVersion(nullptr);
        assert(Device::getTelemetrySchemaVersion() == nullptr);
        std::printf("  [PASS] Reset telemetry schema version to nullptr\n");
    }

    // Test 4: Default firmware version is nullptr
    {
        const char* version = Device::getFirmwareVersion();
        assert(version == nullptr);
        std::printf("  [PASS] Default firmware version is nullptr\n");
    }

    // Test 5: Set and get firmware version
    {
        Device::setFirmwareVersion("1.2.3");
        assert(Device::getFirmwareVersion() != nullptr);
        std::printf("  [PASS] Set and get firmware version\n");
    }

    // Test 6: Reset firmware version to nullptr
    {
        Device::setFirmwareVersion(nullptr);
        assert(Device::getFirmwareVersion() == nullptr);
        std::printf("  [PASS] Reset firmware version to nullptr\n");
    }

    // Test 7: MAX_TRIGGERED_SENSORS constant
    {
        assert(Device::MAX_TRIGGERED_SENSORS == 4);
        std::printf("  [PASS] MAX_TRIGGERED_SENSORS constant\n");
    }

    // Test 8: DeviceConstants::MAX_GPIO_INTERRUPTS
    {
        assert(DeviceConstants::MAX_GPIO_INTERRUPTS == 64);
        std::printf("  [PASS] DeviceConstants::MAX_GPIO_INTERRUPTS\n");
    }

    // Test 9: NotificationBitFlag values
    {
        assert(static_cast<uint32_t>(Device::NotificationBitFlag::TimerTickArrived) == 0x1);
        assert(static_cast<uint32_t>(Device::NotificationBitFlag::TerminationSignal) == 0x2);
        assert(static_cast<uint32_t>(Device::NotificationBitFlag::SensorTrigger) == 0x4);
        assert(static_cast<uint32_t>(Device::NotificationBitFlag::CommandAvailable) == 0x8);
        std::printf("  [PASS] NotificationBitFlag values\n");
    }

    // Test 10: PowerMode enum values
    {
        assert(static_cast<uint8_t>(Device::PowerMode::Periodic) == 0);
        assert(static_cast<uint8_t>(Device::PowerMode::Tickless) == 1);
        assert(static_cast<uint8_t>(Device::PowerMode::DeepSleep) == 2);
        std::printf("  [PASS] PowerMode enum values\n");
    }

    // Test 11: CommandEntry default constructor
    {
        Device::CommandEntry entry;
        assert(entry.targetHandler == nullptr);
        assert(entry.command.identifier == -1);
        std::printf("  [PASS] CommandEntry default constructor\n");
    }

    // Test 12: CommandEntry parameterized constructor
    {
        struct FakeHandler : CommandHandler {
            void handle(const Command) override {}
        };
        FakeHandler handler;
        Device::CommandEntry entry(&handler, Command{42});
        assert(entry.targetHandler == &handler);
        assert(entry.command.identifier == 42);
        std::printf("  [PASS] CommandEntry parameterized constructor\n");
    }

    // Test 13: DiagnosticsSnapshot high-priority fields default to zero
    {
        DiagnosticsSnapshot snapshot;
        assert(snapshot.highPriorityEnqueued == 0);
        assert(snapshot.highPriorityDropped == 0);
        assert(snapshot.highPriorityQueueHighWaterMark == 0);
        std::printf("  [PASS] DiagnosticsSnapshot high-priority fields default to zero\n");
    }

    // Test 14: TelemetryPackage high-priority in JSON output
    {
        class HighPriorityTelemetryPackage : public TelemetryPackage {
        public:
            void write(TelemetryWriter& writer) const override {
                writer.field("code", static_cast<int32_t>(42));
            }
        };
        HighPriorityTelemetryPackage highPriority;
        highPriority.setEnvelopeMetadata(1000, 1);
        highPriority.markAsHighPriority();
        String json = highPriority.toJsonString();
        assert(json.find("\"priority\"") != String::npos);
        assert(json.find("\"high\"") != String::npos);
        assert(json.find("\"code\"") != String::npos);
        std::printf("  [PASS] TelemetryPackage high-priority in JSON output\n");
    }

    // Test 15: TelemetryPackage without high-priority has no priority field
    {
        class NormalTelemetryPackage : public TelemetryPackage {
        public:
            void write(TelemetryWriter& writer) const override {
                writer.field("value", static_cast<int32_t>(1));
            }
        };
        NormalTelemetryPackage normal;
        normal.setEnvelopeMetadata(1000, 2);
        String json = normal.toJsonString();
        assert(json.find("\"priority\"") == String::npos);
        std::printf("  [PASS] TelemetryPackage without high-priority has no priority field\n");
    }

    std::printf("All Device tests passed successfully!\n");
    return 0;
}
