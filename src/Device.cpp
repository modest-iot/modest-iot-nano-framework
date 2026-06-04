/**
* @file Device.cpp
 * @brief Implements the Device interface for the ModestIoT library.
 *
 * Placeholder for the abstract Device class in the ModestIoT (C++ Edition) Arduino library.
 * Concrete implementations must define the on() and handle() methods to process events and commands.
 *
 * @author Angel Velasquez
 * @date March 22, 2025
 * @version 0.1
 */

/*
 * This file is part of the ModestIoT Arduino library (C++ Edition).
 * Copyright (c) 2025 Angel Velasquez
 *
 * Licensed under the Creative Commons Attribution-NoDerivatives 4.0 International (CC BY-ND 4.0).
 * You may use, copy, and distribute this software in its original, unmodified form, provided
 * you give appropriate credit to the original author (Angel Velasquez) and include this notice.
 * Modifications, adaptations, or derivative works are not permitted.
 *
 * Full license text: https://creativecommons.org/licenses/by-nd/4.0/legalcode
 */

#include "Device.h"


Device::Device()
    : registeredSlotCount(0),
      activeSequencePointer(0),
      hardwareSamplingTaskHandle(nullptr),
      networkDispatcherTaskHandle(nullptr),
      telemetryQueueHandle(nullptr) {

    for (uint8_t i = 0; i < MAXIMUM_SCHEDULED_SLOTS; ++i) {
        scheduledSequences[i] = { nullptr, -1 };
    }
}

Device::~Device() {
    if (hardwareSamplingTaskHandle != nullptr) vTaskDelete(hardwareSamplingTaskHandle);
    if (networkDispatcherTaskHandle != nullptr) vTaskDelete(networkDispatcherTaskHandle);
    if (telemetryQueueHandle != nullptr) vQueueDelete(telemetryQueueHandle);
}

void Device::initializeAsynchronousEngine(size_t elementSize, uint8_t maximumQueueLength) {
    telemetryQueueHandle = xQueueCreate(maximumQueueLength, elementSize);

    // Core 1, Priority 3: Fast, hard-deterministic real-time hardware scanning loop
    xTaskCreatePinnedToCore(&Device::executeHardwareSamplingContext, "HardwareWorker", 4096, this, 3, &hardwareSamplingTaskHandle, 1);

    // Core 1, Priority 1: Low-priority network I/O execution loop (Shields hardware from internet lag)
    xTaskCreatePinnedToCore(&Device::executeNetworkDispatchContext, "NetworkWorker", 8192, this, 1, &networkDispatcherTaskHandle, 1);
}

bool Device::appendSensorToScheduler(EventHandler* targetSensorInstance, int requestedEventIdentifier) {
    if (registeredSlotCount >= MAXIMUM_SCHEDULED_SLOTS || targetSensorInstance == nullptr)
        return false;
    scheduledSequences[registeredSlotCount] = { targetSensorInstance, requestedEventIdentifier };
    registeredSlotCount++;
    return true;
}

void IRAM_ATTR Device::notifyHardwareTimerInterrupt() {
    if (hardwareSamplingTaskHandle != nullptr) {
        BaseType_t higherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(hardwareSamplingTaskHandle, &higherPriorityTaskWoken);
        if (higherPriorityTaskWoken) portYIELD_FROM_ISR();
    }
}

bool Device::enqueueTelemetryPayload(const void* telemetryDataSnapshot) {
    if (telemetryQueueHandle == nullptr || telemetryDataSnapshot == nullptr) return false;
    return xQueueSend(telemetryQueueHandle, telemetryDataSnapshot, 0) == pdPASS;
}

// --- FREE RTOS BACKGROUND WORKER THREAD LOOPS ---

void Device::executeHardwareSamplingContext(void* taskParameters) {
    Device* self = static_cast<Device*>(taskParameters);
    while (true) {
        // Task sleeps instantly with 0% CPU consumption until hardware timer alarm interrupts
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (self->registeredSlotCount > 0) {
            uint8_t currentSlotIndex = self->activeSequencePointer;
            ScheduledTaskSlot activeTask = self->scheduledSequences[currentSlotIndex];

            if (activeTask.sensorInstance != nullptr)
                // Safely sample slow physical components inside this thread context
                activeTask.sensorInstance->on(Event(activeTask.targetEventIdentifier));

            // General Round-Robin Pointer Rotation
            self->activeSequencePointer = (currentSlotIndex + 1) % self->registeredSlotCount;
        }
    }
}

void Device::executeNetworkDispatchContext(void* taskParameters) {
    Device* self = static_cast<Device*>(taskParameters);
    uint8_t* memoryBufferProtectionZone = new uint8_t[128];

    while (true) {
        // Task blocks until data gets pushed into the telemetry queue channel
        if (xQueueReceive(self->telemetryQueueHandle, memoryBufferProtectionZone, portMAX_DELAY) == pdPASS) {
            // Unload byte payload directly into the child class network wrapper hook
            self->processQueuedTelemetryData(memoryBufferProtectionZone);
        }
    }
}