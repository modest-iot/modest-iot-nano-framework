/**
 * @file Device.cpp
 * @brief Implements asynchronous scheduling, queueing, and telemetry worker routines.
 * @date 2026-06-06
 * @version 1.0
 */

#include "Device.h"
#include "ModestLog.h"
#include <memory>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/idf_additions.h"
#include "esp_attr.h"

#ifndef traceISR_EXIT_TO_SCHEDULER
#define traceISR_EXIT_TO_SCHEDULER()
#endif

Device::Device(unsigned long samplingIntervalInMilliseconds, uint8_t timerGroupIndex)
    : scheduler(samplingIntervalInMilliseconds),
      engineStarted(false),
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
      hardwareTimerIndex(timerGroupIndex) {
}

Device::~Device() {
    // Note: Derived classes should call stopAsynchronousEngine() in their destructor
    // before their own destructor body runs, because sensor/actuator members are
    // destroyed before ~Device() executes. As a safety net, we stop the engine here
    // if it's still running, but tasks may have already accessed freed members.
    stopAsynchronousEngine();
}

void IRAM_ATTR Device::handleHardwareTimerInterrupt(void *deviceInstance) {
    if (deviceInstance != nullptr) {
        auto *self = static_cast<Device *>(deviceInstance);
        BaseType_t higherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(self->hardwareSamplingTaskHandle, &higherPriorityTaskWoken);
        if (higherPriorityTaskWoken)
        portYIELD_FROM_ISR();
    }
}

void Device::initializeAsynchronousEngine(uint8_t maximumQueueLength) {
    EngineConfig config;
    config.queueLength = maximumQueueLength;
    initializeAsynchronousEngine(config);
}

void Device::initializeAsynchronousEngine(const EngineConfig &config) {
    // Resolve legacy (two-argument) slots before starting the engine
    // Legacy slots get periodInTicks = total legacy slot count, reproducing the old round-robin
    // This is handled internally: legacy slots are marked with period 0 and resolved here
    engineStarted = true;

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

bool Device::appendSensorToScheduler(EventHandler *targetSensorInstance, int requestedEventIdentifier) {
    if (engineStarted || targetSensorInstance == nullptr || requestedEventIdentifier == -1) {
        return false;
    }
    // Legacy two-argument form: period 0 marks it for resolution at engine start
    // At engine start, legacy slots get periodInTicks = legacy count, reproducing round-robin
    auto result = scheduler.add(targetSensorInstance, requestedEventIdentifier, 0);
    return result != SamplingScheduler::AddResult::TableFull &&
           result != SamplingScheduler::AddResult::InvalidArgument;
}

bool Device::appendSensorToScheduler(Sensor *targetSensorInstance, int requestedEventIdentifier) {
    if (engineStarted || targetSensorInstance == nullptr || requestedEventIdentifier == -1) {
        return false;
    }
    uint32_t periodInMilliseconds = targetSensorInstance->getSamplingIntervalInMilliseconds();
    uint32_t minimumPeriodInMilliseconds = targetSensorInstance->getEffectiveMinimalSamplingIntervalInMilliseconds();
    return appendSensorToScheduler(targetSensorInstance, requestedEventIdentifier, periodInMilliseconds,
                                   minimumPeriodInMilliseconds);
}

bool Device::appendSensorToScheduler(EventHandler *targetSensorInstance, int requestedEventIdentifier,
                                     uint32_t periodInMilliseconds,
                                     uint32_t minimumPeriodInMilliseconds) {
    if (engineStarted || targetSensorInstance == nullptr || requestedEventIdentifier == -1) {
        return false;
    }
    auto result = scheduler.add(targetSensorInstance, requestedEventIdentifier,
                                periodInMilliseconds, minimumPeriodInMilliseconds);
    if (result == SamplingScheduler::AddResult::ClampedToMinimum ||
        result == SamplingScheduler::AddResult::ClampedToTick) {
        const bool clampedToMinimum = result == SamplingScheduler::AddResult::ClampedToMinimum;
        [[maybe_unused]] const char *clampReason = clampedToMinimum ? "minimum" : "tick";
        [[maybe_unused]] const unsigned long clampPeriod = clampedToMinimum
                                                               ? static_cast<unsigned long>(minimumPeriodInMilliseconds)
                                                               : pollingIntervalInMilliseconds;
        MODEST_LOG_W("Device", "Sensor period clamped to %s: %lu ms", clampReason, clampPeriod);
    }
    return result != SamplingScheduler::AddResult::TableFull &&
           result != SamplingScheduler::AddResult::InvalidArgument;
}

bool Device::enqueueTelemetryPayload(TelemetryPackage **telemetryPackagePointerAddress) const {
    if (telemetryQueueHandle == nullptr || telemetryPackagePointerAddress == nullptr) return false;
    if (xQueueSend(telemetryQueueHandle, telemetryPackagePointerAddress, 0) == pdPASS) {
        packetsEnqueuedCount++;
        // Update the high water mark if the current queue size is greater
        if (const auto currentQueueSize = static_cast<uint8_t>(uxQueueMessagesWaiting(telemetryQueueHandle));
            currentQueueSize > queueHighWaterMarkCount) {
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

bool Device::enqueueTelemetryPayload(std::unique_ptr<TelemetryPackage> telemetryPackageStandardUniquePointer) const {
    if (telemetryQueueHandle == nullptr || telemetryPackageStandardUniquePointer == nullptr) return false;
    TelemetryPackage *rawPtr = telemetryPackageStandardUniquePointer.release();
    if (xQueueSend(telemetryQueueHandle, &rawPtr, 0) == pdPASS) {
        // Ownership transferred to the queue.
        packetsEnqueuedCount++;
        if (const auto currentQueueSize = static_cast<uint8_t>(uxQueueMessagesWaiting(telemetryQueueHandle)); currentQueueSize > queueHighWaterMarkCount) {
            queueHighWaterMarkCount = currentQueueSize;
        }
        return true;
    }
    // Restore ownership so the package is deleted automatically on failure.
    telemetryPackageStandardUniquePointer.reset(rawPtr);
    packetsDroppedCount++;
    if (!firstDropLogged) {
        MODEST_LOG_W("Device", "First packet dropped: telemetry queue full");
        firstDropLogged = true;
    }
    return false;
}

void Device::executeHardwareSamplingContext(void *taskParameters) {
    auto *self = static_cast<Device *>(taskParameters);
    while (true) {
        // Wait for timer tick or termination signal
        // vTaskNotifyGiveFromISR increments the notification value on each tick
        // stopAsynchronousEngine sets bit 1 (0x2) to signal termination
        const uint32_t notificationValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Check termination signal (bit 1)
        if (notificationValue & 0x2) {
            break;
        }

        // The notification value is the accumulated tick count since last wake
        // ulTaskNotifyTake(pdTRUE, ...) clears the notification after reading
        uint32_t elapsed = notificationValue;
        if (elapsed == 0) {
            // Should not happen, but treat as 1 tick for safety
            elapsed = 1;
        }
        if (elapsed > 1) {
            self->missedTickCount += (elapsed - 1);
        }

        // Advance the scheduler by the elapsed tick count
        // This handles all due slots, with phase-aligned re-arming
        self->scheduler.advance(elapsed);
    }
    vTaskDelete(nullptr);
}

void Device::executeNetworkDispatchContext(void *taskParameters) {
    const auto *self = static_cast<Device *>(taskParameters);
    TelemetryPackage *incomingPolymorphicPacketPointer = nullptr;

    while (true) {
        // Check for termination signal (bit 1 = stop) without clearing the notification
        uint32_t notificationValue;
        if (xTaskNotifyWait(pdFALSE, pdTRUE, &notificationValue, 0) == pdTRUE) {
            if (notificationValue & 0x2) {
                break;
            }
        }

        // Poll queue with a timeout so network maintenance (MQTT keep-alive, connection checks) executes regularly
        if (xQueueReceive(self->telemetryQueueHandle, &incomingPolymorphicPacketPointer, pdMS_TO_TICKS(100)) ==
            pdPASS) {
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
void Device::processQueuedTelemetryData(const TelemetryPackage *rawQueueItemPayload) const {
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
    snapshot.worstCaseWakeMicros = 0; // Populated when handler timing measurement is added
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
        TelemetryPackage *pendingPacket = nullptr;
        while (xQueueReceive(telemetryQueueHandle, &pendingPacket, 0) == pdPASS) {
            delete pendingPacket;
        }
        vQueueDelete(telemetryQueueHandle);
        telemetryQueueHandle = nullptr;
    }
}
