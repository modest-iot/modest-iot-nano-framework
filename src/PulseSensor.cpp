/**
 * @file PulseSensor.cpp
 * @brief Implements adaptive-threshold heartbeat detection and BPM estimation.
 * @date 2026-06-06
 * @version 1.0
 */

#include "PulseSensor.h"
#include <Arduino.h>

PulseSensor::PulseSensor(int analogInputGpioPin, EventHandler* parentHandler)
    : Sensor(analogInputGpioPin, parentHandler),
      cachedBeatsPerMinute(0),
      sampleSignalValue(0),
      lastBeatTimestamp(0),
      searchForHeartbeat(false),
      runningSignalPeak(DEFAULT_STABILIZATION_PEAK_BASELINE),
      runningSignalTrough(DEFAULT_STABILIZATION_PEAK_BASELINE),
      adaptiveThresholdValue(DEFAULT_STABILIZATION_PEAK_BASELINE) 
{
    pinMode(pin, INPUT); // Configure assigned GPIO target for analog ADC conversion reads
}

void PulseSensor::processEvent(Event& event) {
    // Intercept the global standard framework polling request code
    if (event.identifier == MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER) {
        unsigned long currentTimestamp = millis();
        sampleSignalValue = analogRead(pin);
        
        // Track the real-time trajectory of analog waves to establish local peak and trough limits
        if (sampleSignalValue < adaptiveThresholdValue && 
           (currentTimestamp - lastBeatTimestamp) > (DEBOUNCE_HEARTBEAT_WINDOW_IN_MILLISECONDS * DOUBLE_DEBOUNCE_MULTIPLIER)) 
        {
            if (sampleSignalValue < runningSignalTrough) {
                runningSignalTrough = sampleSignalValue;
            }
        }
        if (sampleSignalValue > adaptiveThresholdValue && sampleSignalValue > runningSignalPeak) {
            runningSignalPeak = sampleSignalValue;
        }

        // Evaluate if a heartbeat spike occurred
        int totalSignalAmplitude = runningSignalPeak - runningSignalTrough;
        if ((currentTimestamp - lastBeatTimestamp) > DEBOUNCE_HEARTBEAT_WINDOW_IN_MILLISECONDS) {
            if ((sampleSignalValue > adaptiveThresholdValue) && !searchForHeartbeat && (totalSignalAmplitude > HEARTBEAT_IGNITION_MINIMUM_AMPLITUDE)) {
                
                searchForHeartbeat = true;
                unsigned long localizedTimeDeltaInMilliseconds = currentTimestamp - lastBeatTimestamp;
                lastBeatTimestamp = currentTimestamp;

                // Derive Beats Per Minute calculation based on millisecond window durations
                int calculatedBeatsPerMinute = static_cast<int>(MILLISECONDS_PER_MINUTE_COEFFICIENT / localizedTimeDeltaInMilliseconds);
                
                // Enforce safety boundary checks via physiological threshold constants
                if (calculatedBeatsPerMinute > MINIMUM_PHYSIOLOGICAL_HEART_RATE_BPM && 
                    calculatedBeatsPerMinute < MAXIMUM_PHYSIOLOGICAL_HEART_RATE_BPM) 
                {
                    if (calculatedBeatsPerMinute != cachedBeatsPerMinute) {
                        cachedBeatsPerMinute = calculatedBeatsPerMinute;
                        
                        // MUTATION: Transition the request into the framework's standard read notification
                        event = DATA_READ_EVENT;
                        return;
                    }
                }
            }
        }

        // Reset tracking flags as the signal waveform declines
        if (sampleSignalValue < adaptiveThresholdValue && searchForHeartbeat) {
            searchForHeartbeat = false;
            runningSignalPeak = adaptiveThresholdValue = sampleSignalValue;
            runningSignalTrough = sampleSignalValue;
        }

        // CONDITIONAL PERFORMANCE GUARD:
        // Set the identifier to -1 if no heartbeat transaction occurred during this tick.
        // This stops thread propagation immediately, keeping the Device mediator task sound asleep.
        event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
    }
}

int PulseSensor::getBeatsPerMinute() const { 
    return cachedBeatsPerMinute; 
}

int PulseSensor::getRawSignalValue() const { 
    return sampleSignalValue; 
}