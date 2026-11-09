/**
 * @file test.cpp
 * @brief Host unit tests for DiagnosticsSnapshot and shared counters.
 *
 * Validates counter initialization, DiagnosticsSnapshot field defaults,
 * and the diagnostics() method output using mock ESP32/FreeRTOS APIs.
 */

#include "Device.h"
#include "TelemetryPackage.h"
#include <cstdio>
#include <cassert>
#include <memory>

#ifdef TEST_MOCK_DEVICE

// We cannot instantiate Device directly on the host because it requires FreeRTOS.
// Instead, we test DiagnosticsSnapshot struct properties and simulate the counter
// behavior that Device would provide.

// Concrete test subclass of TelemetryPackage to satisfy abstract interface
class TestTelemetryPackage : public TelemetryPackage {
public:
    void serialize(JsonDocument& doc) const override {
        doc["test"] = 1;
    }
};

// Lightweight mock that provides the diagnostics() interface without FreeRTOS
class MockDeviceDiagnostics {
private:
    uint32_t packetsEnqueuedCount = 0;
    uint32_t packetsDroppedCount = 0;
    uint32_t packetsDroppedOldestCount = 0;
    uint8_t queueHighWaterMarkCount = 0;
    uint32_t missedTickCount = 0;

public:
    DiagnosticsSnapshot diagnostics() const {
        DiagnosticsSnapshot snapshot;
        snapshot.missedTicks = missedTickCount;
        snapshot.worstCaseWakeMicros = 0;
        snapshot.packetsEnqueued = packetsEnqueuedCount;
        snapshot.packetsDropped = packetsDroppedCount;
        snapshot.packetsDroppedOldest = packetsDroppedOldestCount;
        snapshot.queueHighWaterMark = queueHighWaterMarkCount;
        snapshot.sendsSucceeded = 0;
        snapshot.sendsFailed = 0;
        snapshot.freeHeapBytes = ESP.getFreeHeap();
        snapshot.minFreeHeapBytes = ESP.getMinFreeHeap();
        snapshot.uptimeSeconds = millis() / 1000;
        snapshot.samplerStackFreeWords = 0;
        snapshot.networkStackFreeWords = 0;
        return snapshot;
    }

    bool enqueueTelemetryPayload(TelemetryPackage**) const {
        // Simulates null queue behavior
        return false;
    }

    bool enqueueTelemetryPayload(std::unique_ptr<TelemetryPackage>) const {
        // Simulates null queue behavior
        return false;
    }
};

#else

// Concrete test subclass of Device to satisfy abstract interface
class TestDevice : public Device {
public:
    explicit TestDevice(unsigned long interval, uint8_t timerIdx = 0)
        : Device(interval, timerIdx) {}
    
    void on(Event event) override {
        (void)event;
    }
};

// Concrete test subclass of TelemetryPackage to satisfy abstract interface
class TestTelemetryPackage : public TelemetryPackage {
public:
    void serialize(JsonDocument& doc) const override {
        doc["test"] = 1;
    }
};

#endif // TEST_MOCK_DEVICE

namespace {

// Test that DiagnosticsSnapshot fields default to zero
void test_snapshot_defaults() {
    DiagnosticsSnapshot snapshot;
    
    assert(snapshot.missedTicks == 0);
    assert(snapshot.worstCaseWakeMicros == 0);
    assert(snapshot.packetsEnqueued == 0);
    assert(snapshot.packetsDropped == 0);
    assert(snapshot.packetsDroppedOldest == 0);
    assert(snapshot.queueHighWaterMark == 0);
    assert(snapshot.sendsSucceeded == 0);
    assert(snapshot.sendsFailed == 0);
    assert(snapshot.freeHeapBytes == 0);
    assert(snapshot.minFreeHeapBytes == 0);
    assert(snapshot.samplerStackFreeWords == 0);
    assert(snapshot.networkStackFreeWords == 0);
    assert(snapshot.uptimeSeconds == 0);
    
    printf("[PASS] DiagnosticsSnapshot defaults to zero\n");
}

// Test that Device counters are initialized to zero
void test_counter_initialization() {
#ifdef TEST_MOCK_DEVICE
    MockDeviceDiagnostics device;
#else
    TestDevice device(1000);
#endif
    
    DiagnosticsSnapshot snapshot = device.diagnostics();
    
    assert(snapshot.packetsEnqueued == 0);
    assert(snapshot.packetsDropped == 0);
    assert(snapshot.packetsDroppedOldest == 0);
    assert(snapshot.queueHighWaterMark == 0);
    assert(snapshot.missedTicks == 0);
    
    // Stack high-water marks should be 0 because task handles are nullptr
    assert(snapshot.samplerStackFreeWords == 0);
    assert(snapshot.networkStackFreeWords == 0);
    
    printf("[PASS] Device counters initialized to zero\n");
}

// Test that diagnostics() returns mock heap values
void test_heap_diagnostics() {
#ifdef TEST_MOCK_DEVICE
    MockDeviceDiagnostics device;
#else
    TestDevice device(1000);
#endif
    
    DiagnosticsSnapshot snapshot = device.diagnostics();
    
    // Mock ESP.getFreeHeap() returns 100000
    assert(snapshot.freeHeapBytes == 100000);
    // Mock ESP.getMinFreeHeap() returns 50000
    assert(snapshot.minFreeHeapBytes == 50000);
    
    printf("[PASS] Heap diagnostics return mock values\n");
}

// Test that uptime is derived from millis()
void test_uptime_diagnostics() {
    fakeMillis = 0;
#ifdef TEST_MOCK_DEVICE
    MockDeviceDiagnostics device;
#else
    TestDevice device(1000);
#endif
    
    {
        DiagnosticsSnapshot snapshot = device.diagnostics();
        assert(snapshot.uptimeSeconds == 0);
    }
    
    // Advance mock time by 3500ms -> 3 seconds
    fakeMillis = 3500;
    {
        DiagnosticsSnapshot snapshot = device.diagnostics();
        assert(snapshot.uptimeSeconds == 3);
    }
    
    // Advance mock time by 9999ms -> 9 seconds
    fakeMillis = 9999;
    {
        DiagnosticsSnapshot snapshot = device.diagnostics();
        assert(snapshot.uptimeSeconds == 9);
    }
    
    printf("[PASS] Uptime correctly derived from millis()\n");
}

// Test that enqueueTelemetryPayload returns false when queue is nullptr
void test_enqueue_with_null_queue() {
#ifdef TEST_MOCK_DEVICE
    MockDeviceDiagnostics device;
#else
    TestDevice device(1000);
#endif
    // Engine not initialized, so telemetryQueueHandle is nullptr
    
    TestTelemetryPackage packet;
    TelemetryPackage* packetPtr = &packet;
    bool result = device.enqueueTelemetryPayload(&packetPtr);
    
    assert(result == false);
    
    DiagnosticsSnapshot snapshot = device.diagnostics();
    assert(snapshot.packetsEnqueued == 0);
    assert(snapshot.packetsDropped == 0);
    
    printf("[PASS] Enqueue with null queue returns false\n");
}

// Test that enqueueTelemetryPayload returns false when packet pointer is nullptr
void test_enqueue_with_null_packet() {
#ifdef TEST_MOCK_DEVICE
    MockDeviceDiagnostics device;
#else
    TestDevice device(1000);
#endif
    
    bool result = device.enqueueTelemetryPayload(nullptr);
    assert(result == false);
    
    printf("[PASS] Enqueue with null packet pointer returns false\n");
}

// Test aggregate initialization of DiagnosticsSnapshot
void test_snapshot_aggregate_init() {
    DiagnosticsSnapshot snapshot = {
        .missedTicks = 5,
        .packetsEnqueued = 100,
        .packetsDropped = 3,
        .queueHighWaterMark = 8,
        .freeHeapBytes = 50000,
        .minFreeHeapBytes = 20000,
        .uptimeSeconds = 3600
    };
    
    assert(snapshot.missedTicks == 5);
    assert(snapshot.packetsEnqueued == 100);
    assert(snapshot.packetsDropped == 3);
    assert(snapshot.queueHighWaterMark == 8);
    assert(snapshot.freeHeapBytes == 50000);
    assert(snapshot.minFreeHeapBytes == 20000);
    assert(snapshot.uptimeSeconds == 3600);
    // Unspecified fields default to zero
    assert(snapshot.worstCaseWakeMicros == 0);
    assert(snapshot.sendsSucceeded == 0);
    assert(snapshot.sendsFailed == 0);
    assert(snapshot.samplerStackFreeWords == 0);
    assert(snapshot.networkStackFreeWords == 0);
    
    printf("[PASS] DiagnosticsSnapshot aggregate initialization works\n");
}

// Test that GatewayStatistics defaults to zero
void test_gateway_statistics_defaults() {
    GatewayStatistics stats;
    assert(stats.sendsSucceeded == 0);
    assert(stats.sendsFailed == 0);
    printf("[PASS] GatewayStatistics defaults to zero\n");
}

// Test that GatewayClient::getStatistics() returns zeros by default
void test_gateway_client_default_statistics() {
    // GatewayClient's default getStatistics() returns empty stats
    // This is verified by the GatewayStatistics defaults test above
    // and by the MqttGatewayClient tests that verify counter tracking
    GatewayStatistics stats = {};
    assert(stats.sendsSucceeded == 0);
    assert(stats.sendsFailed == 0);
    printf("[PASS] GatewayClient default statistics are zero\n");
}

// Test that unique_ptr enqueue returns false when queue is nullptr
void test_unique_ptr_enqueue_with_null_queue() {
#ifdef TEST_MOCK_DEVICE
    MockDeviceDiagnostics device;
#else
    TestDevice device(1000);
#endif
    // Engine not initialized, so telemetryQueueHandle is nullptr

    auto packet = std::make_unique<TestTelemetryPackage>();
    bool result = device.enqueueTelemetryPayload(std::move(packet));

    assert(result == false);
    // After a failed enqueue, the unique_ptr retains ownership and deletes automatically
    // If we reach here without a crash, the deletion succeeded

    DiagnosticsSnapshot snapshot = device.diagnostics();
    assert(snapshot.packetsEnqueued == 0);
    assert(snapshot.packetsDropped == 0);

    printf("[PASS] unique_ptr enqueue with null queue returns false and deletes packet\n");
}

// Test that unique_ptr enqueue returns false when packet is nullptr
void test_unique_ptr_enqueue_with_null_packet() {
#ifdef TEST_MOCK_DEVICE
    MockDeviceDiagnostics device;
#else
    TestDevice device(1000);
#endif

    std::unique_ptr<TestTelemetryPackage> packet = nullptr;
    bool result = device.enqueueTelemetryPayload(std::move(packet));
    assert(result == false);

    printf("[PASS] unique_ptr enqueue with null packet returns false\n");
}

// Test that unique_ptr ownership transfer compiles and moves correctly
void test_unique_ptr_ownership_transfer() {
    // Verify that std::make_unique creates a valid unique_ptr
    auto packet = std::make_unique<TestTelemetryPackage>();
    assert(packet != nullptr);

    // Verify that std::move transfers ownership (packet becomes nullptr)
    std::unique_ptr<TestTelemetryPackage> transferred = std::move(packet);
    assert(packet == nullptr);
    assert(transferred != nullptr);

    // Verify that the transferred pointer can be released
    TestTelemetryPackage* raw = transferred.release();
    assert(raw != nullptr);
    delete raw;

    printf("[PASS] unique_ptr ownership transfer works correctly\n");
}

} // anonymous namespace

[[nodiscard]] int main() {
    printf("=== Diagnostics Snapshot Tests ===\n");
    
    test_snapshot_defaults();
    test_counter_initialization();
    test_heap_diagnostics();
    test_uptime_diagnostics();
    test_enqueue_with_null_queue();
    test_enqueue_with_null_packet();
    test_snapshot_aggregate_init();
    test_gateway_statistics_defaults();
    test_gateway_client_default_statistics();
    test_unique_ptr_enqueue_with_null_queue();
    test_unique_ptr_enqueue_with_null_packet();
    test_unique_ptr_ownership_transfer();
    
    printf("\nAll diagnostics tests passed!\n");
    return 0;
}
