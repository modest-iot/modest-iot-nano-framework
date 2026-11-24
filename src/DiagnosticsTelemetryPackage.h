#ifndef DIAGNOSTICS_TELEMETRY_PACKAGE_H
#define DIAGNOSTICS_TELEMETRY_PACKAGE_H

/**
 * @file DiagnosticsTelemetryPackage.h
 * @brief Declares a TelemetryPackage that wraps DiagnosticsSnapshot for on-demand diagnostics.
 * @ingroup core_module
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-11-19
 * @version 1.0
 */

#include "TelemetryPackage.h"

// JSON key constants for diagnostics fields
#define JSON_KEY_MISSED_TICKS             "missedTicks"
#define JSON_KEY_WORST_CASE_WAKE_MICROS   "worstCaseWakeMicros"
#define JSON_KEY_PACKETS_ENQUEUED         "packetsEnqueued"
#define JSON_KEY_PACKETS_DROPPED          "packetsDropped"
#define JSON_KEY_PACKETS_DROPPED_OLDEST   "packetsDroppedOldest"
#define JSON_KEY_QUEUE_HIGH_WATER_MARK    "queueHighWaterMark"
#define JSON_KEY_HIGH_PRIORITY_ENQUEUED   "highPriorityEnqueued"
#define JSON_KEY_HIGH_PRIORITY_DROPPED    "highPriorityDropped"
#define JSON_KEY_HIGH_PRIORITY_Q_HWM      "highPriorityQueueHighWaterMark"
#define JSON_KEY_SENDS_SUCCEEDED          "sendsSucceeded"
#define JSON_KEY_SENDS_FAILED             "sendsFailed"
#define JSON_KEY_FREE_HEAP_BYTES          "freeHeapBytes"
#define JSON_KEY_MIN_FREE_HEAP_BYTES      "minFreeHeapBytes"
#define JSON_KEY_SAMPLER_STACK_FREE       "samplerStackFreeWords"
#define JSON_KEY_NETWORK_STACK_FREE       "networkStackFreeWords"
#define JSON_KEY_UPTIME_SECONDS           "uptimeSeconds"

/**
 * @brief TelemetryPackage that wraps a DiagnosticsSnapshot for on-demand diagnostics.
 * @ingroup core_module
 *
 * When the cloud sends a diagnostics command (commandId = 300), the device
 * creates this package from Device::collectDiagnostics() and enqueues it via the
 * standard telemetry queue. The JSON output contains all 16 health metrics
 * plus envelope metadata (timestamp, sequence, schema version, firmware version).
 */
class DiagnosticsTelemetryPackage : public TelemetryPackage {
public:
    /** @brief Schema version for this diagnostics telemetry format. */
    static constexpr const char* SCHEMA_VERSION = "1.0";

private:
    uint32_t    missedTicks;
    uint32_t    worstCaseWakeMicros;
    uint32_t    packetsEnqueued;
    uint32_t    packetsDropped;
    uint32_t    packetsDroppedOldest;
    uint8_t     queueHighWaterMark;
    uint32_t    highPriorityEnqueued;
    uint32_t    highPriorityDropped;
    uint8_t     highPriorityQueueHighWaterMark;
    uint32_t    sendsSucceeded;
    uint32_t    sendsFailed;
    uint32_t    freeHeapBytes;
    uint32_t    minFreeHeapBytes;
    uint16_t    samplerStackFreeWords;
    uint16_t    networkStackFreeWords;
    uint32_t    uptimeSeconds;

public:
    /**
     * @brief Constructs a DiagnosticsTelemetryPackage from a DiagnosticsSnapshot.
     * @param snapshot The diagnostics snapshot to wrap.
     */
    explicit DiagnosticsTelemetryPackage(const struct DiagnosticsSnapshot& snapshot);

    /**
     * @brief Writes all 16 diagnostics fields to the given TelemetryWriter.
     * @param writer The telemetry writer to write fields to.
     */
    void write(TelemetryWriter& writer) const override;
};

#endif // DIAGNOSTICS_TELEMETRY_PACKAGE_H
