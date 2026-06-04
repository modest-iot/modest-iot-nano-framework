#ifndef DEVICE_H
#define DEVICE_H

/**
 * @file Device.h
 * @brief Declares the Device interface.
 *
 * This abstract base class in the Modest IoT Nano-framework combines event and command handling,
 * serving as the foundation for complete IoT devices that integrate sensors and actuators.
 *
 * @author Angel Velasquez
 * @date March 22, 2025
 * @version 0.1
 */

/*
 * This file is part of the Modest IoT Nano-framework (C++ Edition).
 * Copyright (c) 2025 Angel Velasquez
 *
 * Licensed under the Creative Commons Attribution-NoDerivatives 4.0 International (CC BY-ND 4.0).
 * You may use, copy, and distribute this software in its original, unmodified form, provided
 * you give appropriate credit to the original author (Angel Velasquez) and include this notice.
 * Modifications, adaptations, or derivative works are not permitted.
 *
 * Full license text: https://creativecommons.org/licenses/by-nd/4.0/legalcode
 */

#include "EventHandler.h"
#include "CommandHandler.h"
#include <Arduino.h>
#include <cstdint>

class Device : public EventHandler, public CommandHandler {
private:
    /**
     * @brief Internal scheduler entry that maps a sensor instance to an event identifier.
     */
    struct ScheduledTaskSlot {
        EventHandler* sensorInstance;
        int targetEventIdentifier;
    };

    static const uint8_t MAXIMUM_SCHEDULED_SLOTS = 8;
    ScheduledTaskSlot scheduledSequences[MAXIMUM_SCHEDULED_SLOTS];
    uint8_t registeredSlotCount;
    uint8_t activeSequencePointer;

    // FreeRTOS OS Kernel Objects
    TaskHandle_t hardwareSamplingTaskHandle;
    TaskHandle_t networkDispatcherTaskHandle;
    QueueHandle_t telemetryQueueHandle;

    static void executeHardwareSamplingContext(void* taskParameters);
    static void executeNetworkDispatchContext(void* taskParameters);
protected:
    /**
     * @brief Creates a device with no scheduled sensors and no background workers running.
     */
    Device();

    /**
     * @brief Stops background workers and releases asynchronous engine resources.
     */
    ~Device() override;

    /**
     * @brief Spawns background FreeRTOS tasks and structures.
     * @param elementSize Size in bytes of each telemetry payload stored in the queue.
     * @param maximumQueueLength Maximum number of telemetry payloads that can wait in the queue.
     */
    void initializeAsynchronousEngine(size_t elementSize, uint8_t maximumQueueLength = 5);

    /**
     * @brief Pushes telemetry memory packets out to the low-priority background dispatcher.
     * @param telemetryDataSnapshot Pointer to a telemetry payload with the size configured for the queue.
     * @return True when the payload was enqueued successfully.
     */
    bool enqueueTelemetryPayload(const void* telemetryDataSnapshot);

    /**
     * @brief Registers a sensor driver component directly into the automated round-robin scheduler.
     * @param targetSensorInstance Sensor handler to invoke during the sampling cycle.
     * @param requestedEventIdentifier Event identifier passed to the sensor during sampling.
     * @return True when the sensor was added to the scheduler.
     */
    bool appendSensorToScheduler(EventHandler* targetSensorInstance, int requestedEventIdentifier);

    /**
     * @brief PURE VIRTUAL EXTENSION HOOK: Implementations unpack telemetry and process transmissions.
     * @param rawQueueItemPayload Raw telemetry bytes received from the asynchronous queue.
     */
    virtual void processQueuedTelemetryData(const void* rawQueueItemPayload) = 0;
public:
    /**
     * @brief Hardware timer ISR entry point used to wake the sensor sampling task.
     */
    void IRAM_ATTR notifyHardwareTimerInterrupt();
};

#endif // DEVICE_H
