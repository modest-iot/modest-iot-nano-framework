/**
 * @file Device.cpp
 * @brief Implements asynchronous scheduling, queueing, and telemetry worker routines.
 * @date 2026-06-06
 * @version 1.0
 */

#include "Device.h"
#include "ModestLog.h"
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

Device::Device(const unsigned long samplingIntervalInMilliseconds, const uint8_t timerGroupIndex)
    : scheduler(samplingIntervalInMilliseconds),
      engineStarted(false),
      hardwareSamplingTaskHandle(nullptr),
      networkDispatcherTaskHandle(nullptr),
      telemetryQueueHandle(nullptr),
      packetsEnqueuedCount(0),
      packetsDroppedCount(0),
      packetsDroppedOldestCount(0),
      queueHighWaterMarkCount(0),
      missedTickCount(0),
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

void Device::initializeAsynchronousEngine(const uint8_t maximumQueueLength) {
    EngineConfiguration engineConfiguration;
    engineConfiguration.queueLength = maximumQueueLength;
    initializeAsynchronousEngine(engineConfiguration);
}

void Device::initializeAsynchronousEngine(const EngineConfiguration &engineConfiguration) {
    // Resolve legacy (two-argument) slots before starting the engine
    // Legacy slots get periodInTicks = total legacy slot count, reproducing the old round-robin
    // This is handled internally: legacy slots are marked with period 0 and resolved here
    engineStarted = true;

    telemetryQueueHandle = xQueueCreate(engineConfiguration.queueLength, sizeof(TelemetryPackage*));
    commandQueueHandle = xQueueCreate(engineConfiguration.queueLength, sizeof(CommandEntry));

    xTaskCreatePinnedToCore(
        &executeHardwareSamplingContext, SAMPLER_TASK_NAME,
        engineConfiguration.samplerStackBytes, this, engineConfiguration.samplerPriority,
        &hardwareSamplingTaskHandle, engineConfiguration.samplerCore
    );
    xTaskCreatePinnedToCore(
        &executeNetworkDispatchContext, NETWORK_TASK_NAME,
        engineConfiguration.networkStackBytes, this, engineConfiguration.networkPriority,
        &networkDispatcherTaskHandle, engineConfiguration.networkCore
    );

    hardwareTimer = timerBegin(TIMER_FREQUENCY_IN_HERTZ);
    timerAttachInterruptArg(hardwareTimer, &handleHardwareTimerInterrupt, this);

    const auto alarmIntervalInMicroseconds = static_cast<uint64_t>(pollingIntervalInMilliseconds) *
                                             MILLISECONDS_PER_MICROSECOND;
    const bool autoReload = powerMode == PowerMode::Periodic;
    timerAlarm(hardwareTimer, alarmIntervalInMicroseconds, autoReload, 0);
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

bool Device::enqueueTelemetryPayload(TelemetryPackage **telemetryPackagePointerAddress) const {
    if (telemetryQueueHandle == nullptr || telemetryPackagePointerAddress == nullptr) return false;

    // Assign envelope metadata before enqueue attempt so drops are visible as gaps
    TelemetryPackage *packet = *telemetryPackagePointerAddress;
    packet->setEnvelopeMetadata(static_cast<uint32_t>(millis()), nextSequenceNumber++);

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

bool Device::enqueueTelemetryPayload(std::unique_ptr<TelemetryPackage> telemetryPackageStandardUniquePointer) const {
    if (telemetryQueueHandle == nullptr || telemetryPackageStandardUniquePointer == nullptr) return false;

    // Assign envelope metadata before enqueue attempt so drops are visible as gaps
    telemetryPackageStandardUniquePointer->setEnvelopeMetadata(static_cast<uint32_t>(millis()), nextSequenceNumber++);

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

void Device::executeHardwareSamplingContext(void *taskParameters) {
    auto *self = static_cast<Device *>(taskParameters);
    while (true) {
        // Wait for any notification bit to be set, then clear all bits
        uint32_t notificationValue = 0;
        xTaskNotifyWait(pdFALSE, pdTRUE, &notificationValue, portMAX_DELAY);

        // Check termination signal
        if (notificationValue & static_cast<uint32_t>(NotificationBitFlag::TerminationSignal)) {
            break;
        }

        // Timer tick: read and clear the atomic pending tick counter
        if (notificationValue & static_cast<uint32_t>(NotificationBitFlag::TimerTickArrived)) {
            uint32_t elapsed = self->pendingTicks.exchange(0, std::memory_order_acq_rel);
            if (elapsed == 0) {
                // Should not happen, but treat as 1 tick for safety
                elapsed = 1;
            }
            if (elapsed > 1) {
                self->missedTickCount += elapsed - 1;
            }
            // Advance the scheduler by the elapsed tick count
            self->scheduler.advance(elapsed);

            // Power management: re-arm timer and enter sleep mode based on power mode
            if (self->powerMode == PowerMode::Tickless) {
                self->rearmTimerForNextDue();
                self->enterLightSleep();
            } else if (self->powerMode == PowerMode::DeepSleep) {
                self->rearmTimerForNextDue();
                self->enterDeepSleep();
            }
        }

        // Interrupt-driven sensor event
        if (notificationValue & static_cast<uint32_t>(NotificationBitFlag::SensorTrigger)) {
            self->processTriggeredSensors();
        }

        // Command available from network task
        if (notificationValue & static_cast<uint32_t>(NotificationBitFlag::CommandAvailable)) {
            self->processPendingCommands();
        }
    }
    vTaskDelete(nullptr);
}

void Device::executeNetworkDispatchContext(void *taskParameters) {
    const auto *self = static_cast<Device *>(taskParameters);
    TelemetryPackage *incomingPolymorphicPacketPointer = nullptr;

    while (true) {
        // Check for termination signal without clearing the notification
        uint32_t notificationValue;
        if (xTaskNotifyWait(pdFALSE, pdTRUE, &notificationValue, 0) == pdTRUE) {
            if (notificationValue & static_cast<uint32_t>(NotificationBitFlag::TerminationSignal)) {
                break;
            }
        }

        // Poll queue with a timeout so network maintenance (MQTT keep-alive, connection checks) executes regularly
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
    vTaskDelete(nullptr);
}

/**
 * @brief Default fallback implementation of CommandHandler interface.
 * Allows derived application controllers to completely omit empty override boilerplate.
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

Device *Device::activeDevice = nullptr;

// ISR handle table: maps GPIO interrupt number to trigger handle (slot index + 1)
static uint32_t isrHandleTable[DeviceConstants::MAX_GPIO_INTERRUPTS] = {};

/**
 * @brief Static C function pointer wrapper for the GPIO ISR.
 *
 * Arduino's attachInterrupt requires a plain function pointer (not a lambda).
 * This static member scans the trigger table and delegates to onSensorTrigger.
 */
void IRAM_ATTR Device::onSensorTriggerWrapper() {
    if (activeDevice != nullptr) {
        for (uint8_t i = 0; i < MAX_TRIGGERED_SENSORS; i++) {
            if (activeDevice->triggerTable[i].sensor != nullptr) {
                onSensorTrigger(static_cast<uint32_t>(i + 1));
            }
        }
    }
}

/**
 * @brief C adapter function for attachInterrupt compatibility.
 *
 * attachInterrupt requires a plain C function pointer, not a C++ member function.
 * This free function delegates to the static member Device::onSensorTriggerWrapper().
 */
extern "C" {
static void c_interrupt_adapter(void) {
    Device::onSensorTriggerWrapper();
}
}

/**
 * @brief Static ISR handler for interrupt-driven sensor triggers.
 *
 * Called from the GPIO ISR when an interrupt-driven sensor fires.
 * Marks the sensor as triggered and wakes the sampler task.
 */
void IRAM_ATTR Device::onSensorTrigger(const uint32_t handle) {
    if (activeDevice != nullptr && handle > 0 && handle <= MAX_TRIGGERED_SENSORS) {
        const auto index = static_cast<uint8_t>(handle - 1);
        activeDevice->triggerTable[index].wasTriggered = true;
        BaseType_t higherPriorityTaskWoken = pdFALSE;
        xTaskNotifyFromISR(activeDevice->hardwareSamplingTaskHandle,
                           static_cast<uint32_t>(NotificationBitFlag::SensorTrigger),
                           eSetBits, &higherPriorityTaskWoken);
        if (higherPriorityTaskWoken)
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
        if (i.sensor != nullptr && i.wasTriggered) {
            i.wasTriggered = false;
            Event event(i.eventIdentifier);
            event.sourceId = i.sensor->getPin();
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

    // Find an empty slot in the trigger table
    for (uint8_t i = 0; i < MAX_TRIGGERED_SENSORS; i++) {
        if (triggerTable[i].sensor == nullptr) {
            triggerTable[i].sensor = targetSensorInstance;
            triggerTable[i].eventIdentifier = requestedEventIdentifier;
            triggerTable[i].interruptMode = interruptMode;
            triggerTable[i].wasTriggered = false;
            triggerTable[i].slotIndex = i;

            // Set the static active device pointer for ISR access
            activeDevice = this;

            // Attach the GPIO interrupt using the C adapter function
            // (Arduino's attachInterrupt requires a plain C function pointer)
            const uint32_t handle = static_cast<uint32_t>(i + 1);
            const int interruptNumber = digitalPinToInterrupt(targetSensorInstance->getPin());
            isrHandleTable[interruptNumber] = handle;
            attachInterrupt(interruptNumber, c_interrupt_adapter, interruptMode);

            // Return handle (slot index + 1, so 0 is invalid)
            return handle;
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

    if (const uint8_t index = static_cast<uint8_t>(triggerHandle - 1); triggerTable[index].sensor != nullptr) {
        // Detach the GPIO interrupt
        detachInterrupt(digitalPinToInterrupt(triggerTable[index].sensor->getPin()));

        // Clear the trigger table entry (all state is in Device, not Sensor)
        triggerTable[index].sensor = nullptr;
        triggerTable[index].eventIdentifier = INVALID_EVENT_IDENTIFIER;
        triggerTable[index].interruptMode = CHANGE;
        triggerTable[index].wasTriggered = false;
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

    const bool samplingOk = appendSensorToScheduler(targetSensorInstance, requestedEventIdentifier);
    const uint32_t triggerHandle = appendSensorToTriggeringScheduler(targetSensorInstance,
                                                                     requestedEventIdentifier,
                                                                     interruptMode);

    return samplingOk && triggerHandle != 0;
}

/**
 * @name Command Marshaling
 * @{
 *
 * @brief Cross-task command delivery.
 * Commands received in the network task are enqueued and processed
 * in the sampler task for thread safety.
 */

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
 * Drains the command queue and calls `targetHandler->handle(command)` for
 * each entry on the sampler task's own context.
 */
void Device::processPendingCommands() const {
    CommandEntry entry;
    while (xQueueReceive(commandQueueHandle, &entry, 0) == pdPASS) {
        if (entry.targetHandler != nullptr) {
            entry.targetHandler->handle(entry.command);
        }
    }
}

DiagnosticsSnapshot Device::diagnostics() const {
    DiagnosticsSnapshot snapshot;
    snapshot.missedTicks = missedTickCount;
    snapshot.worstCaseWakeMicros = 0; // Populated when handler timing measurement is added
    snapshot.packetsEnqueued = packetsEnqueuedCount;
    snapshot.packetsDropped = packetsDroppedCount;
    snapshot.packetsDroppedOldest = packetsDroppedOldestCount;
    snapshot.queueHighWaterMark = queueHighWaterMarkCount;

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
 * Uses `esp_sleep_get_wakeup_cause()` to determine how the device woke up
 * and prints the result to Serial. Call early in setup() to diagnose
 * unexpected wake cycles.
 */
void Device::reportWakeReason() {
#ifdef ARDUINO_ARCH_ESP32
    switch (esp_sleep_get_wakeup_cause()) {
        case ESP_SLEEP_WAKEUP_EXT0:
            Serial.println(F("[Device] Wakeup: External signal (RTC_IO)"));
            break;
        case ESP_SLEEP_WAKEUP_EXT1:
            Serial.println(F("[Device] Wakeup: External signal (RTC_CNTL)"));
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println(F("[Device] Wakeup: Timer"));
            break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD:
            Serial.println(F("[Device] Wakeup: Touchpad"));
            break;
        case ESP_SLEEP_WAKEUP_ULP:
            Serial.println(F("[Device] Wakeup: ULP coprogram"));
            break;
        default:
            Serial.println(F("[Device] Wakeup: Reset or unknown"));
            break;
    }
#else
    (void)0; // Suppress unused warning in host builds
#endif
}

void Device::setPowerMode(const PowerMode mode) {
    powerMode = mode;
}

Device::PowerMode Device::getPowerMode() const {
    return powerMode;
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

    // 2. Signal both tasks to terminate cooperatively
    if (hardwareSamplingTaskHandle != nullptr) {
        xTaskNotify(hardwareSamplingTaskHandle,
                    static_cast<uint32_t>(NotificationBitFlag::TerminationSignal), eSetBits);
    }
    if (networkDispatcherTaskHandle != nullptr) {
        xTaskNotify(networkDispatcherTaskHandle,
                    static_cast<uint32_t>(NotificationBitFlag::TerminationSignal), eSetBits);
    }

    // 3. Wait for tasks to terminate (they call vTaskDelete(nullptr) at loop exit)
    // Give them time to finish their current iteration and exit
    vTaskDelay(pdMS_TO_TICKS(TASK_TERMINATION_WAIT_IN_MILLISECONDS));

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

    // 6. Drain and delete the command queue
    if (commandQueueHandle != nullptr) {
        CommandEntry pendingCommand;
        while (xQueueReceive(commandQueueHandle, &pendingCommand, 0) == pdPASS) {
            // Commands are value types; no cleanup needed beyond draining
        }
        vQueueDelete(commandQueueHandle);
        commandQueueHandle = nullptr;
    }
}
