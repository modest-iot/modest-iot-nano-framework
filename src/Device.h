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
#include "CommandHandler.h"
#include "TelemetryPackage.h"
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
    /**
     * @brief Internal scheduler slot that maps one sensor to one event request.
     */
    struct ScheduledTaskSlot {
        EventHandler* sensorInstance; ///< Pointer to the sensor handler.
        int targetEventIdentifier; ///< Event identifier requested from the sensor.
    };

    static constexpr uint8_t MAXIMUM_SCHEDULED_SLOTS = 8; ///< Maximum number of sensor slots.
    ScheduledTaskSlot scheduledSequences[MAXIMUM_SCHEDULED_SLOTS] = {}; ///< Array of scheduled task slots.
    uint8_t registeredSlotCount; ///< Number of registered slots.
    uint8_t activeSequencePointer; ///< Index of the currently active sequence.

    TaskHandle_t hardwareSamplingTaskHandle; ///< Handle for the hardware sampling task.
    TaskHandle_t networkDispatcherTaskHandle; ///< Handle for the network dispatcher task.
    QueueHandle_t telemetryQueueHandle; ///< Handle for the telemetry queue.

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
     * @brief Registers a sensor/event pair in the round-robin scheduler.
     * @param targetSensorInstance Sensor (or any `EventHandler`) to poll.
     * @param requestedEventIdentifier Event identifier to request from that sensor.
     * @return `true` when the slot is registered, `false` when scheduler capacity is full.
     */
    bool appendSensorToScheduler(EventHandler* targetSensorInstance, int requestedEventIdentifier);

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
     * @brief Concrete fallback command interface fulfillment.
     * Intercepts framework-wide actions; overrides CommandHandler.
     */
    void handle(Command command) override;
};

/** @} */ // End of Core: Device Orchestration

#endif // DEVICE_H