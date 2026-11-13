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
#include <atomic>

/**
 * @brief Configuration parameters for the asynchronous FreeRTOS engine.
 * @ingroup core_module
 *
 * Allows tuning stack sizes, task priorities, and core pinning without modifying framework source.
 * Default values match the original hard-coded parameters: sampler (4096 bytes, priority 3, core 1),
 * network worker (8192 bytes, priority 1, core 1), queue length 10.
 */
struct EngineConfiguration {
    uint32_t    samplerStackBytes   = 4096; ///< Stack size for the hardware sampling task.
    uint32_t    networkStackBytes   = 8192; ///< Stack size for the network dispatcher task.
    uint8_t     samplerPriority     = 3; ///< FreeRTOS priority for the sampler task.
    uint8_t     networkPriority     = 1; ///< FreeRTOS priority for the network task.
    BaseType_t  samplerCore         = 1; ///< Core pinning for the sampler task.
    BaseType_t  networkCore         = 1; ///< Core pinning for the network task.
    uint8_t     queueLength         = 10; ///< Maximum telemetry queue depth.
};

/**
 * @brief Runtime health snapshot containing stack, heap, queue, and timing metrics.
 * @ingroup core_module
 *
 * Returned by value from `Device::diagnostics()`. Stack margins are reported in
 * FreeRTOS `StackType_t` words (4 bytes on ESP32), not raw bytes.
 */
struct DiagnosticsSnapshot {
    uint32_t    missedTicks             = 0; ///< Ticks missed by the sampler due to handler overrun.
    uint32_t    worstCaseWakeMicros     = 0; ///< Highest observed sampler handler duration.
    uint32_t    packetsEnqueued         = 0; ///< Total telemetry packets successfully enqueued.
    uint32_t    packetsDropped          = 0; ///< Total telemetry packets dropped due to full queue.
    uint32_t    packetsDroppedOldest    = 0; ///< Total oldest packets dropped to make room for new ones.
    uint8_t     queueHighWaterMark      = 0; ///< Maximum queue depth observed.
    uint32_t    sendsSucceeded          = 0; ///< Successful gateway transmissions.
    uint32_t    sendsFailed             = 0; ///< Failed gateway transmissions.
    uint32_t    freeHeapBytes           = 0; ///< Current free heap in bytes.
    uint32_t    minFreeHeapBytes        = 0; ///< Lowest free heap observed since boot.
    uint16_t    samplerStackFreeWords   = 0; ///< Sampler task stack high-water mark (words).
    uint16_t    networkStackFreeWords   = 0; ///< Network task stack high-water mark (words).
    uint32_t    uptimeSeconds           = 0; ///< Device uptime in seconds.
};


/**
 * @brief Base class for composed IoT devices with asynchronous sampling and telemetry pipelines.
 * @ingroup core_module
 *
 * `Device` coordinates periodic sensor polling, queue-based telemetry handoff, and
 * dispatch hooks implemented by subclasses.
 */
// Named constants to avoid magic numbers and strings.
// MAX_GPIO_INTERRUPTS is at namespace scope because it's used by the file-level isrHandleTable in Device.cpp.
namespace DeviceConstants {
    constexpr uint8_t MAX_GPIO_INTERRUPTS = 64; ///< Maximum GPIO interrupt lines on ESP32.
}

class Device : public EventHandler, public CommandHandler {
private:
    // Named constants to avoid magic numbers and strings
    static constexpr uint32_t   TIMER_FREQUENCY_IN_HERTZ                        = 1000000; ///< Hardware timer frequency (1 MHz).
    static constexpr uint32_t   MILLISECONDS_PER_MICROSECOND                    = 1000; ///< Milliseconds to microseconds conversion.
    static constexpr uint32_t   NETWORK_QUEUE_RECEIVE_TIMEOUT_IN_MILLISECONDS   = 100; ///< Network dispatch queue poll timeout.
    static constexpr uint32_t   TASK_TERMINATION_WAIT_IN_MILLISECONDS           = 100; ///< Wait time for task termination.
    static constexpr int        INVALID_EVENT_IDENTIFIER                        = -1; ///< Sentinel for invalid event identifier.
    static constexpr int        INVALID_COMMAND_IDENTIFIER                      = -1; ///< Sentinel for invalid command identifier.
    static constexpr auto       SAMPLER_TASK_NAME                               = "HardwareWorker"; ///< FreeRTOS sampler task name.
    static constexpr auto       NETWORK_TASK_NAME                               = "NetworkWorker"; ///< FreeRTOS network task name.
    static constexpr auto       DEVICE_COMPONENT_NAME                           = "Device"; ///< Log component name.

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

    // Notification infrastructure
    std::atomic<uint32_t> pendingTicks; ///< ISR-incremented tick counter for bit-flag notifications.

    // Triggering scheduler infrastructure
    /**
     * @brief Entry in the trigger table for an interrupt-driven sensor.
     *
     * All triggering state (interrupt mode, trigger flag) is owned by Device,
     * not by Sensor. This keeps Sensor as a pure sensor abstraction.
     */
    struct TriggerEntry {
        Sensor* sensor          = nullptr;          ///< Pointer to the registered sensor.
        int     eventIdentifier = -1;          ///< Event identifier to emit on trigger.
        int     interruptMode   = CHANGE;        ///< GPIO interrupt mode (Arduino core: RISING, FALLING, CHANGE).
        bool    wasTriggered    = false;         ///< True if this sensor was triggered since last poll.
        uint8_t slotIndex       = 0;             ///< Index in the trigger table (also the trigger handle).
    };

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

    /**
     * @brief Re-arms the hardware timer for the next due sensor sample.
     *
     * Calculates the time until the next sensor sample is due using
     * `scheduler.ticksUntilNextDue()` and sets a one-shot alarm for that interval.
     */
    void rearmTimerForNextDue() const;

    /**
     * @brief Enters ESP32 light sleep mode until the next timer alarm.
     *
     * Configures the timer as the wake source and calls `esp_light_sleep_start()`.
     * The CPU halts until the hardware timer fires, reducing idle power consumption.
     * This method is a no-op when not on ESP32 or when the timer is null.
     */
    void enterLightSleep() const;

    /**
     * @brief Enters ESP32 deep sleep mode until the timer alarm.
     *
     * Configures the timer as the wake source and calls `esp_deep_sleep_start()`.
     * Unlike light sleep, deep sleep shuts down the CPU, Wi-Fi, and most peripherals
     * (~10µA vs ~100mA). The device reboots on wake — only RTC memory survives.
     * This method is a no-op when not on ESP32 or when the timer is null.
     * @runs_on{sampler}
     */
    void enterDeepSleep() const;

public:
    /**
     * @brief Power mode for the hardware timer.
     */
    enum class PowerMode : uint8_t {
        Periodic,   ///< Fixed-period timer alarm (default, backward compatible).
        Tickless,   ///< One-shot timer re-armed after each sampler dispatch.
        DeepSleep   ///< Full CPU shutdown, RTC memory only, timer wake.
    };

    PowerMode powerMode; ///< Current power mode.

    /**
     * @brief Reports the wake reason from the last sleep cycle.
     *
     * Uses `esp_sleep_get_wakeup_cause()` to determine how the device woke up
     * (timer, GPIO, touchpad, ULP, or reset) and prints the result to Serial.
     * Call early in setup() to diagnose unexpected wake cycles.
     * @runs_on{any}
     */
    static void reportWakeReason();

    /**
     * @brief Static ISR handler for interrupt-driven sensor triggers.
     * @param handle Trigger handle (slot index in trigger table).
     *
     * Called from the GPIO ISR when an interrupt-driven sensor fires.
     * Marks the sensor as triggered and wakes the sampler task.
     * Declared public so host tests can simulate ISR triggers.
     */
    static void IRAM_ATTR onSensorTrigger(uint32_t handle);

    /**
     * @brief Processes all triggered sensors in the sampler task context.
     *
     * Iterates the trigger table, and for each sensor that was triggered,
     * clears the flag and calls on() (which internally invokes processEvent()).
     */
    void processTriggeredSensors();

    /**
     * @brief Static pointer for ISR access to the active Device instance.
     * @note Declared public so host tests can set it to simulate ISR scenarios.
     */
    static Device* activeDevice; ///< Static pointer for ISR access to the active Device instance.

    /**
     * @brief Maximum number of interrupt-driven sensors.
     * @note Declared public so host tests can verify capacity limits.
     */
    static constexpr uint8_t MAX_TRIGGERED_SENSORS = 4; ///< Maximum interrupt-driven sensors.

    /**
     * @brief Framework-managed trigger table for interrupt-driven sensors.
     * @note Declared public so host tests can inspect trigger state.
     */
    TriggerEntry triggerTable[MAX_TRIGGERED_SENSORS]; ///< Framework-managed trigger table.

    /**
     * @brief Handle for the inbound command queue.
     * @note Declared public so host tests can simulate queue state.
     */
    QueueHandle_t commandQueueHandle; ///< Handle for the inbound command queue.

    /**
     * @brief Entry in the command queue for cross-task command marshaling.
     *
     * Carries a command and its target handler so the sampler task can deliver
     * the command on its own context, preserving the single-context guarantee.
     */
    struct CommandEntry {
        CommandHandler* targetHandler = nullptr; ///< CommandHandler that should process the command.
        Command command;                         ///< The command to execute.

        CommandEntry() : command(INVALID_COMMAND_IDENTIFIER) {}
        CommandEntry(CommandHandler* handler, const Command commandToExecute) : targetHandler(handler), command(commandToExecute) {}
    };

    /**
     * @brief Processes all pending commands in the sampler task context.
     *
     * Drains the command queue and calls `targetHandler->handle(command)` for
     * each entry on the sampler task's own context.
     */
    void processPendingCommands() const;

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
     * @param engineConfiguration Stack sizes, priorities, core pinning, and queue depth.
     */
    void initializeAsynchronousEngine(const EngineConfiguration& engineConfiguration);

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
     * @brief Registers a sensor for interrupt-driven operation.
     * @param targetSensorInstance Sensor to trigger on GPIO edge.
     * @param requestedEventIdentifier Event identifier to emit on trigger.
     * @param interruptMode GPIO interrupt mode (Arduino core constants: RISING, FALLING, CHANGE).
     * @return Trigger handle (slot index + 1, or 0 on failure).
     *
     * The sensor's pin is configured with attachInterrupt(). When the GPIO
     * edge fires, the ISR marks the sensor as triggered and wakes the sampler
     * task. The sampler then calls the sensor's processEvent() on its own context.
     *
     * Calls after initializeAsynchronousEngine() are rejected.
     */
    virtual uint32_t appendSensorToTriggeringScheduler(Sensor* targetSensorInstance,
                                                int requestedEventIdentifier,
                                                int interruptMode = CHANGE);

    /**
     * @brief Removes an interrupt-driven sensor registration.
     * @param triggerHandle Handle returned by appendSensorToTriggeringScheduler.
     *
     * Detaches the GPIO interrupt and clears the trigger table entry.
     */
    virtual void removeSensorFromTriggeringScheduler(uint32_t triggerHandle);

    /**
     * @brief Registers a sensor with both the sampling and triggering schedulers.
     * @param targetSensorInstance Sensor to register.
     * @param requestedEventIdentifier Event identifier to emit.
     * @param interruptMode GPIO interrupt mode (Arduino core constants: RISING, FALLING, CHANGE).
     * @return `true` when both registrations succeed, `false` otherwise.
     *
     * This is a convenience method for hybrid sensors (e.g., YF-S201 water flow)
     * that need both interrupt-driven pulse counting and periodic sampling.
     */
    virtual bool appendToBothSchedulers(Sensor* targetSensorInstance,
                                 int requestedEventIdentifier,
                                 int interruptMode = CHANGE);

    /**
     * @brief Subclass hook for converting queued payloads into network transmissions.
     * @param rawQueueItemPayload Pointer to the queued telemetry payload.
     * @runs_on{network}
     */
    virtual void processQueuedTelemetryData(const TelemetryPackage* rawQueueItemPayload) const;

    /**
     * @brief Subclass hook for background network and gateway upkeep (e.g., MQTT keep-alive).
     * Invoked periodically by the network dispatcher task during idle queue periods.
     * @runs_on{network}
     */
    virtual void maintainNetworkServices() const;

    /**
     * @brief Sets the power mode for the hardware timer.
     * @param mode Power mode: Periodic (default) or Tickless.
     *
     * In Periodic mode, the hardware timer fires at a fixed interval.
     * In Tickless mode, the timer is re-armed after each sampler dispatch
     * using the time until the next sensor sample is due, allowing the
     * device to sleep between samples.
     *
     * Must be called before initializeAsynchronousEngine() to take effect.
     */
    void setPowerMode(PowerMode mode);

    /**
     * @brief Returns the current power mode.
     * @return The configured PowerMode.
     */
    [[nodiscard]] PowerMode getPowerMode() const;

    /**
     * @brief Gracefully stops the asynchronous engine and releases all resources.
     *
     * Stops the hardware timer, signals both FreeRTOS tasks to exit cooperatively,
     * waits for them to terminate, drains and deletes any pending telemetry packets
     * and commands, and deletes the queues. After calling this method, the engine cannot be restarted.
     *
     * Derived classes must call this method in their destructor **before** their own
     * destructor body runs, because sensor/actuator members are destroyed before
     * ~Device() executes, and a running task could access freed memory.
     */
    void stopAsynchronousEngine();

public:
    GatewayClient* telemetryGateway = nullptr;

    /**
     * @brief Sets the telemetry gateway client for diagnostics aggregation.
     * @param gateway The gateway client to register.
     *
     * Registers a gateway so that diagnostics() can query send statistics
     * (sends succeeded/failed). Call this before initializeAsynchronousEngine().
     */
    void setTelemetryGateway(GatewayClient* gateway);

    /**
     * @brief Notification bit flags for the sampler task.
     *
     * Each bit represents a distinct event source that can wake the sampler task.
     * The sampler task checks these bits to determine which work to perform.
     *
     * | Bit         | Enum Value        | Source                         |
     * |-------------|-------------------|--------------------------------|
     * | Bit 0 (0x1) | TimerTickArrived  | Hardware timer ISR             |
     * | Bit 1 (0x2) | TerminationSignal | stopAsynchronousEngine()       |
     * | Bit 2 (0x4) | SensorTrigger     | GPIO ISR                       |
     * | Bit 3 (0x8) | CommandAvailable  | Device::enqueueCommand()       |
     */
    enum class NotificationBitFlag : uint32_t {
        TimerTickArrived    = 0x1, ///< Hardware timer tick arrived.
        TerminationSignal   = 0x2, ///< Engine termination signal.
        SensorTrigger       = 0x4, ///< Interrupt-driven sensor event.
        CommandAvailable    = 0x8  ///< Command marshaled from network task.
    };

    /**
     * @brief Static C function pointer wrapper for the GPIO ISR.
     *
     * Arduino's attachInterrupt requires a plain function pointer (not a lambda).
     * This static member scans the trigger table and delegates to onSensorTrigger.
     * Declared public so the free C adapter function can call it.
     */
    static void IRAM_ATTR onSensorTriggerWrapper();

    /**
     * @brief Returns a snapshot of runtime health and diagnostic counters.
     *
     * Queries FreeRTOS stack high-water marks, heap statistics, and aggregates
     * the shared counters (packets enqueued/dropped, missed ticks, queue high-water).
     * Safe to call from any task; does not block.
     */
    [[nodiscard]] DiagnosticsSnapshot diagnostics() const;
    /**
     * @brief Enqueues a command for execution on the sampler task context.
     * @param targetHandler The CommandHandler that should process the command.
     * @param command The command to execute.
     * @return `true` when enqueued successfully, `false` when the queue is full.
     *
     * This method enables cross-task command marshaling: the network task
     * receives a command (e.g., via MQTT subscribe) and enqueues it for
     * execution on the sampler task, preserving the single-context guarantee.
     *
     * The sampler task drains the command queue and calls
     * `targetHandler->handle(command)` on its own context.
     */
    virtual bool enqueueCommand(CommandHandler* targetHandler, Command command);

    /**
     * @brief Concrete fallback command interface fulfillment.
     * Intercepts framework-wide actions; overrides CommandHandler.
     */
    void handle(Command command) override;
};

#endif // DEVICE_H