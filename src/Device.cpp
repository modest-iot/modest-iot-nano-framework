/**
 * @file Device.cpp
 * @brief Implements asynchronous scheduling, queueing, and telemetry worker routines.
 * @date 2026-06-06
 * @version 1.0
 */

#include "Device.h"
#include "ModestLog.h"
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
      packetsEnqueuedCount(0),
      packetsDroppedCount(0),
      queueHighWaterMarkCount(0),
      missedTickCount(0),
      firstDropLogged(false),
      hardwareTimer(nullptr),
      pollingIntervalInMilliseconds(samplingIntervalInMilliseconds),
      hardwareTimerIndex(timerGroupIndex)
{
    for (auto& slot : scheduledSequences) {
        slot = { nullptr, -1 };
    }
}

Device::~Device() {
    // Note: Derived classes should call stopAsynchronousEngine() in their destructor
    // before their own destructor body runs, because sensor/actuator members are
    // destroyed before ~Device() executes. As a safety net, we stop the engine here
    // if it's still running, but tasks may have already accessed freed members.
    stopAsynchronousEngine();
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
    EngineConfig config;
    config.queueLength = maximumQueueLength;
    initializeAsynchronousEngine(config);
}

void Device::initializeAsynchronousEngine(const EngineConfig& config) {
    telemetryQueueHandle = xQueueCreate(config.queueLength, sizeof(TelemetryPackage*));

    xTaskCreatePinnedToCore(
        &Device::executeHardwareSamplingContext, "HardwareWorker",
        config.samplerStackBytes, this, config.samplerPriority,
        &hardwareSamplingTaskHandle, config.samplerCore
    );
    xTaskCreatePinnedToCore(
        &Device::executeNetworkDispatchContext, "NetworkWorker",
        config.networkStackBytes, this, config.networkPriority,
        &networkDispatcherTaskHandle, config.networkCore
    );

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
    if (xQueueSend(telemetryQueueHandle, telemetryPackagePointerAddress, 0) == pdPASS) {
        packetsEnqueuedCount++;
        uint8_t currentQueueSize = static_cast<uint8_t>(uxQueueMessagesWaiting(telemetryQueueHandle));
        if (currentQueueSize > queueHighWaterMarkCount) {
            queueHighWaterMarkCount = currentQueueSize;
        }
        return true;
    }
    packetsDroppedCount++;
    if (!firstDropLogged) {
        MODEST_LOG_W("Device", "First packet dropped: telemetry queue full");
        firstDropLogged = true;
    }
    return false;
}

void Device::executeHardwareSamplingContext(void* taskParameters) {
    auto* self = static_cast<Device*>(taskParameters);
    while (true) {
        // Wait for timer tick or termination signal (bit 0 = tick, bit 1 = stop)
        uint32_t notificationValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Check termination signal (bit 1)
        if (notificationValue & 0x2) {
            break;
        }

        if (self->registeredSlotCount > 0) {
            uint8_t currentSlotIndex = self->activeSequencePointer;
            ScheduledTaskSlot activeTask = self->scheduledSequences[currentSlotIndex];

            if (activeTask.sensorInstance != nullptr) {
                activeTask.sensorInstance->on(Event(activeTask.targetEventIdentifier));
            }

            self->activeSequencePointer = (currentSlotIndex + 1) % self->registeredSlotCount;
        }
    }
    vTaskDelete(nullptr);
}

void Device::executeNetworkDispatchContext(void* taskParameters) {
    auto* self = static_cast<Device*>(taskParameters);
    TelemetryPackage* incomingPolymorphicPacketPointer = nullptr;

    while (true) {
        // Check for termination signal (bit 1 = stop) without clearing the notification
        uint32_t notificationValue;
        if (xTaskNotifyWait(pdFALSE, pdTRUE, &notificationValue, 0) == pdTRUE) {
            if (notificationValue & 0x2) {
                break;
            }
        }

        // Poll queue with a timeout so network maintenance (MQTT keep-alive, connection checks) executes regularly
        if (xQueueReceive(self->telemetryQueueHandle, &incomingPolymorphicPacketPointer, pdMS_TO_TICKS(100)) == pdPASS) {
            if (incomingPolymorphicPacketPointer != nullptr) {
                self->processQueuedTelemetryData(incomingPolymorphicPacketPointer);
                delete incomingPolymorphicPacketPointer;
                incomingPolymorphicPacketPointer = nullptr;
            }
        }
        self->maintainNetworkServices();
    }
    vTaskDelete(nullptr);
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

/**
 * @brief Default fallback implementation of background network services hook.
 * Allows devices without background keep-alive requirements to ignore the service cycle seamlessly.
 */
void Device::maintainNetworkServices() const {
    // Intentional default no-op
}

DiagnosticsSnapshot Device::diagnostics() const {
    DiagnosticsSnapshot snapshot;
    snapshot.missedTicks = missedTickCount;
    snapshot.worstCaseWakeMicros = 0; // Will be populated when SamplingScheduler is implemented
    snapshot.packetsEnqueued = packetsEnqueuedCount;
    snapshot.packetsDropped = packetsDroppedCount;
    snapshot.queueHighWaterMark = queueHighWaterMarkCount;
    
    // Aggregate gateway send statistics if a gateway is registered
    if (telemetryGateway != nullptr) {
        GatewayStatistics gwStats = telemetryGateway->getStatistics();
        snapshot.sendsSucceeded = gwStats.sendsSucceeded;
        snapshot.sendsFailed = gwStats.sendsFailed;
    } else {
        snapshot.sendsSucceeded = 0;
        snapshot.sendsFailed = 0;
    }
    
    snapshot.freeHeapBytes = ESP.getFreeHeap();
    snapshot.minFreeHeapBytes = ESP.getMinFreeHeap();
    snapshot.uptimeSeconds = millis() / 1000;

    // Query FreeRTOS stack high-water marks (returned in StackType_t words, not bytes)
    if (hardwareSamplingTaskHandle != nullptr) {
        snapshot.samplerStackFreeWords = static_cast<uint16_t>(
            uxTaskGetStackHighWaterMark(hardwareSamplingTaskHandle));
    }
    if (networkDispatcherTaskHandle != nullptr) {
        snapshot.networkStackFreeWords = static_cast<uint16_t>(
            uxTaskGetStackHighWaterMark(networkDispatcherTaskHandle));
    }

    return snapshot;
}

void Device::stopAsynchronousEngine() {
    // Guard against double-stop
    if (hardwareSamplingTaskHandle == nullptr && networkDispatcherTaskHandle == nullptr) {
        return;
    }

    // 1. Stop the hardware timer first to prevent new sampler ticks
    if (hardwareTimer != nullptr) {
        timerEnd(hardwareTimer);
        timerDetachInterrupt(hardwareTimer);
        hardwareTimer = nullptr;
    }

    // 2. Signal both tasks to terminate cooperatively using bit 1 (0x2)
    if (hardwareSamplingTaskHandle != nullptr) {
        xTaskNotify(hardwareSamplingTaskHandle, 0x2, eSetValueWithOverwrite);
    }
    if (networkDispatcherTaskHandle != nullptr) {
        xTaskNotify(networkDispatcherTaskHandle, 0x2, eSetValueWithOverwrite);
    }

    // 3. Wait for tasks to terminate (they call vTaskDelete(nullptr) at loop exit)
    // Give them time to finish their current iteration and exit
    vTaskDelay(pdMS_TO_TICKS(100));

    // 4. Clean up task handles (tasks should have already deleted themselves)
    hardwareSamplingTaskHandle = nullptr;
    networkDispatcherTaskHandle = nullptr;

    // 5. Drain and delete any pending telemetry packets in the queue
    if (telemetryQueueHandle != nullptr) {
        TelemetryPackage* pendingPacket = nullptr;
        while (xQueueReceive(telemetryQueueHandle, &pendingPacket, 0) == pdPASS) {
            delete pendingPacket;
        }
        vQueueDelete(telemetryQueueHandle);
        telemetryQueueHandle = nullptr;
    }
}