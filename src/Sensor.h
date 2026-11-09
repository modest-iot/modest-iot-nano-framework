#ifndef SENSOR_H
#define SENSOR_H

/**
 * @file Sensor.h
 * @brief Declares the Sensor base class.
 * @ingroup sensors_module
 *
 * This abstract base class represents input devices in the Modest IoT Nano-framework, providing
 * a foundation for sensors that generate events. It includes event propagation to an optional
 * handler, supporting the framework’s event-driven design.
 * 
 * @author Angel Velasquez
 * @date 2026-06-06
 * @version 1.0
 */

/*
 * This file is part of the Modest IoT Nano-framework (C++ Edition).
 * Copyright (c) 2025-2026 Angel Velasquez
 *
 * Licensed under the MIT License.
 * See LICENSE.md in the root of this repository for full terms.
 */

#include <Arduino.h>

#include "EventHandler.h"
#include "Event.h"
#include <cstdint>

class Sensor : public EventHandler {

protected:
    int pin; ///< GPIO pin assigned to the sensor.
    EventHandler* eventHandler; ///< Optional handler to receive propagated events.

    uint32_t samplingIntervalInMilliseconds = 0; ///< Per-instance sampling interval override (0 = use subclass default).
    uint32_t minimalSamplingIntervalInMilliseconds = 0; ///< Per-instance minimum interval override (0 = use subclass default).

    /**
     * @brief Template-method hook that lets subclasses transform or suppress an event before propagation.
     * @param event Event being processed.
     * @runs_on sampler
     */
    virtual void processEvent(Event& event) = 0;

    /**
     * @brief Returns the subclass-specific default sampling interval constant.
     * @return Default sampling interval in milliseconds.
     */
    [[nodiscard]] virtual uint32_t getDefaultSamplingIntervalInMilliseconds() const = 0;

    /**
     * @brief Returns the subclass-specific minimum sampling interval constant.
     * @return Minimum sampling interval in milliseconds.
     */
    [[nodiscard]] virtual uint32_t getMinimalSamplingIntervalInMilliseconds() const = 0;

    /**
     * @brief Reports a successful read, potentially triggering a recovery event if previously faulted.
     * @param event The event to potentially modify.
     */
    void reportReadSuccess(Event& event);

    /**
     * @brief Reports a failed read, potentially triggering a fault event after threshold consecutive failures.
     * @param event The event to potentially modify.
     */
    void reportReadFailure(Event& event);

private:
    uint8_t consecutiveFailures; ///< Count of consecutive read failures.
    uint8_t faultThreshold; ///< Number of consecutive failures before fault event (0 = disabled).
    bool isFaulted; ///< Whether the sensor is currently in a faulted state.

public:
    /// --- FRAMEWORK-WIDE UNIFIED SENSOR SPECIFICATION EVENTS ---
    static constexpr int DATA_READ_EVENT_IDENTIFIER = 20;
    static constexpr int MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER = 21;
    static constexpr int SENSOR_FAULT_EVENT_IDENTIFIER = 22;
    static constexpr int SENSOR_RECOVERED_EVENT_IDENTIFIER = 23;

    static constexpr Event DATA_READ_EVENT = Event(DATA_READ_EVENT_IDENTIFIER);
    static constexpr Event MEASURE_DATA_REQUESTED_EVENT = Event(MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    /**
     * @brief Constructs a Sensor with a pin and optional event handler.
     * @param signalGpioPin The GPIO pin for the sensor.
     * @param parentHandler Pointer to an EventHandler to receive events (default: nullptr).
     */
    Sensor(int signalGpioPin, EventHandler* parentHandler = nullptr);
    ~Sensor() override = default;

    /**
     * @brief Handles an event by propagating it to the assigned handler.
     * @param event The event to handle.
     * @runs_on sampler
     */
    void on(Event event) override;

    /**
     * @brief Sets or updates the event handler for this sensor.
     * @param parentHandler Pointer to the new EventHandler.
     */
    void setHandler(EventHandler* parentHandler);

    /**
     * @brief Sets the number of consecutive failures required before emitting a fault event.
     * @param count Consecutive failure threshold (default: 0 = disabled for backward compatibility).
     * @note Call this from the sketch to opt in to sensor fault detection.
     */
    void setFaultThreshold(uint8_t count);

    /**
     * @brief Returns the effective sampling interval.
     * @return The per-instance override if set, otherwise the subclass default.
     */
    [[nodiscard]] uint32_t getSamplingIntervalInMilliseconds() const {
        return samplingIntervalInMilliseconds
            ? samplingIntervalInMilliseconds
            : getDefaultSamplingIntervalInMilliseconds();
    }

    /**
     * @brief Sets a per-instance sampling interval override.
     * @param intervalMs Sampling interval in milliseconds (0 clears the override).
     */
    void setSamplingIntervalInMilliseconds(uint32_t intervalMs) {
        samplingIntervalInMilliseconds = intervalMs;
    }

    /**
     * @brief Returns the effective minimum sampling interval.
     * @return The per-instance override if set, otherwise the subclass default.
     */
    [[nodiscard]] uint32_t getEffectiveMinimalSamplingIntervalInMilliseconds() const {
        return minimalSamplingIntervalInMilliseconds
            ? minimalSamplingIntervalInMilliseconds
            : getMinimalSamplingIntervalInMilliseconds();
    }

    /**
     * @brief Sets a per-instance minimum sampling interval override.
     * @param intervalMs Minimum interval in milliseconds (0 clears the override).
     */
    void setMinimalSamplingIntervalInMilliseconds(uint32_t intervalMs) {
        minimalSamplingIntervalInMilliseconds = intervalMs;
    }

    /**
     * @brief Returns the GPIO pin assigned to this sensor.
     * @return GPIO pin number.
     */
    [[nodiscard]] int getPin() const { return pin; }
};

#endif // SENSOR_H