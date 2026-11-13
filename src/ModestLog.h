#ifndef MODEST_LOG_H
#define MODEST_LOG_H

/**
 * @file ModestLog.h
 * @brief Zero-overhead compile-time logging facility with tag and level filtering.
 * @ingroup core_module
 * @copyright (c) 2025-2026 Angel Velasquez
 * @license Apache-2.0
 *
 * Provides standardized logging macros (`MODEST_LOG_*`) and ergonomic shorthand
 * aliases (`MIOT_LOG_*`). On Arduino-ESP32, logging maps to native hardware-buffered
 * `log_*` facilities. In native host test environments or when a custom sink is
 * registered, output routes to a pluggable sink.
 *
 * @date 2026-11-04
 */

#include <cstdarg>
#include <cstdio>

/** @name Log Level Constants */
//@{
#ifndef MODEST_LOG_LEVEL_NONE
#define MODEST_LOG_LEVEL_NONE    0
#endif
#ifndef MODEST_LOG_LEVEL_ERROR
#define MODEST_LOG_LEVEL_ERROR   1
#endif
#ifndef MODEST_LOG_LEVEL_WARN
#define MODEST_LOG_LEVEL_WARN    2
#endif
#ifndef MODEST_LOG_LEVEL_INFO
#define MODEST_LOG_LEVEL_INFO    3
#endif
#ifndef MODEST_LOG_LEVEL_DEBUG
#define MODEST_LOG_LEVEL_DEBUG   4
#endif
#ifndef MODEST_LOG_LEVEL_VERBOSE
#define MODEST_LOG_LEVEL_VERBOSE 5
#endif

//@} // Log Level Constants

/** @name Active Log Level Configuration */
//@{
#ifndef MODEST_LOG_LEVEL
#if defined(MODEST_IOT_LOG_LEVEL)
#define MODEST_LOG_LEVEL MODEST_IOT_LOG_LEVEL
#else
#define MODEST_LOG_LEVEL MODEST_LOG_LEVEL_WARN
#endif
#endif

//@} // Active Log Level Configuration

/** @name Backend Routing & Sink Definitions */
//@{
#if defined(ARDUINO) && defined(ESP32) && !defined(MODEST_LOG_CUSTOM_SINK)
#include <esp32-hal-log.h>
#define MODEST_LOG_WRITE_E(tag, fmt, ...) log_e("[%s] " fmt, tag, ##__VA_ARGS__)
#define MODEST_LOG_WRITE_W(tag, fmt, ...) log_w("[%s] " fmt, tag, ##__VA_ARGS__)
#define MODEST_LOG_WRITE_I(tag, fmt, ...) log_i("[%s] " fmt, tag, ##__VA_ARGS__)
#define MODEST_LOG_WRITE_D(tag, fmt, ...) log_d("[%s] " fmt, tag, ##__VA_ARGS__)
#define MODEST_LOG_WRITE_V(tag, fmt, ...) log_v("[%s] " fmt, tag, ##__VA_ARGS__)
#else
namespace modest_iot {
    typedef void (*LogSinkFn)(int level, const char* tag, const char* fmt, va_list args);

    inline LogSinkFn& getLogSink() {
        static LogSinkFn sink = nullptr;
        return sink;
    }

    inline void setLogSink(LogSinkFn sink) {
        getLogSink() = sink;
    }

    inline void writeLog(int level, const char* tag, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        if (getLogSink() != nullptr) {
            getLogSink()(level, tag, fmt, args);
        } else {
            const char* prefix = "[I]";
            if (level == MODEST_LOG_LEVEL_ERROR) prefix = "[E]";
            else if (level == MODEST_LOG_LEVEL_WARN) prefix = "[W]";
            else if (level == MODEST_LOG_LEVEL_DEBUG) prefix = "[D]";
            else if (level == MODEST_LOG_LEVEL_VERBOSE) prefix = "[V]";

            std::printf("%s [%s] ", prefix, tag);
            std::vprintf(fmt, args);
            std::printf("\n");
        }
        va_end(args);
    }
} // namespace modest_iot

#define MODEST_LOG_WRITE_E(tag, fmt, ...) ::modest_iot::writeLog(MODEST_LOG_LEVEL_ERROR, tag, fmt, ##__VA_ARGS__)
#define MODEST_LOG_WRITE_W(tag, fmt, ...) ::modest_iot::writeLog(MODEST_LOG_LEVEL_WARN, tag, fmt, ##__VA_ARGS__)
#define MODEST_LOG_WRITE_I(tag, fmt, ...) ::modest_iot::writeLog(MODEST_LOG_LEVEL_INFO, tag, fmt, ##__VA_ARGS__)
#define MODEST_LOG_WRITE_D(tag, fmt, ...) ::modest_iot::writeLog(MODEST_LOG_LEVEL_DEBUG, tag, fmt, ##__VA_ARGS__)
#define MODEST_LOG_WRITE_V(tag, fmt, ...) ::modest_iot::writeLog(MODEST_LOG_LEVEL_VERBOSE, tag, fmt, ##__VA_ARGS__)
#endif

//@} // Backend Routing & Sink Definitions

/** @name Compile-Time Filtered Macros */
//@{
#if MODEST_LOG_LEVEL >= MODEST_LOG_LEVEL_ERROR
#define MODEST_LOG_E(tag, fmt, ...) MODEST_LOG_WRITE_E(tag, fmt, ##__VA_ARGS__)
#else
#define MODEST_LOG_E(tag, fmt, ...) ((void)0)
#endif

#if MODEST_LOG_LEVEL >= MODEST_LOG_LEVEL_WARN
#define MODEST_LOG_W(tag, fmt, ...) MODEST_LOG_WRITE_W(tag, fmt, ##__VA_ARGS__)
#else
#define MODEST_LOG_W(tag, fmt, ...) ((void)0)
#endif

#if MODEST_LOG_LEVEL >= MODEST_LOG_LEVEL_INFO
#define MODEST_LOG_I(tag, fmt, ...) MODEST_LOG_WRITE_I(tag, fmt, ##__VA_ARGS__)
#else
#define MODEST_LOG_I(tag, fmt, ...) ((void)0)
#endif

#if MODEST_LOG_LEVEL >= MODEST_LOG_LEVEL_DEBUG
#define MODEST_LOG_D(tag, fmt, ...) MODEST_LOG_WRITE_D(tag, fmt, ##__VA_ARGS__)
#else
#define MODEST_LOG_D(tag, fmt, ...) ((void)0)
#endif

#if MODEST_LOG_LEVEL >= MODEST_LOG_LEVEL_VERBOSE
#define MODEST_LOG_V(tag, fmt, ...) MODEST_LOG_WRITE_V(tag, fmt, ##__VA_ARGS__)
#else
#define MODEST_LOG_V(tag, fmt, ...) ((void)0)
#endif

//@} // Compile-Time Filtered Macros

/** @name Ergonomic Shorthand Aliases (MIOT_LOG_*) */
//@{
#if !defined(MODEST_DISABLE_SHORT_LOG_MACROS)
#define MIOT_LOG_E MODEST_LOG_E
#define MIOT_LOG_W MODEST_LOG_W
#define MIOT_LOG_I MODEST_LOG_I
#define MIOT_LOG_D MODEST_LOG_D
#define MIOT_LOG_V MODEST_LOG_V
#endif

//@} // Ergonomic Shorthand Aliases (MIOT_LOG_*)

#endif // MODEST_LOG_H
