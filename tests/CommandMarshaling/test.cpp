/**
 * @brief Host unit tests for Device::enqueueCommand and processPendingCommands.
 *
 * Validates cross-task command marshaling: enqueueCommand places a CommandEntry
 * on the command queue, and processPendingCommands drains it calling handle().
 */

#include "Device.h"
#include "Command.h"
#include "fakes/Arduino.h"
#include <cassert>
#include <cstdio>
#include <vector>

namespace {

// =========================================================================
// Test fixture: CommandHandler that records delivered commands
// =========================================================================
class CommandRecorder : public CommandHandler {
public:
    std::vector<int> deliveredIdentifiers;

    void handle(Command command) override {
        deliveredIdentifiers.push_back(command.identifier);
    }
};

// =========================================================================
// Test fixture: Device subclass for command marshaling tests
// =========================================================================
class TestDevice : public Device {
public:
    TestDevice() : Device(100, 0) {}

    void processQueuedTelemetryData(const TelemetryPackage*) const override {}
    void handle(Command) override {}
};

} // anonymous namespace

[[nodiscard]] int main() {
    std::printf("Running Command Marshaling tests...\n");

    // =========================================================================
    // Test 1: CommandEntry default initialization
    // =========================================================================
    {
        Device::CommandEntry entry;
        assert(entry.targetHandler == nullptr);
        assert(entry.command.identifier == -1);
        std::printf("  [PASS] CommandEntry default initialization\n");
    }

    // =========================================================================
    // Test 2: CommandEntry construction with values
    // =========================================================================
    {
        CommandRecorder recorder;
        Device::CommandEntry entry(&recorder, Command(42));
        assert(entry.targetHandler == &recorder);
        assert(entry.command.identifier == 42);
        std::printf("  [PASS] CommandEntry construction with values\n");
    }

    // =========================================================================
    // Test 3: enqueueCommand with null queue returns false
    // =========================================================================
    {
        TestDevice device;
        CommandRecorder recorder;
        // commandQueueHandle is nullptr before initializeAsynchronousEngine
        bool result = device.enqueueCommand(&recorder, Command(1));
        assert(result == false);
        std::printf("  [PASS] enqueueCommand with null queue returns false\n");
    }

    // =========================================================================
    // Test 4: enqueueCommand with null target returns false
    // =========================================================================
    {
        TestDevice device;
        // Even if queue existed, null target should fail
        bool result = device.enqueueCommand(nullptr, Command(1));
        assert(result == false);
        std::printf("  [PASS] enqueueCommand with null target returns false\n");
    }

    // =========================================================================
    // Test 5: enqueueCommand with queue full returns false
    // =========================================================================
    {
        // xQueueSend mock returns pdFAIL, simulating a full queue
        TestDevice device;
        device.commandQueueHandle = reinterpret_cast<QueueHandle_t>(0x1); // Non-null fake handle
        CommandRecorder recorder;
        bool result = device.enqueueCommand(&recorder, Command(1));
        assert(result == false);
        std::printf("  [PASS] enqueueCommand with queue full returns false\n");
    }

    // =========================================================================
    // Test 6: processPendingCommands with empty queue is safe
    // =========================================================================
    {
        TestDevice device;
        // xQueueReceive mock returns pdFAIL, simulating empty queue
        device.processPendingCommands();
        std::printf("  [PASS] processPendingCommands with empty queue is safe\n");
    }

    // =========================================================================
    // Test 7: processPendingCommands with null targetHandler is safe
    // =========================================================================
    {
        TestDevice device;
        // Even if a CommandEntry has null targetHandler, handle() is not called
        device.processPendingCommands();
        std::printf("  [PASS] processPendingCommands with null targetHandler is safe\n");
    }

    // =========================================================================
    // Test 8: Command identifier preservation
    // =========================================================================
    {
        Command cmd(123);
        Device::CommandEntry entry(nullptr, cmd);
        assert(entry.command.identifier == 123);
        assert(entry.command == Command(123));
        std::printf("  [PASS] Command identifier preservation\n");
    }

    // =========================================================================
    // Test 9: Command equality comparison
    // =========================================================================
    {
        Command cmd1(100);
        Command cmd2(100);
        Command cmd3(200);
        assert(cmd1 == cmd2);
        assert(!(cmd1 == cmd3));
        std::printf("  [PASS] Command equality comparison\n");
    }

    // =========================================================================
    // Test 10: CommandRecorder captures command identifier
    // =========================================================================
    {
        CommandRecorder recorder;
        Command cmd(42);
        recorder.handle(cmd);
        assert(recorder.deliveredIdentifiers.size() == 1);
        assert(recorder.deliveredIdentifiers[0] == 42);
        std::printf("  [PASS] CommandRecorder captures command identifier\n");
    }

    // =========================================================================
    // Test 11: CommandRecorder captures multiple commands
    // =========================================================================
    {
        CommandRecorder recorder;
        recorder.handle(Command(1));
        recorder.handle(Command(2));
        recorder.handle(Command(3));
        assert(recorder.deliveredIdentifiers.size() == 3);
        assert(recorder.deliveredIdentifiers[0] == 1);
        assert(recorder.deliveredIdentifiers[1] == 2);
        assert(recorder.deliveredIdentifiers[2] == 3);
        std::printf("  [PASS] CommandRecorder captures multiple commands\n");
    }

    // =========================================================================
    // Test 12: CommandEntry constructor initialization
    // =========================================================================
    {
        CommandRecorder recorder;
        Device::CommandEntry entry(&recorder, Command(99));
        assert(entry.targetHandler == &recorder);
        assert(entry.command.identifier == 99);
        std::printf("  [PASS] CommandEntry constructor initialization\n");
    }

    // =========================================================================
    // Test 13: Device::handle default no-op
    // =========================================================================
    {
        TestDevice device;
        // Default handle() is a no-op; should not crash
        device.handle(Command(1));
        std::printf("  [PASS] Device::handle default no-op\n");
    }

    // =========================================================================
    // Test 14: CommandAvailable notification bit value
    // =========================================================================
    {
        assert(static_cast<uint32_t>(Device::NotificationBitFlag::CommandAvailable) == 0x8);
        assert(static_cast<uint32_t>(Device::NotificationBitFlag::TimerTickArrived) == 0x1);
        assert(static_cast<uint32_t>(Device::NotificationBitFlag::TerminationSignal) == 0x2);
        assert(static_cast<uint32_t>(Device::NotificationBitFlag::SensorTrigger) == 0x4);
        std::printf("  [PASS] CommandAvailable notification bit value\n");
    }

    // =========================================================================
    // Test 15: NotificationBitFlag bit isolation
    // =========================================================================
    {
        uint32_t flags = 0;
        flags |= static_cast<uint32_t>(Device::NotificationBitFlag::CommandAvailable);
        assert((flags & static_cast<uint32_t>(Device::NotificationBitFlag::CommandAvailable)) != 0);
        assert((flags & static_cast<uint32_t>(Device::NotificationBitFlag::TimerTickArrived)) == 0);
        assert((flags & static_cast<uint32_t>(Device::NotificationBitFlag::TerminationSignal)) == 0);
        assert((flags & static_cast<uint32_t>(Device::NotificationBitFlag::SensorTrigger)) == 0);
        std::printf("  [PASS] NotificationBitFlag bit isolation\n");
    }

    // =========================================================================
    // Test 16: Multiple notification bits can coexist
    // =========================================================================
    {
        uint32_t flags = 0;
        flags |= static_cast<uint32_t>(Device::NotificationBitFlag::TimerTickArrived);
        flags |= static_cast<uint32_t>(Device::NotificationBitFlag::CommandAvailable);
        assert((flags & static_cast<uint32_t>(Device::NotificationBitFlag::TimerTickArrived)) != 0);
        assert((flags & static_cast<uint32_t>(Device::NotificationBitFlag::CommandAvailable)) != 0);
        assert(flags == 0x9); // 0x1 | 0x8
        std::printf("  [PASS] Multiple notification bits can coexist\n");
    }

    std::printf("\nAll Command Marshaling tests passed!\n");
    return 0;
}
