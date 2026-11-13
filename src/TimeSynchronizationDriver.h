/**
 * @file TimeSynchronizationDriver.h
 * @brief Provides absolute Unix time via NTP with RTC fallback for deep-sleep devices.
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-11-12
 * @version 1.0
 * @ingroup core_module
 */

#ifndef MODEST_IOT_NANO_FRAMEWORK_TIME_SYNCHRONIZATION_DRIVER_H
#define MODEST_IOT_NANO_FRAMEWORK_TIME_SYNCHRONIZATION_DRIVER_H

#include <cstdint>

/**
 * @brief Manages time synchronization for telemetry timestamps.
 *
 * Replaces `millis()`-based timestamps with absolute Unix epoch seconds via NTP
 * synchronization over WiFi. Provides RTC memory fallback for deep-sleep devices
 * to maintain approximate time across reboots.
 *
 * All methods are static — the component is a singleton infrastructure service.
 * Falls back to `millis() / 1000` when NTP is unavailable or not initialized,
 * preserving backward compatibility with existing code.
 *
 * @par Threading
 * @runs_on{any} — all methods are thread-safe; safe to call from any task.
 */
class TimeSynchronizationDriver {
public:
    /**
     * @brief Initializes NTP time synchronization.
     * @param tz Timezone identifier (e.g., "UTC", "EST5EDT", "PST8PDT").
     * @param defaultNtpServer Primary NTP server address (e.g., "pool.ntp.org").
     * @param alternativeNtpServer Optional fallback NTP server address.
     * @note On ESP32, this calls `configTzTime()` internally. On host builds,
     *       this is a no-op that marks the driver as initialized.
     */
    static void initialize(const char* tz, const char* defaultNtpServer,
                           const char* alternativeNtpServer = nullptr);

    /**
     * @brief Returns the current timestamp as Unix epoch seconds.
     * @return Current Unix epoch seconds, or `millis() / 1000` as fallback if
     *         NTP is not synchronized.
     * @note The fallback returns seconds (not milliseconds) to maintain type
     *       consistency with the `captureTimestamp` field in TelemetryPackage.
     */
    [[nodiscard]] static uint32_t getCurrentTimestamp();

    /**
     * @brief Checks whether NTP synchronization has completed.
     * @return true if NTP has successfully synchronized the system clock.
     */
    [[nodiscard]] static bool isSynchronized();

    /**
     * @brief Saves the current Unix time to RTC memory for deep-sleep persistence.
     * @note Call this before entering deep sleep. The saved timestamp survives
     *       device reboots via `RTC_DATA_ATTR` storage.
     */
    static void saveToRtc();

    /**
     * @brief Restores the system clock from RTC memory after a deep-sleep wake.
     * @note Call this early in setup() after reporting the wake reason.
     *       The restored time is approximate until the next NTP synchronization.
     */
    static void restoreFromRtc();

private:
#if defined(ARDUINO_ARCH_ESP32)
    /** @brief Persisted Unix timestamp across deep-sleep reboots. */
    static uint32_t rtcSavedTimestamp;

    /** @brief Tracks whether NTP synchronization has completed. */
    static bool synchronized;
#else
    /** @brief Host test simulated RTC timestamp. */
    static uint32_t rtcSavedTimestamp;

    /** @brief Host test simulated sync flag. */
    static bool synchronized;
#endif
};

#endif // MODEST_IOT_NANO_FRAMEWORK_TIME_SYNCHRONIZATION_DRIVER_H
