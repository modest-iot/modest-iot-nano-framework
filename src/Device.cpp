/**
 * @file Device.cpp
 * @brief Implements asynchronous scheduling, queueing, and telemetry worker routines.
 */

#include "Device.h"

Device::Device(unsigned long samplingIntervalInMilliseconds, uint8_t timerGroupIndex)
    : registeredSlotCount(0),
      activeSequencePointer(0),
      hardwareSamplingTaskHandle(nullptr),
      networkDispatcherTaskHandle(nullptr),
      telemetryQueueHandle(nullptr),
      hardwareTimer(nullptr),
      pollingIntervalInMilliseconds(samplingIntervalInMilliseconds),
      hardwareTimerIndex(timerGroupIndex)
{
    for (uint8_t i = 0; i < MAXIMUM_SCHEDULED_SLOTS; ++i) {
        scheduledSequences[i] = { nullptr, -1 };
    }
}

Device::~Device() {
    if (hardwareTimer != nullptr) {
        timerAlarmDisable(hardwareTimer);
        timerDetachInterrupt(hardwareTimer);
        hardwareTimer = nullptr;
    }
    if (hardwareSamplingTaskHandle != nullptr) vTaskDelete(hardwareSamplingTaskHandle);
    if (networkDispatcherTaskHandle != nullptr) vTaskDelete(networkDispatcherTaskHandle);
    if (telemetryQueueHandle != nullptr) vQueueDelete(telemetryQueueHandle);
}

void IRAM_ATTR Device::handleHardwareTimerInterrupt(void* deviceInstance) {
    if (deviceInstance != nullptr) {
        Device* self = static_cast<Device*>(deviceInstance);
        BaseType_t higherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(self->hardwareSamplingTaskHandle, &higherPriorityTaskWoken);
        if (higherPriorityTaskWoken) portYIELD_FROM_ISR();
    }
}

void Device::initializeAsynchronousEngine(uint8_t maximumQueueLength) {
    telemetryQueueHandle = xQueueCreate(maximumQueueLength, sizeof(TelemetryPackage*));

    xTaskCreatePinnedToCore(&Device::executeHardwareSamplingContext, "HardwareWorker", 4096, this, 3, &hardwareSamplingTaskHandle, 1);
    xTaskCreatePinnedToCore(&Device::executeNetworkDispatchContext, "NetworkWorker", 8192, this, 1, &networkDispatcherTaskHandle, 1);

    hardwareTimer = timerBegin(hardwareTimerIndex, 80, true);
    timerAttachInterruptArg(hardwareTimer, &Device::handleHardwareTimerInterrupt, this, true);

    uint64_t alarmIntervalInMicroseconds = static_cast<uint64_t>(pollingIntervalInMilliseconds) * 1000;
    timerAlarmWrite(hardwareTimer, alarmIntervalInMicroseconds, true);
    timerAlarmEnable(hardwareTimer);
}

bool Device::appendSensorToScheduler(EventHandler* targetSensorInstance, int requestedEventIdentifier) {
    if (registeredSlotCount >= MAXIMUM_SCHEDULED_SLOTS || targetSensorInstance == nullptr) {
        return false;
    }
    scheduledSequences[registeredSlotCount] = { targetSensorInstance, requestedEventIdentifier };
    registeredSlotCount++;
    return true;
}

bool Device::enqueueTelemetryPayload(TelemetryPackage** telemetryPackagePointerAddress) {
    if (telemetryQueueHandle == nullptr || telemetryPackagePointerAddress == nullptr) return false;
    return xQueueSend(telemetryQueueHandle, telemetryPackagePointerAddress, 0) == pdPASS;
}

void Device::executeHardwareSamplingContext(void* taskParameters) {
    Device* self = static_cast<Device*>(taskParameters);
    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (self->registeredSlotCount > 0) {
            uint8_t currentSlotIndex = self->activeSequencePointer;
            ScheduledTaskSlot activeTask = self->scheduledSequences[currentSlotIndex];

            if (activeTask.sensorInstance != nullptr) {
                activeTask.sensorInstance->on(Event(activeTask.targetEventIdentifier));
            }

            self->activeSequencePointer = (currentSlotIndex + 1) % self->registeredSlotCount;
        }
    }
}

void Device::executeNetworkDispatchContext(void* taskParameters) {
    Device* self = static_cast<Device*>(taskParameters);
    TelemetryPackage* incomingPolymorphicPacketPointer = nullptr;

    while (true) {
        if (xQueueReceive(self->telemetryQueueHandle, &incomingPolymorphicPacketPointer, portMAX_DELAY) == pdPASS) {
            if (incomingPolymorphicPacketPointer != nullptr) {
                self->processQueuedTelemetryData(incomingPolymorphicPacketPointer);
                delete incomingPolymorphicPacketPointer;
                incomingPolymorphicPacketPointer = nullptr;
            }
        }
    }
}