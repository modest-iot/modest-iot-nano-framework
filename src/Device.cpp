/**
 * @file Device.cpp
 * @brief Implements asynchronous scheduling, queueing, and telemetry worker routines.
 * @date 2026-06-06
 * @version 1.0
 */

#include "Device.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/idf_additions.h"
#include "esp_attr.h"

#ifndef traceISR_EXIT_TO_SCHEDULER
#define traceISR_EXIT_TO_SCHEDULER()
#endif

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
    for (auto& slot : scheduledSequences) {
        slot = { nullptr, -1 };
    }
}

Device::~Device() {
    if (hardwareTimer != nullptr) {
        timerEnd(hardwareTimer);
        timerDetachInterrupt(hardwareTimer);
        hardwareTimer = nullptr;
    }
    if (hardwareSamplingTaskHandle != nullptr) vTaskDelete(hardwareSamplingTaskHandle);
    if (networkDispatcherTaskHandle != nullptr) vTaskDelete(networkDispatcherTaskHandle);
    if (telemetryQueueHandle != nullptr) vQueueDelete(telemetryQueueHandle);
}

void IRAM_ATTR Device::handleHardwareTimerInterrupt(void* deviceInstance) {
    if (deviceInstance != nullptr) {
        auto* self = static_cast<Device*>(deviceInstance);
        BaseType_t higherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(self->hardwareSamplingTaskHandle, &higherPriorityTaskWoken);
        if (higherPriorityTaskWoken) portYIELD_FROM_ISR();
    }
}

void Device::initializeAsynchronousEngine(uint8_t maximumQueueLength) {
    telemetryQueueHandle = xQueueCreate(maximumQueueLength, sizeof(TelemetryPackage*));

    xTaskCreatePinnedToCore(&Device::executeHardwareSamplingContext, "HardwareWorker", 4096, this, 3, &hardwareSamplingTaskHandle, 1);
    xTaskCreatePinnedToCore(&Device::executeNetworkDispatchContext, "NetworkWorker", 8192, this, 1, &networkDispatcherTaskHandle, 1);

    hardwareTimer = timerBegin(1000000);
    timerAttachInterruptArg(hardwareTimer, &Device::handleHardwareTimerInterrupt, this);

    auto alarmIntervalInMicroseconds = static_cast<uint64_t>(pollingIntervalInMilliseconds) * 1000;
    timerAlarm(hardwareTimer, alarmIntervalInMicroseconds, true, 0);
}

bool Device::appendSensorToScheduler(EventHandler* targetSensorInstance, int requestedEventIdentifier) {
    if (registeredSlotCount >= MAXIMUM_SCHEDULED_SLOTS || targetSensorInstance == nullptr) {
        return false;
    }
    scheduledSequences[registeredSlotCount] = { targetSensorInstance, requestedEventIdentifier };
    registeredSlotCount++;
    return true;
}

bool Device::enqueueTelemetryPayload(TelemetryPackage** telemetryPackagePointerAddress) const {
    if (telemetryQueueHandle == nullptr || telemetryPackagePointerAddress == nullptr) return false;
    return xQueueSend(telemetryQueueHandle, telemetryPackagePointerAddress, 0) == pdPASS;
}

void Device::executeHardwareSamplingContext(void* taskParameters) {
    auto* self = static_cast<Device*>(taskParameters);
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
    auto* self = static_cast<Device*>(taskParameters);
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

/**
 * @brief Default fallback implementation of CommandHandler interface.
 * Allows derived application controllers to completely omit empty override boilerplate.
 */
void Device::handle(Command command) {
    // Intentional default no-op for devices that only process Events up
}

/**
 * @brief Default fallback implementation of Telemetry Queue dispatcher hook.
 * Allows action/control-only nodes to ignore the network queue cycle seamlessly.
 */
void Device::processQueuedTelemetryData(const TelemetryPackage* rawQueueItemPayload) const {
    // Intentional default no-op
}