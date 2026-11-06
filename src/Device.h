#ifndef DEVICE_H
#define DEVICE_H

/**
 * @file Device.h
 * @brief Declares the Device base class that orchestrates sensors, commands, and telemetry dispatch.
 * @ingroup core_module
 * @date 2026-06-06
 * @version 1.0
 */

#include "EventHandler.h"
#include "Sensor.h"
#include "CommandHandler.h"
#include "TelemetryPackage.h"
#include "GatewayClient.h"
#include "SamplingScheduler.h"
#include <memory>
#include <Arduino.h>

struct hw_timer_s;

/**
 * @brief Configuration parameters for the asynchronous FreeRTOS engine.
 * @ingroup core_module
 *
 * Allows tuning stack sizes, task priorities, and core pinning without modifying framework source.
 * Default values match the original hard-coded parameters: sampler (4096 bytes, priority 3, core 1),
 * network worker (8192 bytes, priority 1, core 1), queue length 10.
 */
struct EngineConfig {
    uint32_t samplerStackBytes = 4096; ///< Stack size for the hardware sampling task.
    uint32_t networkStackBytes = 8192; ///< Stack size for the network dispatcher task.
    uint8_t samplerPriority = 3; ///< FreeRTOS priority for the sampler task.
    uint8_t networkPriority = 1; ///< FreeRTOS priority for the network task.
    BaseType_t samplerCore = 1; ///< Core pinning for the sampler task.
    BaseType_t networkCore = 1; ///< Core pinning for the network task.
    uint8_t queueLength = 10; ///< Maximum telemetry queue depth.
};

/**
 * @brief Runtime health snapshot containing stack, heap, queue, and timing metrics.
 * @ingroup core_module
 *
 * Returned by value from `Device::diagnostics()`. Stack margins are reported in
 * FreeRTOS `StackType_t` words (4 bytes on ESP32), not raw bytes.
 */
struct DiagnosticsSnapshot {
    uint32_t missedTicks = 0; ///< Ticks missed by the sampler due to handler overrun.
    uint32_t worstCaseWakeMicros = 0; ///< Highest observed sampler handler duration.
    uint32_t packetsEnqueued = 0; ///< Total telemetry packets successfully enqueued.
    uint32_t packetsDropped = 0; ///< Total telemetry packets dropped due to full queue.
    uint32_t packetsDroppedOldest = 0; ///< Total oldest packets dropped to make room for new ones.
    uint8_t queueHighWaterMark = 0; ///< Maximum queue depth observed.
    uint32_t sendsSucceeded = 0; ///< Successful gateway transmissions.
    uint32_t sendsFailed = 0; ///< Failed gateway transmissions.
    uint32_t freeHeapBytes = 0; ///< Current free heap in bytes.
    uint32_t minFreeHeapBytes = 0; ///< Lowest free heap observed since boot.
    uint16_t samplerStackFreeWords = 0; ///< Sampler task stack high-water mark (words).
    uint16_t networkStackFreeWords = 0; ///< Network task stack high-water mark (words).
    uint32_t uptimeSeconds = 0; ///< Device uptime in seconds.
};


/**
 * @name Core: Device Orchestration
 * @ingroup core_module
 * @{
 */

/**
 * @brief Base class for composed IoT devices with asynchronous sampling and telemetry pipelines.
 * @ingroup core_module
 *
 * `Device` coordinates periodic sensor polling, queue-based telemetry handoff, and
 * dispatch hooks implemented by subclasses.
 */
class Device : public EventHandler, public CommandHandler {
private:
    SamplingScheduler scheduler; ///< Per-slot sampling scheduler.
    bool engineStarted; ///< Guards against registration after engine start.

    TaskHandle_t hardwareSamplingTaskHandle; ///< Handle for the hardware sampling task.
    TaskHandle_t networkDispatcherTaskHandle; ///< Handle for the network dispatcher task.
    QueueHandle_t telemetryQueueHandle; ///< Handle for the telemetry queue.

    // Shared diagnostic counters (single-writer rule: each counter written by one task)
    mutable uint32_t packetsEnqueuedCount; ///< Total packets successfully enqueued.
    mutable uint32_t packetsDroppedCount; ///< Total packets dropped due to full queue.
    mutable uint32_t packetsDroppedOldestCount; ///< Total oldest packets dropped to make room for new ones.
    mutable uint8_t queueHighWaterMarkCount; ///< Maximum queue depth observed.
    mutable uint32_t missedTickCount; ///< Ticks missed by the sampler.
    mutable bool firstDropLogged; ///< Tracks whether the first drop warning was emitted.
    mutable uint32_t nextSequenceNumber; ///< Monotonically increasing sequence for telemetry envelope.

    hw_timer_t* hardwareTimer; ///< Pointer to the hardware timer.
    const unsigned long pollingIntervalInMilliseconds; ///< Polling interval in milliseconds.
    const uint8_t hardwareTimerIndex; ///< Index of the hardware timer.

    /**
     * @brief FreeRTOS task entry point for sensor polling.
     * @param taskParameters Pointer to the owning `Device` instance.
     */
    static void executeHardwareSamplingContext(void* taskParameters);

    /**
     * @brief FreeRTOS task entry point for queue-to-network processing.
     * @param taskParameters Pointer to the owning `Device` instance.
     */
    static void executeNetworkDispatchContext(void* taskParameters);

    /**
     * @brief Hardware timer ISR trampoline that advances the scheduler pointer.
     * @param deviceInstance Pointer to the owning `Device` instance.
     */
    static void IRAM_ATTR handleHardwareTimerInterrupt(void* deviceInstance);

protected:
    /**
     * @brief Creates a device with a fixed polling cadence and timer index.
     * @param samplingIntervalInMilliseconds Period between sampling ticks.
     * @param timerGroupIndex ESP32 hardware timer index used by the scheduler.
     */
    explicit Device(unsigned long samplingIntervalInMilliseconds, uint8_t timerGroupIndex = 0);

    /**
     * @brief Cleans up tasks, timer resources, and queue handles.
     */
    ~Device() override;

    /**
     * @brief Allocates queue/task infrastructure and starts asynchronous workers.
     * @param maximumQueueLength Capacity of the telemetry queue.
     */
    void initializeAsynchronousEngine(uint8_t maximumQueueLength = 10);

    /**
     * @brief Allocates queue/task infrastructure with full engine configuration.
     * @param config Stack sizes, priorities, core pinning, and queue depth.
     */
    void initializeAsynchronousEngine(const EngineConfig& config);

    /**
     * @brief Pushes a telemetry payload pointer into the internal queue.
     * @param telemetryPackagePointerAddress Address of a payload pointer to enqueue.
     * @return `true` when enqueued successfully, otherwise `false`.
     */
    bool enqueueTelemetryPayload(TelemetryPackage** telemetryPackagePointerAddress) const;

    /**
     * @brief Pushes a telemetry payload into the internal queue with safe ownership transfer.
     * @param telemetryPackageStandardUniquePointer Unique pointer to the payload to enqueue.
     * @return `true` when enqueued successfully, otherwise `false`.
     *
     * When the enqueue succeeds, ownership is transferred to the framework and the
     * unique_ptr is released. When the enqueue fails (e.g., queue full), the unique_ptr
     * retains ownership and the payload is deleted automatically — no manual cleanup needed.
     */
    bool enqueueTelemetryPayload(std::unique_ptr<TelemetryPackage> telemetryPackageStandardUniquePointer) const;

    /**
     * @brief Registers a sensor/event pair in the scheduler (two-argument fallback form).
     * @param targetSensorInstance Sensor (or any `EventHandler`) to poll.
     * @param requestedEventIdentifier Event identifier to request from that sensor.
     * @return `true` when the slot is registered, `false` when scheduler capacity is full.
     *
     * This overload passes a period of 0 to the scheduler, which treats it as "every tick."
     * It serves as a valid fallback for non-Sensor `EventHandler` instances (e.g., a Device
     * subclass polling itself). For Sensor arguments, the `Sensor*` overload is preferred
     * because it resolves the sensor's own sampling interval defaults automatically.
     */
    bool appendSensorToScheduler(EventHandler* targetSensorInstance, int requestedEventIdentifier);

    /**
     * @brief Registers a sensor using its own sampling interval defaults.
     * @param targetSensorInstance Sensor to poll; period is resolved via
     *        `getSamplingIntervalInMilliseconds()` and `getMinimalSamplingIntervalInMilliseconds()`.
     * @param requestedEventIdentifier Event identifier to request from that sensor.
     * @return `true` when the slot is registered, `false` when the scheduler is full or
     *        the engine has already started.
     *
     * This overload queries the sensor's effective sampling interval (per-instance override
     * if set, otherwise the subclass default) and registers it with the scheduler.
     * Calls after `initializeAsynchronousEngine()` are rejected.
     */
    bool appendSensorToScheduler(Sensor* targetSensorInstance, int requestedEventIdentifier);

    /**
     * @brief Registers a sensor/event pair with an explicit sampling period.
     * @param targetSensorInstance Sensor (or any `EventHandler`) to poll.
     * @param requestedEventIdentifier Event identifier to request from that sensor.
     * @param periodInMilliseconds Desired sampling period in milliseconds.
     * @param minimumPeriodInMilliseconds Optional minimum period; if the requested period
     *        is below this value, it is clamped up.
     * @return `true` when the slot is registered (including clamped periods), `false` when
     *        the scheduler is full, the engine has already started, or arguments are invalid.
     *
     * Periods are rounded up to whole ticks. Equal-period slots are staggered so they
     * first fire on consecutive ticks, then each repeats every period.
     * Calls after `initializeAsynchronousEngine()` are rejected.
     */
    bool appendSensorToScheduler(EventHandler* targetSensorInstance, int requestedEventIdentifier,
                                  uint32_t periodInMilliseconds,
                                  uint32_t minimumPeriodInMilliseconds = 0);

    /**
     * @brief Optional gateway reference for diagnostics aggregation.
     * Derived classes can set this so diagnostics() can query send statistics.
     */
    GatewayClient* telemetryGateway = nullptr;

    /**
     * @brief Subclass hook for converting queued payloads into network transmissions.
     * @param rawQueueItemPayload Pointer to the queued telemetry payload.
     */
    virtual void processQueuedTelemetryData(const TelemetryPackage* rawQueueItemPayload) const;

    /**
     * @brief Subclass hook for background network and gateway upkeep (e.g., MQTT keep-alive).
     * Invoked periodically by the network dispatcher task during idle queue periods.
     */
    virtual void maintainNetworkServices() const;

    /**
     * @brief Gracefully stops the asynchronous engine and releases all resources.
     *
     * Stops the hardware timer, signals both FreeRTOS tasks to exit cooperatively,
     * waits for them to terminate, drains and deletes any pending telemetry packets,
     * and deletes the queue. After calling this method, the engine cannot be restarted.
     *
     * Derived classes must call this method in their destructor **before** their own
     * destructor body runs, because sensor/actuator members are destroyed before
     * ~Device() executes, and a running task could access freed memory.
     */
    void stopAsynchronousEngine();

public:
    /**
     * @brief Returns a snapshot of runtime health and diagnostic counters.
     *
     * Queries FreeRTOS stack high-water marks, heap statistics, and aggregates
     * the shared counters (packets enqueued/dropped, missed ticks, queue high-water).
     * Safe to call from any task; does not block.
     */
    [[nodiscard]] DiagnosticsSnapshot diagnostics() const;
    /**
     * @brief Concrete fallback command interface fulfillment.
     * Intercepts framework-wide actions; overrides CommandHandler.
     */
    void handle(Command command) override;
};

/** @} */ // End of Core: Device Orchestration

#endif // DEVICE_H