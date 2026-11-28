/**
 * @file Sensor.cpp
 * @brief Implements the Sensor base class.
 * @copyright (c) 2025-2026 Angel Velasquez
 *
 * Provides core functionality for sensors in the Modest IoT Nano-framework, including event
 * propagation to an assigned handler. Subclasses should configure hardware and define specific
 * event generation logic.
 *
 * @author Angel Velasquez
 * @date 2026-06-06
 * @version 1.0
 */

#include "Sensor.h"
#include <limits>

/**
 * @brief Stores the target GPIO pin and optional event handler for this sensor.
 */
Sensor::Sensor(const int signalGpioPin, EventHandler* parentHandler)
     : pin(signalGpioPin), eventHandler(parentHandler), consecutiveFailures(0), faultThreshold(0), isFaulted(false) {}

/**
 * @brief Copies sensor configuration and state, including atomic lifecycle state.
 */
Sensor::Sensor(const Sensor& other)
    : EventHandler(other),
      pin(other.pin),
      eventHandler(other.eventHandler),
      samplingIntervalInMilliseconds(other.samplingIntervalInMilliseconds),
      minimalSamplingIntervalInMilliseconds(other.minimalSamplingIntervalInMilliseconds),
      consecutiveFailures(other.consecutiveFailures),
      faultThreshold(other.faultThreshold),
      isFaulted(other.isFaulted.load(std::memory_order_acquire)),
      hasSuccessfulRead(other.hasSuccessfulRead.load(std::memory_order_acquire)),
      lastSuccessfulReadAt(other.lastSuccessfulReadAt.load(std::memory_order_acquire)),
      pendingLifecycleEventIdentifier(other.pendingLifecycleEventIdentifier) {}

/**
 * @brief Copies sensor configuration and state, including atomic lifecycle state.
 */
Sensor& Sensor::operator=(const Sensor& other) {
    if (this != &other) {
        pin = other.pin;
        eventHandler = other.eventHandler;
        samplingIntervalInMilliseconds = other.samplingIntervalInMilliseconds;
        minimalSamplingIntervalInMilliseconds = other.minimalSamplingIntervalInMilliseconds;
        consecutiveFailures = other.consecutiveFailures;
        faultThreshold = other.faultThreshold;
        isFaulted.store(other.isFaulted.load(std::memory_order_acquire), std::memory_order_release);
        hasSuccessfulRead.store(other.hasSuccessfulRead.load(std::memory_order_acquire), std::memory_order_release);
        lastSuccessfulReadAt.store(other.lastSuccessfulReadAt.load(std::memory_order_acquire), std::memory_order_release);
        pendingLifecycleEventIdentifier = other.pendingLifecycleEventIdentifier;
    }
    return *this;
}

/**
 * @brief Processes the event locally and then forwards it to the configured handler.
 */
void Sensor::on(Event event) {
    pendingLifecycleEventIdentifier = 0;

    // Execute sensor-specific reading and validation logic
    processEvent(event);

    const int lifecycleEventIdentifier = pendingLifecycleEventIdentifier;
    pendingLifecycleEventIdentifier = 0;

    if (eventHandler == nullptr) {
        return;
    }

    if (lifecycleEventIdentifier != 0) {
        eventHandler->on(Event(lifecycleEventIdentifier, pin));
    }

    // Lifecycle notifications are delivered independently; do not forward a duplicate
    // if the concrete sensor left the same identifier in the mutable data event.
    if (event.identifier == -1 ||
        (lifecycleEventIdentifier != 0 && event.identifier == lifecycleEventIdentifier)) {
        return;
    }

    // Push event upstream to the registered mediator device
    eventHandler->on(event);
}

/**
 * @brief Replaces the current propagation target for this sensor.
 */
void Sensor::setHandler(EventHandler* parentHandler) {
    eventHandler = parentHandler;
}

/**
 * @brief Sets the number of consecutive failures required before emitting a fault event.
 */
void Sensor::setFaultThreshold(uint8_t count) {
    faultThreshold = count;
}

/**
 * @brief Reports whether any successful sample has been recorded.
 */
bool Sensor::hasValidReading() const {
    return hasSuccessfulRead.load(std::memory_order_acquire);
}

/**
 * @brief Returns the age of the most recent successful sample.
 */
uint32_t Sensor::getLastSuccessfulReadAgeInMilliseconds() const {
    if (!hasValidReading()) {
        return std::numeric_limits<uint32_t>::max();
    }

    const uint32_t now = static_cast<uint32_t>(millis());
    const uint32_t lastReadAt = lastSuccessfulReadAt.load(std::memory_order_acquire);
    return now - lastReadAt;
}

/**
 * @brief Classifies the latest sample using a caller-supplied freshness limit.
 */
Sensor::DataValidity Sensor::getDataValidity(const uint32_t maximumAgeInMilliseconds) const {
    if (!hasValidReading()) {
        return DataValidity::NoValidSample;
    }

    return getLastSuccessfulReadAgeInMilliseconds() <= maximumAgeInMilliseconds
        ? DataValidity::Fresh
        : DataValidity::Stale;
}

/**
 * @brief Reports whether a read fault is currently latched.
 */
bool Sensor::isReadFaultActive() const {
    return isFaulted.load(std::memory_order_acquire);
}

/**
 * @brief Reports a successful read, potentially triggering a recovery event if previously faulted.
 * If the sensor was in a faulted state, emits SENSOR_RECOVERED_EVENT_IDENTIFIER (23) once
 * and clears the fault state. Resets the consecutive failure counter.
 */
void Sensor::reportReadSuccess(Event& event) {
    lastSuccessfulReadAt.store(static_cast<uint32_t>(millis()), std::memory_order_release);
    hasSuccessfulRead.store(true, std::memory_order_release);

    if (isFaulted.exchange(false, std::memory_order_acq_rel)) {
        // Emit recovery event once, then clear fault state
        event.identifier = SENSOR_RECOVERED_EVENT_IDENTIFIER;
        event.sourceId = pin;
        pendingLifecycleEventIdentifier = SENSOR_RECOVERED_EVENT_IDENTIFIER;
    }
    consecutiveFailures = 0;
}

/**
 * @brief Reports a failed read, potentially triggering a fault event after threshold consecutive failures.
 * If fault detection is enabled (threshold > 0) and consecutive failures reach the threshold,
 * emits SENSOR_FAULT_EVENT_IDENTIFIER (22) exactly once (latching state).
 */
void Sensor::reportReadFailure(Event& event) {
    if (consecutiveFailures < std::numeric_limits<uint8_t>::max()) {
        consecutiveFailures++;
    }
    
    // Only emit fault event if threshold is configured and not already faulted
    if (faultThreshold > 0 && !isFaulted.load(std::memory_order_acquire) && consecutiveFailures >= faultThreshold) {
        event.identifier = SENSOR_FAULT_EVENT_IDENTIFIER;
        event.sourceId = pin;
        isFaulted.store(true, std::memory_order_release);
        pendingLifecycleEventIdentifier = SENSOR_FAULT_EVENT_IDENTIFIER;
    }
}