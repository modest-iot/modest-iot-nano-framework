#ifndef PULSE_SENSOR_H
#define PULSE_SENSOR_H

#include "Sensor.h"

class PulseSensor : public Sensor {
private:
    int cachedBeatsPerMinute; 
    int sampleSignalValue;
    unsigned long lastBeatTimestamp;
    
    bool searchForHeartbeat;
    int runningSignalPeak;
    int runningSignalTrough;
    int adaptiveThresholdValue;

    // --- SCOPED EXPLICIT CONSTANTS (ELIMINATING MAGIC NUMBERS) ---
    static const int DEFAULT_STABILIZATION_PEAK_BASELINE = 512;
    static const int HEARTBEAT_IGNITION_MINIMUM_AMPLITUDE = 100;
    static const unsigned long DEBOUNCE_HEARTBEAT_WINDOW_IN_MILLISECONDS = 300;
    static const int PROPAGATION_SUPPRESSION_IDENTIFIER = -1;
    
    // Mathematical normalization scaling figures
    static const int MILLISECONDS_PER_MINUTE_COEFFICIENT = 60000;
    static const int MINIMUM_PHYSIOLOGICAL_HEART_RATE_BPM = 30;
    static const int MAXIMUM_PHYSIOLOGICAL_HEART_RATE_BPM = 200;
    static const int DOUBLE_DEBOUNCE_MULTIPLIER = 2;

protected:
    /**
     * @brief Intercepts the unified base-class event identifier to perform ADC sampling.
     * Mutates the event to Sensor::DATA_READ_EVENT only when a valid new heartbeat cycle is captured.
     */
    void processEvent(Event& event) override;

public:
    PulseSensor(int analogInputGpioPin, EventHandler* parentHandler = nullptr);

    int getBeatsPerMinute() const;
    int getRawSignalValue() const;
};

#endif // PULSE_SENSOR_H