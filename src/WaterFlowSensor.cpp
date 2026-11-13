/**
 * @file WaterFlowSensor.cpp
 * @brief Implements water flow rate measurement via hybrid interrupt + periodic scheduling.
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-11-09
 * @version 1.0
 */

#include "WaterFlowSensor.h"
#include <Arduino.h>
#include <cmath>

WaterFlowSensor::WaterFlowSensor(const int signalGpioPin, EventHandler* parentHandler)
    : Sensor(signalGpioPin, parentHandler),
      pulseCount(0),
      cachedFlowRateInLitersPerMinute(INITIAL_CACHED_FLOW_RATE_IN_LITERS_PER_MINUTE),
      lastNotifiedFlowRateInLitersPerMinute(INITIAL_LAST_NOTIFIED_FLOW_RATE_IN_LITERS_PER_MINUTE),
      lastPulseCount(0)
{
    pinMode(pin, INPUT_PULLUP);
}

void WaterFlowSensor::processEvent(Event& event) {
    if (event.identifier == MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER) {
        // Read the current pulse count (volatile, but safe in sampler task context)
        const uint32_t currentPulseCount = pulseCount;

        // Calculate pulses since last sample
        const uint32_t deltaPulses = currentPulseCount - lastPulseCount;
        lastPulseCount = currentPulseCount;

        // Convert to liters per minute:
        // Flow rate (L/min) = (pulses / pulses_per_ml) / sampling_interval_s * 60
        // With 7 pulses/ml and 1 second sampling interval:
        // Flow rate = (deltaPulses / 7.0) * 60.0
        const float samplingIntervalSeconds = static_cast<float>(getDefaultSamplingIntervalInMilliseconds()) / MILLISECONDS_PER_SECOND;
        cachedFlowRateInLitersPerMinute = (static_cast<float>(deltaPulses) / PULSES_PER_MILLILITER / MILLILITERS_PER_LITER)
                                          / samplingIntervalSeconds * SECONDS_PER_MINUTE;

        if (std::fabs(cachedFlowRateInLitersPerMinute - lastNotifiedFlowRateInLitersPerMinute) > FLOW_RATE_CHANGE_EPSILON) {
            lastNotifiedFlowRateInLitersPerMinute = cachedFlowRateInLitersPerMinute;
            reportReadSuccess(event);
            event = Event(DATA_READ_EVENT_IDENTIFIER, pin);
            return;
        }

        reportReadSuccess(event);
        event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
    }
}

uint32_t WaterFlowSensor::getDefaultSamplingIntervalInMilliseconds() const {
    return DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS;
}

uint32_t WaterFlowSensor::getMinimalSamplingIntervalInMilliseconds() const {
    return MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS;
}

float WaterFlowSensor::getFlowRateInLitersPerMinute() const {
    return cachedFlowRateInLitersPerMinute;
}

void WaterFlowSensor::resetPulseCount() {
    pulseCount = 0;
    lastPulseCount = 0;
    cachedFlowRateInLitersPerMinute = INITIAL_CACHED_FLOW_RATE_IN_LITERS_PER_MINUTE;
    lastNotifiedFlowRateInLitersPerMinute = INITIAL_LAST_NOTIFIED_FLOW_RATE_IN_LITERS_PER_MINUTE;
}
