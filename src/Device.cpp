/**
 * @file Device.cpp
 * @brief Implements asynchronous scheduling, queueing, and telemetry worker routines.
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-06-06
 * @version 1.0
 */

#include "Device.h"
#include "DiagnosticsTelemetryPackage.h"
#include "ModestLog.h"

#ifdef ARDUINO_ARCH_ESP32
#include "TimeSynchronizationDriver.h"
#endif
#ifdef ARDUINO_ARCH_ESP32
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#endif


#ifndef traceISR_EXIT_TO_SCHEDULER
#define traceISR_EXIT_TO_SCHEDULER()
#endif

// Firmware version tracking for telemetry envelope metadata
static const char* telemetrySchemaVersion = nullptr; ///< Optional schema version for telemetry format tracking.
static const char* firmwareVersion = nullptr; ///< Optional firmware version for fleet management.

Device::Device(const unsigned long samplingIntervalInMilliseconds, const uint8_t timerGroupIndex)
    : scheduler(samplingIntervalInMilliseconds),
      engineStarted(false),
      hardwareSamplingTaskHandle(nullptr),
      networkDispatcherTaskHandle(nullptr),
      telemetryQueueHandle(nullptr),
      highPriorityQueueHandle(nullptr),
      packetsEnqueuedCount(0),
      packetsDroppedCount(0),
      packetsDroppedOldestCount(0),
      queueHighWaterMarkCount(0),
      highPriorityEnqueuedCount(0),
      highPriorityDroppedCount(0),
      highPriorityQueueHighWaterMarkCount(0),
      firstHighPriorityDropLogged(false),
      missedTickCount(0),
      worstCaseWakeMicros(0),
      firstDropLogged(false),
      nextSequenceNumber(0),
      pendingTicks(0),
      hardwareTimer(nullptr),
      pollingIntervalInMilliseconds(samplingIntervalInMilliseconds),
      hardwareTimerIndex(timerGroupIndex),
      powerMode(PowerMode::Periodic),
      commandQueueHandle(nullptr) {
    (void) hardwareTimerIndex; ///< Stored for future use; not yet read.
}

Device::~Device() {
    // Note: Derived classes should call stopAsynchronousEngine() in their destructor
    // before their own destructor body runs, because sensor/actuator members are
    // destroyed before ~Device() executes. As a safety net, we stop the engine here
    // if it's still running, but tasks may have already accessed freed members.
    stopAsynchronousEngine();
}

// LCOV_EXCL_START — FreeRTOS ISR and task creation, cannot run in host tests
void IRAM_ATTR Device::handleHardwareTimerInterrupt(void *deviceInstance) {
    if (deviceInstance != nullptr) {
        auto *self = static_cast<Device *>(deviceInstance);
        // Increment atomic tick counter so the sampler task knows how many ticks occurred
        self->pendingTicks.fetch_add(1, std::memory_order_relaxed);
        // Set bit 0 to wake the sampler task (bit-flag notification)
        BaseType_t higherPriorityTaskWoken = pdFALSE;
        xTaskNotifyFromISR(self->hardwareSamplingTaskHandle,
                           static_cast<uint32_t>(NotificationBitFlag::TimerTickArrived),
                           eSetBits, &higherPriorityTaskWoken);
        if (higherPriorityTaskWoken)
        portYIELD_FROM_ISR();
    }
}

bool Device::initializeAsynchronousEngine(const uint8_t maximumQueueLength) {
    EngineConfiguration engineConfiguration;
    engineConfiguration.queueLength = maximumQueueLength;
    return initializeAsynchronousEngine(engineConfiguration);
}

bool Device::initializeAsynchronousEngine(const EngineConfiguration &engineConfiguration) {
    if (engineStarted.load(std::memory_order_acquire)) {
        return true;
    }
    if (engineStopping.load(std::memory_order_acquire) ||
        engineShutdownComplete.load(std::memory_order_acquire)) {
        return false;
    }
    if (engineConfiguration.queueLength == 0) {
        return false;
    }

    QueueHandle_t newTelemetryQueue = xQueueCreate(engineConfiguration.queueLength, sizeof(TelemetryPackage*));
    QueueHandle_t newHighPriorityQueue = nullptr;
    QueueHandle_t newCommandQueue = nullptr;
    hw_timer_t* newHardwareTimer = nullptr;
    TaskHandle_t newSamplingTask = nullptr;
    TaskHandle_t newNetworkTask = nullptr;

    const auto releasePartialResources = [&]() {
        engineStarted.store(false, std::memory_order_release);
        engineStopping.store(true, std::memory_order_release);
        if (newHardwareTimer != nullptr) {
            timerEnd(newHardwareTimer);
            newHardwareTimer = nullptr;
        }
        if (newNetworkTask != nullptr) {
            xTaskNotify(newNetworkTask,
                        static_cast<uint32_t>(NotificationBitFlag::TerminationSignal), eSetBits);
        }
        if (newSamplingTask != nullptr) {
            xTaskNotify(newSamplingTask,
                        static_cast<uint32_t>(NotificationBitFlag::TerminationSignal), eSetBits);
        }
        while (!hardwareSamplingWorkerExited.load(std::memory_order_acquire) ||
               !networkDispatcherWorkerExited.load(std::memory_order_acquire)) {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        newNetworkTask = nullptr;
        newSamplingTask = nullptr;
        if (newCommandQueue != nullptr) {
            vQueueDelete(newCommandQueue);
            newCommandQueue = nullptr;
        }
        if (newHighPriorityQueue != nullptr) {
            vQueueDelete(newHighPriorityQueue);
            newHighPriorityQueue = nullptr;
        }
        if (newTelemetryQueue != nullptr) {
            vQueueDelete(newTelemetryQueue);
            newTelemetryQueue = nullptr;
        }
        engineStopping.store(false, std::memory_order_release);
    };

    if (newTelemetryQueue == nullptr) {
        return false;
    }
    newHighPriorityQueue = xQueueCreate(4, sizeof(TelemetryPackage*));
    if (newHighPriorityQueue == nullptr) {
        releasePartialResources();
        return false;
    }
    newCommandQueue = xQueueCreate(engineConfiguration.queueLength, sizeof(CommandEntry));
    if (newCommandQueue == nullptr) {
        releasePartialResources();
        return false;
    }

    newHardwareTimer = timerBegin(TIMER_FREQUENCY_IN_HERTZ);
    if (newHardwareTimer == nullptr) {
        releasePartialResources();
        return false;
    }

    hardwareSamplingWorkerExited.store(false, std::memory_order_release);
    const BaseType_t samplingTaskResult = xTaskCreatePinnedToCore(
        &executeHardwareSamplingContext, SAMPLER_TASK_NAME,
        engineConfiguration.samplerStackBytes, this, engineConfiguration.samplerPriority,
        &newSamplingTask, engineConfiguration.samplerCore
    );
    if (samplingTaskResult != pdPASS || newSamplingTask == nullptr) {
        if (newSamplingTask == nullptr) {
            hardwareSamplingWorkerExited.store(true, std::memory_order_release);
        }
        releasePartialResources();
        return false;
    }

    networkDispatcherWorkerExited.store(false, std::memory_order_release);
    const BaseType_t networkTaskResult = xTaskCreatePinnedToCore(
        &executeNetworkDispatchContext, NETWORK_TASK_NAME,
        engineConfiguration.networkStackBytes, this, engineConfiguration.networkPriority,
        &newNetworkTask, engineConfiguration.networkCore
    );
    if (networkTaskResult != pdPASS || newNetworkTask == nullptr) {
        if (newNetworkTask == nullptr) {
            networkDispatcherWorkerExited.store(true, std::memory_order_release);
        }
        releasePartialResources();
        return false;
    }

    telemetryQueueHandle = newTelemetryQueue;
    highPriorityQueueHandle = newHighPriorityQueue;
    commandQueueHandle = newCommandQueue;
    hardwareSamplingTaskHandle = newSamplingTask;
    networkDispatcherTaskHandle = newNetworkTask;
    hardwareTimer = newHardwareTimer;

    timerAttachInterruptArg(hardwareTimer, &handleHardwareTimerInterrupt, this);
    const auto alarmIntervalInMicroseconds = static_cast<uint64_t>(pollingIntervalInMilliseconds) *
                                             MILLISECONDS_PER_MICROSECOND;
    const bool autoReload = powerMode == PowerMode::Periodic;
    timerAlarm(hardwareTimer, alarmIntervalInMicroseconds, autoReload, 0);

    engineStarted.store(true, std::memory_order_release);
    for (auto& entry : triggerTable) {
        if (entry.registrationActive.load(std::memory_order_acquire)) {
            attachInterruptArg(entry.interruptNumber, &handleSensorTrigger, &entry, entry.interruptMode);
            entry.interruptAttached = true;
        }
    }
    return true;
}
// LCOV_EXCL_END

bool Device::isAsynchronousEngineStarted() const {
    return engineStarted.load(std::memory_order_acquire);
}

bool Device::appendSensorToScheduler(EventHandler *targetSensorInstance, const int requestedEventIdentifier) {
    if (engineStarted || targetSensorInstance == nullptr || requestedEventIdentifier == INVALID_EVENT_IDENTIFIER)
        return false;
    // Legacy two-argument form: period 0 marks it for resolution at engine start
    // At engine start, legacy slots get periodInTicks = legacy count, reproducing round-robin
    const auto result = scheduler.add(targetSensorInstance, requestedEventIdentifier, 0);
    return result != SamplingScheduler::AddResult::TableFull &&
           result != SamplingScheduler::AddResult::InvalidArgument;
}

bool Device::appendSensorToScheduler(Sensor *targetSensorInstance, const int requestedEventIdentifier) {
    if (engineStarted || targetSensorInstance == nullptr || requestedEventIdentifier == INVALID_EVENT_IDENTIFIER) {
        return false;
    }
    const uint32_t periodInMilliseconds = targetSensorInstance->getSamplingIntervalInMilliseconds();
    const uint32_t minimumPeriodInMilliseconds = targetSensorInstance->
            getEffectiveMinimalSamplingIntervalInMilliseconds();
    return appendSensorToScheduler(targetSensorInstance, requestedEventIdentifier, periodInMilliseconds,
                                   minimumPeriodInMilliseconds);
}

bool Device::appendSensorToScheduler(EventHandler *targetSensorInstance, const int requestedEventIdentifier,
                                     const uint32_t periodInMilliseconds,
                                     const uint32_t minimumPeriodInMilliseconds) {
    if (engineStarted || targetSensorInstance == nullptr || requestedEventIdentifier == INVALID_EVENT_IDENTIFIER) {
        return false;
    }
    const auto result = scheduler.add(targetSensorInstance, requestedEventIdentifier,
                                      periodInMilliseconds, minimumPeriodInMilliseconds);
    if (result == SamplingScheduler::AddResult::ClampedToMinimum ||
        result == SamplingScheduler::AddResult::ClampedToTick) {
        const bool clampedToMinimum = result == SamplingScheduler::AddResult::ClampedToMinimum;
        [[maybe_unused]] const char *clampReason = clampedToMinimum ? "minimum" : "tick";
        [[maybe_unused]] const unsigned long clampPeriod = clampedToMinimum
                                                               ? static_cast<unsigned long>(minimumPeriodInMilliseconds)
                                                               : pollingIntervalInMilliseconds;
        MODEST_LOG_W(DEVICE_COMPONENT_NAME, "Sensor period clamped to %s: %lu ms", clampReason, clampPeriod);
    }
    return result != SamplingScheduler::AddResult::TableFull &&
           result != SamplingScheduler::AddResult::InvalidArgument;
}

bool Device::appendSensorToScheduler(EventHandler *targetSensorInstance, const int requestedEventIdentifier,
                                     const uint32_t periodInMilliseconds,
                                     const uint32_t minimumPeriodInMilliseconds,
                                     const float deadband) {
    if (engineStarted || targetSensorInstance == nullptr || requestedEventIdentifier == INVALID_EVENT_IDENTIFIER) {
        return false;
    }
    const auto result = scheduler.add(targetSensorInstance, requestedEventIdentifier,
                                      periodInMilliseconds, minimumPeriodInMilliseconds);
    if (result == SamplingScheduler::AddResult::TableFull ||
        result == SamplingScheduler::AddResult::InvalidArgument) {
        return false;
    }
    // Configure deadband for the slot that was just added (slot index = count - 1)
    const uint8_t slotIndex = static_cast<uint8_t>(scheduler.count() - 1);
    floatChangeFilter.configureSlotWithDeadband(slotIndex, deadband);
    return true;
}

bool Device::appendSensorToScheduler(Sensor *targetSensorInstance, const int requestedEventIdentifier,
                                     const float deadband) {
    if (engineStarted || targetSensorInstance == nullptr || requestedEventIdentifier == INVALID_EVENT_IDENTIFIER) {
        return false;
    }
    const uint32_t periodInMilliseconds = targetSensorInstance->getSamplingIntervalInMilliseconds();
    const uint32_t minimumPeriodInMilliseconds = targetSensorInstance->
            getEffectiveMinimalSamplingIntervalInMilliseconds();
    return appendSensorToScheduler(targetSensorInstance, requestedEventIdentifier,
                                   periodInMilliseconds, minimumPeriodInMilliseconds, deadband);
}

// LCOV_EXCL_START — FreeRTOS queue operations, cannot run in host tests
bool Device::enqueueTelemetryPayload(TelemetryPackage **telemetryPackagePointerAddress) const {
    if (telemetryQueueHandle == nullptr || telemetryPackagePointerAddress == nullptr) return false;

    // Assign envelope metadata before enqueue attempt so drops are visible as gaps
    TelemetryPackage *packet = *telemetryPackagePointerAddress;
#if defined(ARDUINO_ARCH_ESP32)
    packet->setEnvelopeMetadata(TimeSynchronizationDriver::getCurrentTimestamp(), nextSequenceNumber++);
#else
    packet->setEnvelopeMetadata(static_cast<uint32_t>(millis()), nextSequenceNumber++);
#endif
    packet->setVersionMetadata(telemetrySchemaVersion, firmwareVersion);

    if (xQueueSend(telemetryQueueHandle, telemetryPackagePointerAddress, 0) == pdPASS) {
        packetsEnqueuedCount++;
        // Update the high water mark if the current queue size is greater
        if (const auto currentQueueSize = static_cast<uint8_t>(uxQueueMessagesWaiting(telemetryQueueHandle));
            currentQueueSize > queueHighWaterMarkCount) {
            queueHighWaterMarkCount = currentQueueSize;
        }
        return true;
    }
    // Queue is full: drop the oldest packet and insert the new one
    TelemetryPackage *oldest = nullptr;
    if (xQueueReceive(telemetryQueueHandle, &oldest, 0) == pdPASS && oldest != nullptr) {
        delete oldest;
        packetsDroppedOldestCount++;
        if (xQueueSend(telemetryQueueHandle, telemetryPackagePointerAddress, 0) == pdPASS) {
            packetsEnqueuedCount++;
            if (const auto currentQueueSize = static_cast<uint8_t>(uxQueueMessagesWaiting(telemetryQueueHandle));
                currentQueueSize > queueHighWaterMarkCount) {
                queueHighWaterMarkCount = currentQueueSize;
            }
            return true;
        }
    }
    packetsDroppedCount++;
    if (!firstDropLogged) {
        MODEST_LOG_W(DEVICE_COMPONENT_NAME, "First packet dropped: telemetry queue full");
        firstDropLogged = true;
    }
    return false;
}

void Device::assignEnvelopeMetadata(TelemetryPackage* package) const {
#if defined(ARDUINO_ARCH_ESP32)
    package->setEnvelopeMetadata(TimeSynchronizationDriver::getCurrentTimestamp(), nextSequenceNumber++);
#else
    package->setEnvelopeMetadata(static_cast<uint32_t>(millis()), nextSequenceNumber++);
#endif
    package->setVersionMetadata(telemetrySchemaVersion, firmwareVersion);
}
// LCOV_EXCL_END

// LCOV_EXCL_START — FreeRTOS queue operations, cannot run in host tests
void Device::drainTelemetryQueue(QueueHandle_t& queueHandle) {
    if (queueHandle == nullptr) return;
    TelemetryPackage* pending = nullptr;
    while (xQueueReceive(queueHandle, &pending, 0) == pdPASS) {
        delete pending;
    }
    vQueueDelete(queueHandle);
    queueHandle = nullptr;
}
// LCOV_EXCL_END

// LCOV_EXCL_START — FreeRTOS queue operations, cannot run in host tests
bool Device::enqueueTelemetryPayload(std::unique_ptr<TelemetryPackage> telemetryPackageStandardUniquePointer) const {
    if (telemetryQueueHandle == nullptr || telemetryPackageStandardUniquePointer == nullptr) return false;

    // Assign envelope metadata before enqueue attempt so drops are visible as gaps
    assignEnvelopeMetadata(telemetryPackageStandardUniquePointer.get());

    TelemetryPackage *rawPackagePointer = telemetryPackageStandardUniquePointer.release();
    if (xQueueSend(telemetryQueueHandle, &rawPackagePointer, 0) == pdPASS) {
        // Ownership transferred to the queue.
        packetsEnqueuedCount++;
        if (const auto currentQueueSize = static_cast<uint8_t>(uxQueueMessagesWaiting(telemetryQueueHandle));
            currentQueueSize > queueHighWaterMarkCount) {
            queueHighWaterMarkCount = currentQueueSize;
        }
        return true;
    }
    // Queue is full: drop the oldest packet and insert the new one
    TelemetryPackage *oldestPackage = nullptr;
    if (xQueueReceive(telemetryQueueHandle, &oldestPackage, 0) == pdPASS && oldestPackage != nullptr) {
        delete oldestPackage;
        packetsDroppedOldestCount++;
        if (xQueueSend(telemetryQueueHandle, &rawPackagePointer, 0) == pdPASS) {
            packetsEnqueuedCount++;
            if (const auto currentQueueSize = static_cast<uint8_t>(uxQueueMessagesWaiting(telemetryQueueHandle));
                currentQueueSize > queueHighWaterMarkCount) {
                queueHighWaterMarkCount = currentQueueSize;
            }
            return true;
        }
    }
    // Restore ownership so the package is deleted automatically on failure.
    telemetryPackageStandardUniquePointer.reset(rawPackagePointer);
    packetsDroppedCount++;
    if (!firstDropLogged) {
        MODEST_LOG_W(DEVICE_COMPONENT_NAME, "First packet dropped: telemetry queue full");
        firstDropLogged = true;
    }
    return false;
}

bool Device::enqueueHighPriorityPayload(std::unique_ptr<TelemetryPackage> highPriorityPayload) const {
    if (highPriorityQueueHandle == nullptr || highPriorityPayload == nullptr) return false;

    // Assign envelope metadata and high priority before enqueue
    assignEnvelopeMetadata(highPriorityPayload.get());
    highPriorityPayload->markAsHighPriority();

    TelemetryPackage *rawHighPriorityPointer = highPriorityPayload.release();
    if (xQueueSend(highPriorityQueueHandle, &rawHighPriorityPointer, 0) == pdPASS) {
        // Ownership transferred to the high-priority queue.
        highPriorityEnqueuedCount++;
        if (const auto currentQueueSize = static_cast<uint8_t>(uxQueueMessagesWaiting(highPriorityQueueHandle));
            currentQueueSize > highPriorityQueueHighWaterMarkCount) {
            highPriorityQueueHighWaterMarkCount = currentQueueSize;
        }
        return true;
    }
    // High-priority queue is full: drop the new packet (high-priority data is too critical to evict)
    delete rawHighPriorityPointer;
    highPriorityDroppedCount++;
    if (!firstHighPriorityDropLogged) {
        MODEST_LOG_W(DEVICE_COMPONENT_NAME, "First high-priority packet dropped: high-priority queue full");
        firstHighPriorityDropLogged = true;
    }
    return false;
}
// LCOV_EXCL_END

// LCOV_EXCL_START — FreeRTOS task context, cannot run in host tests
void Device::executeHardwareSamplingContext(void *taskParameters) {
    auto *self = static_cast<Device *>(taskParameters);
    while (!self->engineStarted.load(std::memory_order_acquire) &&
           !self->engineStopping.load(std::memory_order_acquire)) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    while (!self->engineStopping.load(std::memory_order_acquire)) {
        // Wait for any notification bit to be set, then clear all bits
        uint32_t notificationValue = 0;
        xTaskNotifyWait(pdFALSE, pdTRUE, &notificationValue, portMAX_DELAY);

        // Check termination signal
        if (notificationValue & static_cast<uint32_t>(NotificationBitFlag::TerminationSignal)) {
            break;
        }

        uint32_t elapsedTimerTicks = 0;
        if (notificationValue & static_cast<uint32_t>(NotificationBitFlag::TimerTickArrived)) {
            elapsedTimerTicks = self->pendingTicks.exchange(0, std::memory_order_acq_rel);
        }
        self->processSamplerNotification(notificationValue, elapsedTimerTicks);
    }
    self->hardwareSamplingWorkerExited.store(true, std::memory_order_release);
    vTaskDelete(nullptr);
}

void Device::processSamplerNotification(const uint32_t notificationValue, uint32_t elapsedTimerTicks) {
    uint32_t synchronousWorkDurationMicros = 0;

    if (notificationValue & static_cast<uint32_t>(NotificationBitFlag::TimerTickArrived)) {
        if (elapsedTimerTicks == 0) {
            elapsedTimerTicks = 1;
        }
        if (elapsedTimerTicks > 1) {
            missedTickCount.fetch_add(elapsedTimerTicks - 1, std::memory_order_relaxed);
        }

        const uint32_t dispatchStartedAt = static_cast<uint32_t>(micros());
        scheduler.advance(elapsedTimerTicks);
        synchronousWorkDurationMicros += static_cast<uint32_t>(micros()) - dispatchStartedAt;

        // Power-management sleep is intentionally excluded from measured work duration.
        if (powerMode == PowerMode::Tickless) {
            rearmTimerForNextDue();
            enterLightSleep();
        } else if (powerMode == PowerMode::DeepSleep) {
            rearmTimerForNextDue();
            enterDeepSleep();
        }
    }

    if (notificationValue & static_cast<uint32_t>(NotificationBitFlag::SensorTrigger)) {
        const uint32_t triggerStartedAt = static_cast<uint32_t>(micros());
        processTriggeredSensors();
        synchronousWorkDurationMicros += static_cast<uint32_t>(micros()) - triggerStartedAt;
    }

    if (notificationValue & static_cast<uint32_t>(NotificationBitFlag::CommandAvailable)) {
        const uint32_t commandStartedAt = static_cast<uint32_t>(micros());
        processPendingCommands();
        synchronousWorkDurationMicros += static_cast<uint32_t>(micros()) - commandStartedAt;
    }

    uint32_t worstObservedDuration = worstCaseWakeMicros.load(std::memory_order_relaxed);
    while (synchronousWorkDurationMicros > worstObservedDuration &&
           !worstCaseWakeMicros.compare_exchange_weak(worstObservedDuration, synchronousWorkDurationMicros,
                                                      std::memory_order_relaxed)) {
    }
}

void Device::executeNetworkDispatchContext(void *taskParameters) {
    auto *self = static_cast<Device *>(taskParameters);
    TelemetryPackage *incomingPolymorphicPacketPointer = nullptr;

    while (!self->engineStarted.load(std::memory_order_acquire) &&
           !self->engineStopping.load(std::memory_order_acquire)) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    while (!self->engineStopping.load(std::memory_order_acquire)) {
        // Check for termination signal without clearing the notification
        uint32_t notificationValue;
        if (xTaskNotifyWait(pdFALSE, pdTRUE, &notificationValue, 0) == pdTRUE) {
            if (notificationValue & static_cast<uint32_t>(NotificationBitFlag::TerminationSignal)) {
                break;
            }
        }

        // Check high-priority queue first (non-blocking) — high-priority packets are processed before standard telemetry
        if (xQueueReceive(self->highPriorityQueueHandle, &incomingPolymorphicPacketPointer, 0) == pdPASS) {
            if (incomingPolymorphicPacketPointer != nullptr) {
                self->processQueuedTelemetryData(incomingPolymorphicPacketPointer);
                delete incomingPolymorphicPacketPointer;
                incomingPolymorphicPacketPointer = nullptr;
            }
        }

        // Poll standard telemetry queue with a timeout so network maintenance executes regularly
        if (xQueueReceive(self->telemetryQueueHandle, &incomingPolymorphicPacketPointer,
                          pdMS_TO_TICKS(NETWORK_QUEUE_RECEIVE_TIMEOUT_IN_MILLISECONDS)) ==
            pdPASS) {
            if (incomingPolymorphicPacketPointer != nullptr) {
                self->processQueuedTelemetryData(incomingPolymorphicPacketPointer);
                delete incomingPolymorphicPacketPointer;
                incomingPolymorphicPacketPointer = nullptr;
            }
        }
        self->maintainNetworkServices();
    }
    self->networkDispatcherWorkerExited.store(true, std::memory_order_release);
    vTaskDelete(nullptr);
}
// LCOV_EXCL_END

/**
 * @brief Handles framework-internal commands before dispatching to handle().
 * @param command The command to check.
 * @return True if the command was a framework command and was consumed.
 *
 * Currently handles the diagnostics command (commandId = 300).
 * This keeps handle() purely for developer-defined custom commands.
 */
bool Device::handleInternalCommand(const Command command) const {
    if (command.identifier == DIAGNOSTICS_COMMAND_IDENTIFIER) {
        if (telemetryGateway == nullptr) {
            MODEST_LOG_W(DEVICE_COMPONENT_NAME, "Diagnostics requested but no gateway client registered");
            return true;
        }
        const DiagnosticsSnapshot snapshot = collectDiagnostics();
        auto diagPackage = std::make_unique<DiagnosticsTelemetryPackage>(snapshot);
        enqueueTelemetryPayload(std::move(diagPackage));
        return true;
    }
    return false;
}

/**
 * @brief Concrete fallback command interface fulfillment.
 * Overrides CommandHandler; derived classes can override for custom commands.
 *
 * The default implementation does nothing. Framework-internal commands
 * (e.g., diagnostics) are handled separately by handleInternalCommand()
 * before this method is called, so the developer's override receives
 * only custom commands.
 */
void Device::handle(const Command command) {
    (void) command;
    // Intentional default no-op for devices that only process Events up
}

/**
 * @brief Default fallback implementation of Telemetry Queue dispatcher hook.
 * Allows action/control-only nodes to ignore the network queue cycle seamlessly.
 */
void Device::processQueuedTelemetryData(const TelemetryPackage *rawQueueItemPayload) const {
    (void) rawQueueItemPayload;
    // Intentional default no-op
}

/**
 * @brief Default fallback implementation of background network services hook.
 * Allows devices without background keep-alive requirements to ignore the service cycle seamlessly.
 */
void Device::maintainNetworkServices() const {
    // Intentional default no-op
}

/**
 * @name Triggering Scheduler
 * @{
 *
 * @brief Interrupt-driven sensor support.
 * Interrupt-driven sensors register a GPIO ISR that marks a trigger flag;
 * the sampler task drains these flags and dispatches events.
 */

// LCOV_EXCL_START — GPIO ISR and trigger scheduler, cannot run in host tests
/**
 * @brief Static ISR handler for interrupt-driven sensor triggers.
 *
 * Called from the GPIO ISR when an interrupt-driven sensor fires.
 * Marks only the originating registration as triggered and wakes its sampler.
 */
void IRAM_ATTR Device::handleSensorTrigger(void* const triggerContext) {
    if (triggerContext == nullptr) {
        return;
    }

    auto* entry = static_cast<TriggerEntry*>(triggerContext);
    if (!entry->registrationActive.load(std::memory_order_acquire)) {
        return;
    }

    Device* const owner = entry->owner;
    if (owner == nullptr || !owner->engineStarted.load(std::memory_order_acquire)) {
        return;
    }

    TaskHandle_t const samplerTask = owner->hardwareSamplingTaskHandle;
    if (samplerTask == nullptr) {
        return;
    }

    entry->wasTriggered.store(true, std::memory_order_release);
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    xTaskNotifyFromISR(samplerTask,
                       static_cast<uint32_t>(NotificationBitFlag::SensorTrigger),
                       eSetBits, &higherPriorityTaskWoken);
    if (higherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

/**
 * @brief Processes all triggered sensors in the sampler task context.
 *
 * Iterates the trigger table, and for each sensor that was triggered,
 * clears the flag and calls on() (which internally invokes processEvent()).
 * Uses getPin() for sourceId — no friend access needed.
 */
void Device::processTriggeredSensors() {
    for (auto &i: triggerTable) {
        if (i.sensor != nullptr && i.registrationActive.load(std::memory_order_acquire) &&
            i.wasTriggered.exchange(false, std::memory_order_acq_rel)) {
            Event event(i.eventIdentifier);
            event.sourceId = i.pin;
            // Call on() which internally calls processEvent() — no friend access needed.
            i.sensor->on(event);
        }
    }
}

/**
 * @brief Registers a sensor for interrupt-driven operation.
 */
uint32_t Device::appendSensorToTriggeringScheduler(Sensor *targetSensorInstance,
                                                   const int requestedEventIdentifier,
                                                   const int interruptMode) {
    if (engineStarted || targetSensorInstance == nullptr || requestedEventIdentifier == INVALID_EVENT_IDENTIFIER) {
        return 0;
    }

    const int pin = targetSensorInstance->getPin();
    if (pin < 0 || pin >= DeviceConstants::MAX_GPIO_INTERRUPTS) {
        return 0;
    }
    const int interruptNumber = digitalPinToInterrupt(static_cast<uint8_t>(pin));
    if (interruptNumber < 0 || interruptNumber >= DeviceConstants::MAX_GPIO_INTERRUPTS) {
        return 0;
    }

    for (const auto& entry : triggerTable) {
        if (entry.sensor != nullptr && (entry.sensor == targetSensorInstance || entry.pin == pin)) {
            return 0;
        }
    }

    // Find an empty slot in the trigger table
    for (uint8_t i = 0; i < MAX_TRIGGERED_SENSORS; i++) {
        auto& entry = triggerTable[i];
        if (entry.sensor == nullptr) {
            entry.sensor = targetSensorInstance;
            entry.owner = this;
            entry.eventIdentifier = requestedEventIdentifier;
            entry.interruptMode = interruptMode;
            entry.pin = pin;
            entry.interruptNumber = interruptNumber;
            entry.wasTriggered.store(false, std::memory_order_relaxed);
            entry.interruptAttached = false;
            entry.slotIndex = i;
            entry.registrationActive.store(true, std::memory_order_release);

            return static_cast<uint32_t>(i + 1);
        }
    }

    // No empty slot available
    return 0;
}

/**
 * @brief Removes an interrupt-driven sensor registration.
 */
void Device::removeSensorFromTriggeringScheduler(const uint32_t triggerHandle) {
    if (triggerHandle == 0 || triggerHandle > MAX_TRIGGERED_SENSORS)
        return;

    auto& entry = triggerTable[triggerHandle - 1];
    if (entry.sensor != nullptr) {
        entry.registrationActive.store(false, std::memory_order_release);
        if (entry.interruptAttached) {
            detachInterrupt(entry.interruptNumber);
            entry.interruptAttached = false;
        }

        entry.sensor = nullptr;
        entry.eventIdentifier = INVALID_EVENT_IDENTIFIER;
        entry.interruptMode = CHANGE;
        entry.pin = -1;
        entry.interruptNumber = -1;
        entry.wasTriggered.store(false, std::memory_order_relaxed);
    }
}

/**
 * @brief Registers a sensor with both the sampling and triggering schedulers.
 */
bool Device::appendToBothSchedulers(Sensor *targetSensorInstance,
                                    const int requestedEventIdentifier,
                                    const int interruptMode) {
    if (engineStarted || targetSensorInstance == nullptr || requestedEventIdentifier == INVALID_EVENT_IDENTIFIER)
        return false;

    const uint32_t triggerHandle = appendSensorToTriggeringScheduler(targetSensorInstance,
                                                                     requestedEventIdentifier,
                                                                     interruptMode);
    if (triggerHandle == 0) {
        return false;
    }

    if (!appendSensorToScheduler(targetSensorInstance, requestedEventIdentifier)) {
        removeSensorFromTriggeringScheduler(triggerHandle);
        return false;
    }

    return true;
}
/** @} */
// LCOV_EXCL_END

/**
 * @name Command Marshaling
 * @{
 *
 * @brief Cross-task command delivery.
 * Commands received in the network task are enqueued and processed
 * in the sampler task for thread safety.
 */

// LCOV_EXCL_START — FreeRTOS command marshaling, cannot run in host tests
/**
 * @brief Enqueues a command for execution on the sampler task context.
 *
 * The network task calls this method to marshal a command from the network
 * context to the sampler context. The sampler task drains the queue and
 * calls `targetHandler->handle(command)` on its own context.
 */
bool Device::enqueueCommand(CommandHandler *targetHandler, const Command command) {
    if (commandQueueHandle == nullptr || targetHandler == nullptr) {
        return false;
    }

    const CommandEntry entry(targetHandler, command);

    if (const BaseType_t result = xQueueSend(commandQueueHandle, &entry, 0); result == pdPASS) {
        // Wake the sampler task to process the command
        xTaskNotify(hardwareSamplingTaskHandle,
                    static_cast<uint32_t>(NotificationBitFlag::CommandAvailable),
                    eSetBits);
        return true;
    }
    return false;
}

/**
 * @brief Processes all pending commands in the sampler task context.
 *
 * Drains the command queue and first checks for framework-internal commands
 * via handleInternalCommand(). If not consumed, dispatches to
 * `targetHandler->handle(command)` for developer-defined custom commands.
 */
void Device::processPendingCommands() const {
    CommandEntry entry;
    while (xQueueReceive(commandQueueHandle, &entry, 0) == pdPASS) {
        // Framework-internal commands (e.g., diagnostics) are handled first
        if (handleInternalCommand(entry.command)) {
            continue;
        }
        // Custom commands are dispatched to the target handler
        if (entry.targetHandler != nullptr) {
            entry.targetHandler->handle(entry.command);
        }
    }
}
/** @} */
// LCOV_EXCL_END

// LCOV_EXCL_START — FreeRTOS stack high-water marks, cannot run in host tests
DiagnosticsSnapshot Device::collectDiagnostics() const {
    DiagnosticsSnapshot snapshot;
    snapshot.missedTicks = missedTickCount.load(std::memory_order_relaxed);
    snapshot.worstCaseWakeMicros = worstCaseWakeMicros.load(std::memory_order_relaxed);
    snapshot.packetsEnqueued = packetsEnqueuedCount;
    snapshot.packetsDropped = packetsDroppedCount;
    snapshot.packetsDroppedOldest = packetsDroppedOldestCount;
    snapshot.queueHighWaterMark = queueHighWaterMarkCount;
    snapshot.highPriorityEnqueued = highPriorityEnqueuedCount;
    snapshot.highPriorityDropped = highPriorityDroppedCount;
    snapshot.highPriorityQueueHighWaterMark = highPriorityQueueHighWaterMarkCount;

    // Aggregate gateway send statistics if a gateway is registered
    if (telemetryGateway != nullptr) {
        auto [sendsSucceeded, sendsFailed] = telemetryGateway->getStatistics();
        snapshot.sendsSucceeded = sendsSucceeded;
        snapshot.sendsFailed = sendsFailed;
    } else {
        snapshot.sendsSucceeded = 0;
        snapshot.sendsFailed = 0;
    }

    // ESP heap functions are ESP32-specific; default to 0 for host compilation
#ifdef ARDUINO_ARCH_ESP32
    snapshot.freeHeapBytes = ESP.getFreeHeap();
    snapshot.minFreeHeapBytes = ESP.getMinFreeHeap();
#else
    snapshot.freeHeapBytes = 0;
    snapshot.minFreeHeapBytes = 0;
#endif
    snapshot.uptimeSeconds = millis() / MILLISECONDS_PER_MICROSECOND;

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
// LCOV_EXCL_END

// LCOV_EXCL_START — ESP32 power management, cannot run in host tests
/**
 * @brief Re-arms the hardware timer for the next due sensor sample.
 *
 * Calculates the time until the next sensor sample is due using
 * `scheduler.ticksUntilNextDue()` and sets a one-shot alarm for that interval.
 * If no slots are registered, falls back to the polling interval.
 */
void Device::rearmTimerForNextDue() const {
    if (hardwareTimer == nullptr) {
        return;
    }
    const uint32_t ticksUntilNext = scheduler.ticksUntilNextDue();
    const uint32_t ticksToWait = ticksUntilNext > 0 ? ticksUntilNext : 1;
    const uint64_t alarmIntervalInMicroseconds = static_cast<uint64_t>(ticksToWait) *
                                                 pollingIntervalInMilliseconds *
                                                 MILLISECONDS_PER_MICROSECOND;
    timerAlarm(hardwareTimer, alarmIntervalInMicroseconds, false, 0);
}

/**
 * @brief Enters ESP32 light sleep mode until the next timer alarm.
 *
 * Configures the hardware timer as the wake source and calls `esp_light_sleep_start()`.
 * The CPU halts until the timer fires, reducing idle power consumption.
 * This method is guarded by `#ifdef ARDUINO_ARCH_ESP32` for host compilation.
 */
void Device::enterLightSleep() const {
#ifdef ARDUINO_ARCH_ESP32
    if (hardwareTimer == nullptr) {
        return;
    }
    const uint32_t ticksUntilNext = scheduler.ticksUntilNextDue();
    const uint32_t ticksToSleep = ticksUntilNext > 0 ? ticksUntilNext : 1;
    const uint64_t sleepTimeInMicroseconds = static_cast<uint64_t>(ticksToSleep) *
                                             pollingIntervalInMilliseconds *
                                             MILLISECONDS_PER_MICROSECOND;
    esp_sleep_enable_timer_wakeup(sleepTimeInMicroseconds);
    esp_light_sleep_start();
#endif
}

/**
 * @brief Enters ESP32 deep sleep mode until the timer alarm.
 *
 * Calculates the sleep duration from `scheduler.ticksUntilNextDue()`, configures
 * the timer wake source, and calls `esp_deep_sleep_start()`. Unlike light sleep,
 * deep sleep shuts down the CPU, Wi-Fi, and most peripherals (~10µA).
 * The device reboots on wake — only RTC memory survives.
 * This method is guarded by `#ifdef ARDUINO_ARCH_ESP32` for host compilation.
 */
void Device::enterDeepSleep() const {
#ifdef ARDUINO_ARCH_ESP32
    if (hardwareTimer == nullptr) {
        return;
    }
    const uint32_t ticksUntilNext = scheduler.ticksUntilNextDue();
    const uint32_t ticksToSleep = ticksUntilNext > 0 ? ticksUntilNext : 1;
    const uint64_t sleepTimeInMicroseconds = static_cast<uint64_t>(ticksToSleep) *
                                             pollingIntervalInMilliseconds *
                                             MILLISECONDS_PER_MICROSECOND;
    esp_sleep_enable_timer_wakeup(sleepTimeInMicroseconds);
    esp_deep_sleep_start();
#endif
}

/**
 * @brief Reports the wake reason from the last sleep cycle.
 *
 * Uses `esp_sleep_get_wakeup_causes()` (ESP-IDF 6.0+) to determine how the
 * device woke up and prints the result to Serial. Multiple wakeup sources
 * may trigger simultaneously; each is reported on a separate line.
 * Call early in setup() to diagnose unexpected wake cycles.
 */
void Device::reportWakeReason() {
#ifdef ARDUINO_ARCH_ESP32
    const uint32_t causes = esp_sleep_get_wakeup_causes();
    bool reported = false;

    if (causes & BIT(ESP_SLEEP_WAKEUP_EXT0)) {
        Serial.println(F("[Device] Wakeup: External signal (RTC_IO)"));
        reported = true;
    }
    if (causes & BIT(ESP_SLEEP_WAKEUP_EXT1)) {
        Serial.println(F("[Device] Wakeup: External signal (RTC_CNTL)"));
        reported = true;
    }
    if (causes & BIT(ESP_SLEEP_WAKEUP_TIMER)) {
        Serial.println(F("[Device] Wakeup: Timer"));
        reported = true;
    }
    if (causes & BIT(ESP_SLEEP_WAKEUP_TOUCHPAD)) {
        Serial.println(F("[Device] Wakeup: Touchpad"));
        reported = true;
    }
    if (causes & BIT(ESP_SLEEP_WAKEUP_ULP)) {
        Serial.println(F("[Device] Wakeup: ULP coprogram"));
        reported = true;
    }
    if (!reported) {
        Serial.println(F("[Device] Wakeup: Reset or unknown"));
    }
#else
    (void)0; // Suppress unused warning in host builds
#endif
}
// LCOV_EXCL_END

void Device::setTelemetryGateway(GatewayClient* gateway) {
    telemetryGateway = gateway;
}

void Device::setPowerMode(const PowerMode mode) {
    powerMode = mode;
}

Device::PowerMode Device::getPowerMode() const {
    return powerMode;
}

/**
 * @name Firmware Version in Telemetry
 * @{
 *
 * @brief Optional schema and firmware version fields in telemetry envelope metadata.
 * Applications call setFirmwareVersion() to identify devices in fleet telemetry.
 */

const char* Device::getTelemetrySchemaVersion() {
    return telemetrySchemaVersion;
}

void Device::setTelemetrySchemaVersion(const char* version) {
    telemetrySchemaVersion = version;
}

const char* Device::getFirmwareVersion() {
    return firmwareVersion;
}

void Device::setFirmwareVersion(const char* version) {
    firmwareVersion = version;
}
/** @} */

// LCOV_EXCL_START — FreeRTOS task management, cannot run in host tests
void Device::stopAsynchronousEngine() {
    if (engineShutdownComplete.load(std::memory_order_acquire)) {
        return;
    }

    bool hasActiveTriggerRegistration = false;
    for (const auto& entry : triggerTable) {
        if (entry.registrationActive.load(std::memory_order_acquire)) {
            hasActiveTriggerRegistration = true;
            break;
        }
    }
    if (hardwareSamplingTaskHandle == nullptr && networkDispatcherTaskHandle == nullptr &&
        hardwareTimer == nullptr && telemetryQueueHandle == nullptr &&
        highPriorityQueueHandle == nullptr && commandQueueHandle == nullptr &&
        !hasActiveTriggerRegistration) {
        return;
    }

    bool expectedStopping = false;
    while (!engineStopping.compare_exchange_weak(expectedStopping, true,
                                                  std::memory_order_acq_rel)) {
        if (engineShutdownComplete.load(std::memory_order_acquire)) {
            return;
        }
        expectedStopping = false;
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    engineStarted.store(false, std::memory_order_release);

    for (auto& entry : triggerTable) {
        if (entry.registrationActive.exchange(false, std::memory_order_acq_rel)) {
            detachInterrupt(entry.interruptNumber);
        }
    }

    // Stop the hardware timer to prevent new sampler ticks
    if (hardwareTimer != nullptr) {
        timerDetachInterrupt(hardwareTimer);
        timerEnd(hardwareTimer);
        hardwareTimer = nullptr;
    }

    // Signal both tasks to terminate cooperatively
    if (hardwareSamplingTaskHandle != nullptr) {
        xTaskNotify(hardwareSamplingTaskHandle,
                    static_cast<uint32_t>(NotificationBitFlag::TerminationSignal), eSetBits);
    }
    if (networkDispatcherTaskHandle != nullptr) {
        xTaskNotify(networkDispatcherTaskHandle,
                    static_cast<uint32_t>(NotificationBitFlag::TerminationSignal), eSetBits);
    }

    while (!hardwareSamplingWorkerExited.load(std::memory_order_acquire) ||
           !networkDispatcherWorkerExited.load(std::memory_order_acquire)) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    hardwareSamplingTaskHandle = nullptr;
    networkDispatcherTaskHandle = nullptr;

    // Drain and delete any pending packets in all queues
    Device::drainTelemetryQueue(highPriorityQueueHandle);
    Device::drainTelemetryQueue(telemetryQueueHandle);
    if (highPriorityQueueHandle != nullptr) {
        vQueueDelete(highPriorityQueueHandle);
        highPriorityQueueHandle = nullptr;
    }
    if (telemetryQueueHandle != nullptr) {
        vQueueDelete(telemetryQueueHandle);
        telemetryQueueHandle = nullptr;
    }

    // Drain and delete the command queue
    if (commandQueueHandle != nullptr) {
        CommandEntry pendingCommand;
        while (xQueueReceive(commandQueueHandle, &pendingCommand, 0) == pdPASS) {
            // Commands are value types; no cleanup needed beyond draining
        }
        vQueueDelete(commandQueueHandle);
        commandQueueHandle = nullptr;
    }
    engineShutdownComplete.store(true, std::memory_order_release);
}
// LCOV_EXCL_END
