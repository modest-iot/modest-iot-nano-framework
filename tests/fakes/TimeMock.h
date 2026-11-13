/**
 * @file Time.h
 * @brief Mock NTP/time functions for host testing.
 * @date 2026-11-12
 * @version 1.0
 */

#ifndef MODEST_IOT_NANO_FRAMEWORK_TEST_FAKES_TIME_H
#define MODEST_IOT_NANO_FRAMEWORK_TEST_FAKES_TIME_H

#include <cstdint>

/** @brief Simulated Unix epoch time for host tests. */
inline uint32_t mockUnixTime = 1700000000u;

/** @brief Simulated NTP sync status for host tests. */
inline bool mockNtpSynchronized = false;

/** @brief Simulated timezone for host tests. */
inline const char* mockTimezone = nullptr;

/** @brief Simulated primary NTP server for host tests. */
inline const char* mockDefaultNtpServer = nullptr;

/** @brief Simulated fallback NTP server for host tests. */
inline const char* mockAlternativeNtpServer = nullptr;

/**
 * @brief Resets all mock time state to defaults.
 */
inline void resetMockTimeState() {
    mockUnixTime = 1700000000u;  // Approximate 2023-11-14 22:13:20 UTC
    mockNtpSynchronized = false;
    mockTimezone = nullptr;
    mockDefaultNtpServer = nullptr;
    mockAlternativeNtpServer = nullptr;
}

#endif // MODEST_IOT_NANO_FRAMEWORK_TEST_FAKES_TIME_H
