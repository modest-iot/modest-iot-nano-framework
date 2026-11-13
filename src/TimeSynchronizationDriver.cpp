/**
 * @file TimeSynchronizationDriver.cpp
 * @brief Implements NTP-based time synchronization with RTC fallback.
 * @date 2026-11-12
 * @version 1.0
 */

#include "TimeSynchronizationDriver.h"

#if !defined(ARDUINO_ARCH_ESP32)
// Host test: declare millis() from the test fake without pulling in Arduino.h
// (which triggers macOS SDK <atomic>/<time.h> include order bugs)
extern "C" unsigned long millis();
#endif

#if defined(ARDUINO_ARCH_ESP32)
#include <time.h>
#include <sys/time.h>
#include "esp_sleep.h"
#endif

/** @name Static Member Definitions */
//@{

#if defined(ARDUINO_ARCH_ESP32)
RTC_DATA_ATTR uint32_t TimeSynchronizationDriver::rtcSavedTimestamp = 0;
bool TimeSynchronizationDriver::synchronized = false;
#else
uint32_t TimeSynchronizationDriver::rtcSavedTimestamp = 0;
bool TimeSynchronizationDriver::synchronized = false;
#endif

//@}

void TimeSynchronizationDriver::initialize(const char* timezone, const char* defaultNtpServer,
                                            const char* alternativeNtpServer) {
    (void) timezone;
    (void) defaultNtpServer;
    (void) alternativeNtpServer;

#if defined(ARDUINO_ARCH_ESP32)
    // Map framework's intention-revealing parameter names to ESP32's server1/server2
    configTzTime(timezone, defaultNtpServer, alternativeNtpServer);
    synchronized = false;
    // NTP sync will complete asynchronously; getLocalTime() will trigger the actual sync
    struct timeinfo tp;
    if (getLocalTime(&tp, 5000)) {
        synchronized = true;
    }
#else
    // Host test fallback: mark as initialized but not synchronized
    // Tests can manually set synchronized = true to simulate NTP completion
    synchronized = false;
#endif
}

uint32_t TimeSynchronizationDriver::getCurrentTimestamp() {
#if defined(ARDUINO_ARCH_ESP32)
    if (synchronized) {
        return static_cast<uint32_t>(time(nullptr));
    }
#else
    if (synchronized) {
        // Host test: return simulated time
        return rtcSavedTimestamp;
    }
#endif
    // Fallback: return seconds since boot (not milliseconds) for type consistency
    return static_cast<uint32_t>(millis() / 1000);
}

bool TimeSynchronizationDriver::isSynchronized() {
    return synchronized;
}

void TimeSynchronizationDriver::saveToRtc() {
#if defined(ARDUINO_ARCH_ESP32)
    if (synchronized) {
        rtcSavedTimestamp = static_cast<uint32_t>(time(nullptr));
    }
#else
    // Host test: save current simulated time
    rtcSavedTimestamp = synchronized ? rtcSavedTimestamp : static_cast<uint32_t>(millis() / 1000);
#endif
}

void TimeSynchronizationDriver::restoreFromRtc() {
    if (rtcSavedTimestamp == 0) return;

#if defined(ARDUINO_ARCH_ESP32)
    // Restore system time from RTC memory
    struct timeval tv;
    tv.tv_sec = static_cast<time_t>(rtcSavedTimestamp);
    tv.tv_usec = 0;
    settimeofday(&tv, nullptr);
    // Mark as not fully synchronized — this is approximate time until next NTP sync
    synchronized = false;
#else
    // Host test: no-op, rtcSavedTimestamp is already accessible
#endif
}
