#ifndef DEVICE_H
#define DEVICE_H

/**
 * @file Device.h
 * @brief Declares the Device base class that orchestrates sensors, commands, and telemetry dispatch.
 */

#include "EventHandler.h"
#include "CommandHandler.h"
#include "TelemetryPackage.h"
#include <Arduino.h>

struct hw_timer_s;
typedef struct hw_timer_s hw_timer_t;

/**
 * @brief Base class for composed IoT devices with asynchronous sampling and telemetry pipelines.
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
        EventHandler* sensorInstance;
        int targetEventIdentifier;
    };

    static const uint8_t MAXIMUM_SCHEDULED_SLOTS = 8;
    ScheduledTaskSlot scheduledSequences[MAXIMUM_SCHEDULED_SLOTS];
    uint8_t registeredSlotCount;
    uint8_t activeSequencePointer;

    TaskHandle_t hardwareSamplingTaskHandle;
    TaskHandle_t networkDispatcherTaskHandle;
    QueueHandle_t telemetryQueueHandle;

    hw_timer_t* hardwareTimer;
    const unsigned long pollingIntervalInMilliseconds;
    const uint8_t hardwareTimerIndex;

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
    virtual ~Device();

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
    bool enqueueTelemetryPayload(TelemetryPackage** telemetryPackagePointerAddress);

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
    virtual void processQueuedTelemetryData(const TelemetryPackage* rawQueueItemPayload) = 0;
};

#endif // DEVICE_H