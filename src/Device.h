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
    Device();
    virtual ~Device();

    /**
     * @brief Spawns background FreeRTOS tasks and structures.
     */
    void initializeAsynchronousEngine(size_t elementSize, uint8_t maximumQueueLength = 5);

    /**
     * @brief Pushes telemetry memory packets out to the low-priority background dispatcher.
     */
    bool enqueueTelemetryPayload(const void* telemetryDataSnapshot);

    /**
     * @brief Registers a sensor driver component directly into the automated round-robin scheduler.
     */
    bool appendSensorToScheduler(EventHandler* targetSensorInstance, int requestedEventIdentifier);

    /**
     * @brief PURE VIRTUAL EXTENSION HOOK: Implementations unpack telemetry and process transmissions.
     */
    virtual void processQueuedTelemetryData(const void* rawQueueItemPayload) = 0;
public:
    /**
         * @brief Hardware ISR vectors signal this method to advance the scheduling cycle.
         */
    void IRAM_ATTR notifyHardwareTimerInterrupt();};

#endif // DEVICE_H
