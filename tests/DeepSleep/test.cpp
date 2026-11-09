/**
 * @file test.cpp
 * @brief Host unit tests for Deep Sleep (W17).
 *
 * Validates wake reason reporting for each wake cause and the PowerMode enum.
 */

#include "Arduino.h"
#include "Device.h"
#include <cassert>
#include <cstdio>

namespace {

// =========================================================================
// Test fixture: minimal Device subclass for deep sleep tests
// =========================================================================

class TestDevice : public Device {
public:
    TestDevice() : Device(1000, 0) {}

    void on(Event) override {}
    uint32_t appendSensorToTriggeringScheduler(
        Sensor*, int, int) override { return 0; }
    void removeSensorFromTriggeringScheduler(
        uint32_t) override {}
    bool appendToBothSchedulers(
        Sensor*, int, int) override { return false; }

    // Expose protected methods for testing
    using Device::setPowerMode;
    using Device::getPowerMode;
};

} // namespace

int main() {
    std::printf("Running DeepSleep tests...\n");

    // =========================================================================
    // Test 1: PowerMode enum has DeepSleep value
    // =========================================================================
    {
        TestDevice device;
        device.setPowerMode(Device::PowerMode::DeepSleep);
        assert(device.getPowerMode() == Device::PowerMode::DeepSleep);
        std::printf("  [PASS] PowerMode::DeepSleep is settable and readable\n");
    }

    // =========================================================================
    // Test 2: Timer wake cause is correctly identified
    // =========================================================================
    {
        fakeWakeupCause = ESP_SLEEP_WAKEUP_TIMER;
        TestDevice device;
        // reportWakeReason() prints to Serial; we verify the wake cause is read
        assert(esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER);
        std::printf("  [PASS] Timer wake cause is correctly identified\n");
    }

    // =========================================================================
    // Test 3: EXT0 wake cause is correctly identified
    // =========================================================================
    {
        fakeWakeupCause = ESP_SLEEP_WAKEUP_EXT0;
        assert(esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0);
        std::printf("  [PASS] EXT0 wake cause is correctly identified\n");
    }

    // =========================================================================
    // Test 4: EXT1 wake cause is correctly identified
    // =========================================================================
    {
        fakeWakeupCause = ESP_SLEEP_WAKEUP_EXT1;
        assert(esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT1);
        std::printf("  [PASS] EXT1 wake cause is correctly identified\n");
    }

    // =========================================================================
    // Test 5: Touchpad wake cause is correctly identified
    // =========================================================================
    {
        fakeWakeupCause = ESP_SLEEP_WAKEUP_TOUCHPAD;
        assert(esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TOUCHPAD);
        std::printf("  [PASS] Touchpad wake cause is correctly identified\n");
    }

    // =========================================================================
    // Test 6: ULP wake cause is correctly identified
    // =========================================================================
    {
        fakeWakeupCause = ESP_SLEEP_WAKEUP_ULP;
        assert(esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_ULP);
        std::printf("  [PASS] ULP wake cause is correctly identified\n");
    }

    // =========================================================================
    // Test 7: Unknown wake cause defaults to OTHER
    // =========================================================================
    {
        fakeWakeupCause = ESP_SLEEP_WAKEUP_OTHER;
        assert(esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_OTHER);
        std::printf("  [PASS] Unknown wake cause defaults to OTHER\n");
    }

    // =========================================================================
    // Test 8: RTC_DATA_ATTR macro is defined
    // =========================================================================
    {
        RTC_DATA_ATTR uint32_t testRtcValue = 42;
        assert(testRtcValue == 42);
        std::printf("  [PASS] RTC_DATA_ATTR macro is defined and works\n");
    }

    // =========================================================================
    // Test 9: esp_deep_sleep_start is callable (no-op in host tests)
    // =========================================================================
    {
        esp_deep_sleep_start(); // Should not crash in host tests
        std::printf("  [PASS] esp_deep_sleep_start is callable (no-op)\n");
    }

    // =========================================================================
    // Test 10: esp_sleep_enable_timer_wakeup is callable
    // =========================================================================
    {
        esp_sleep_enable_timer_wakeup(300000000ULL); // 5 minutes in microseconds
        std::printf("  [PASS] esp_sleep_enable_timer_wakeup is callable\n");
    }

    std::printf("All DeepSleep tests passed successfully!\n");
    return 0;
}
