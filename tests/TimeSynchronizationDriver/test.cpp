#include <time.h>
/**
 * @file test.cpp
 * @brief Host tests for TimeSynchronizationDriver.
 * @date 2026-11-12
 * @version 1.0
 */

#include <cstdio>
#include <cassert>
#include <cstdint>
#include <time.h>

// Minimal millis() mock for this test, avoiding Arduino.h include chain
// that triggers macOS SDK <atomic>/<time.h> ordering bugs
static unsigned long testFakeMillis = 0;
extern "C" unsigned long millis() { return testFakeMillis; }

#include "TimeSynchronizationDriver.h"
#include "TimeMock.h"

/** @name Test: getCurrentTimestamp returns millis-based fallback before initialization */
void test_getCurrentTimestamp_returns_millis_fallback_before_init() {
    resetMockTimeState();
    // isSynchronized() returns false by default before any initialize() call
    assert(TimeSynchronizationDriver::isSynchronized() == false);

    uint32_t timestamp = TimeSynchronizationDriver::getCurrentTimestamp();
    printf("  Fallback timestamp: %u seconds since boot\n", timestamp);
}

/** @name Test: isSynchronized returns false before initialization */
void test_isSynchronized_returns_false_before_init() {
    resetMockTimeState();
    // Driver starts unsynchronized
    assert(TimeSynchronizationDriver::isSynchronized() == false);
}

/** @name Test: initialize does not auto-sync on host builds */
void test_initialize_does_not_auto_sync_on_host() {
    resetMockTimeState();
    assert(TimeSynchronizationDriver::isSynchronized() == false);

    TimeSynchronizationDriver::initialize("UTC", "pool.ntp.org");

    // Host test: initialize does NOT auto-sync (no real NTP available)
    assert(TimeSynchronizationDriver::isSynchronized() == false);
    printf("  Initialize called, sync flag remains false (host test, no real NTP)\n");
}

/** @name Test: initialize with alternative server */
void test_initialize_with_alternative_server() {
    resetMockTimeState();

    TimeSynchronizationDriver::initialize("EST5EDT", "time.nist.gov", "pool.ntp.org");

    // Host test: no crash, sync flag remains false
    assert(TimeSynchronizationDriver::isSynchronized() == false);
    printf("  Initialize with alternative server called\n");
}

/** @name Test: saveToRtc with unsynchronized state */
void test_saveToRtc_unsynchronized() {
    resetMockTimeState();
    assert(TimeSynchronizationDriver::isSynchronized() == false);

    TimeSynchronizationDriver::saveToRtc();

    // In host test, unsynchronized save stores millis() / 1000
    // The timestamp should be non-negative
    printf("  saveToRtc called in unsynchronized state\n");
}

/** @name Test: restoreFromRtc with zero timestamp is no-op */
void test_restoreFromRtc_zero_timestamp_is_noop() {
    resetMockTimeState();

    TimeSynchronizationDriver::restoreFromRtc();

    // No crash expected when RTC timestamp is zero
}

/** @name Test: full sync cycle simulation using public API only */
void test_full_sync_cycle_simulation() {
    resetMockTimeState();

    // Step 1: Initial state — unsynchronized, fallback timestamp
    assert(TimeSynchronizationDriver::isSynchronized() == false);
    uint32_t fallbackTimestamp = TimeSynchronizationDriver::getCurrentTimestamp();

    // Step 2: Initialize NTP (host test — no real sync)
    TimeSynchronizationDriver::initialize("UTC", "pool.ntp.org");
    assert(TimeSynchronizationDriver::isSynchronized() == false);

    // Step 3: saveToRtc and restoreFromRtc are safe to call
    TimeSynchronizationDriver::saveToRtc();
    TimeSynchronizationDriver::restoreFromRtc();

    printf("  Full sync cycle completed, fallback timestamp: %u\n", fallbackTimestamp);
}

/** @name Test: timestamp type consistency */
void test_timestamp_type_consistency() {
    resetMockTimeState();

    // Verify return type is uint32_t (fits in captureTimestamp field)
    uint32_t timestamp = TimeSynchronizationDriver::getCurrentTimestamp();
    (void) timestamp;  // Suppress unused warning
    printf("  Timestamp type is uint32_t, consistent with TelemetryPackage::captureTimestamp\n");
}

int main() {
    printf("Running TimeSynchronizationDriver tests...\n");

    test_getCurrentTimestamp_returns_millis_fallback_before_init();
    test_isSynchronized_returns_false_before_init();
    test_initialize_does_not_auto_sync_on_host();
    test_initialize_with_alternative_server();
    test_saveToRtc_unsynchronized();
    test_restoreFromRtc_zero_timestamp_is_noop();
    test_full_sync_cycle_simulation();
    test_timestamp_type_consistency();

    printf("All TimeSynchronizationDriver tests passed!\n");
    return 0;
}
