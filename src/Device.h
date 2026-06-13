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
public:
    /**
     * @brief Concrete fallback command interface fulfillment.
     * Intercepts framework-wide actions; overrides CommandHandler.
     */
    void handle(Command command) override;
};

/** @} */ // End of Core: Device Orchestration

#endif // DEVICE_H