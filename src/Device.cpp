#include "Device.h"

Device::Device()
    : registeredSlotCount(0),
      activeSequencePointer(0),
      hardwareSamplingTaskHandle(nullptr),
      networkDispatcherTaskHandle(nullptr),
      telemetryQueueHandle(nullptr) {
    for (uint8_t i = 0; i < MAXIMUM_SCHEDULED_SLOTS; ++i) {
        scheduledSequences[i] = {nullptr, -1};
    }
}

Device::~Device() {
    if (hardwareSamplingTaskHandle != nullptr) vTaskDelete(hardwareSamplingTaskHandle);
    if (networkDispatcherTaskHandle != nullptr) vTaskDelete(networkDispatcherTaskHandle);
    if (telemetryQueueHandle != nullptr) vQueueDelete(telemetryQueueHandle);
}

void Device::initializeAsynchronousEngine(uint8_t maximumQueueLength) {
    // Configure the queue to explicitly track memory bounds matching object pointer addresses
    telemetryQueueHandle = xQueueCreate(maximumQueueLength, sizeof(TelemetryPackage *));

    // Core 1, Priority 3: Fast, hard-deterministic real-time hardware scanning loop
    xTaskCreatePinnedToCore(&Device::executeHardwareSamplingContext, "HardwareWorker", 4096, this, 3,
                            &hardwareSamplingTaskHandle, 1);

    // Core 1, Priority 1: Low-priority network I/O execution loop (Shields hardware from internet lag)
    xTaskCreatePinnedToCore(&Device::executeNetworkDispatchContext, "NetworkWorker", 8192, this, 1,
                            &networkDispatcherTaskHandle, 1);
}

bool Device::appendSensorToScheduler(EventHandler *targetSensorInstance, int requestedEventIdentifier) {
    if (registeredSlotCount >= MAXIMUM_SCHEDULED_SLOTS || targetSensorInstance == nullptr) {
        return false;
    }
    scheduledSequences[registeredSlotCount] = {targetSensorInstance, requestedEventIdentifier};
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

bool Device::enqueueTelemetryPayload(TelemetryPackage **telemetryPackagePointerAddress) {
    if (telemetryQueueHandle == nullptr || telemetryPackagePointerAddress == nullptr) return false;
    return xQueueSend(telemetryQueueHandle, telemetryPackagePointerAddress, 0) == pdPASS;
}

void Device::executeHardwareSamplingContext(void *taskParameters) {
    Device *self = static_cast<Device *>(taskParameters);
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

void Device::executeNetworkDispatchContext(void *taskParameters) {
    Device *self = static_cast<Device *>(taskParameters);
    TelemetryPackage *incomingPolymorphicPacketPointer = nullptr;

    while (true) {
        if (xQueueReceive(self->telemetryQueueHandle, &incomingPolymorphicPacketPointer, portMAX_DELAY) == pdPASS) {
            if (incomingPolymorphicPacketPointer != nullptr) {
                // Pass dynamic polymorphic instance directly to extension hook
                self->processQueuedTelemetryData(incomingPolymorphicPacketPointer);

                // DEALLOCATION SAFEGUARD: Clean dynamic heap memory inside the consumer context
                delete incomingPolymorphicPacketPointer;
                incomingPolymorphicPacketPointer = nullptr;
            }
        }
    }
}
