#include "LoadCellAmplifier.h"
#include <Arduino.h>
#include <cmath>

LoadCellAmplifier::LoadCellAmplifier(int dataGpioPin, int clockGpioPin, float minWeightInGrams, float maxWeightInGrams, long minRawValue, long maxRawValue, EventHandler* parentHandler)
    : Sensor(dataGpioPin, parentHandler),
      clockGpioPin(clockGpioPin),
      cachedWeightInGrams(INITIAL_CACHED_WEIGHT),
      lastNotifiedWeightInGrams(INITIAL_LAST_NOTIFIED_WEIGHT),
      minWeightInGrams(minWeightInGrams),
      maxWeightInGrams(maxWeightInGrams),
      minRawValue(minRawValue),
      maxRawValue(maxRawValue) {
    pinMode(dataGpioPin, INPUT);
    pinMode(clockGpioPin, OUTPUT);
    reset();
}

void LoadCellAmplifier::processEvent(Event& event) {
    if (event.identifier == MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER) {
        const long rawValue = readRawValue();
        if (rawValue == -1) {
            // Handle error, e.g., suppress event
            event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
            return;
        }

        // Apply moving average filter
        const long filteredRawValue = applyMovingAverageFilter(rawValue);
        cachedWeightInGrams = calculateWeight(filteredRawValue);

        if (changeDetected()) {
            event = Event(DATA_READ_EVENT_IDENTIFIER, pin);
            return;
        }

        event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
    }
}

float LoadCellAmplifier::calculateWeight(long rawValue) const {
    // Linear interpolation for weight calculation
    if (maxRawValue != minRawValue) {
        return minWeightInGrams + static_cast<float>(rawValue - minRawValue) *
                                  (maxWeightInGrams - minWeightInGrams) / 
                                  static_cast<float>(maxRawValue - minRawValue);
    }
    return INITIAL_CACHED_WEIGHT; // Handle invalid calibration
}

bool LoadCellAmplifier::changeDetected() {
    // Change detection: returns true if weight changed significantly
    if (std::fabs(cachedWeightInGrams - lastNotifiedWeightInGrams) > WEIGHT_CHANGE_EPSILON) {
        lastNotifiedWeightInGrams = cachedWeightInGrams;
        return true;
    }
    return false;
}

long LoadCellAmplifier::applyMovingAverageFilter(long rawValue) {
    rawValueBuffer[bufferIndex] = rawValue;
    bufferIndex = (bufferIndex + 1) % FILTER_SIZE;
    long sum = 0;
    for (int i = 0; i < FILTER_SIZE; ++i) {
        sum += rawValueBuffer[i];
    }
    return sum / FILTER_SIZE;
}

void LoadCellAmplifier::reset() {
    cachedWeightInGrams = INITIAL_CACHED_WEIGHT;
    lastNotifiedWeightInGrams = INITIAL_LAST_NOTIFIED_WEIGHT;
    bufferIndex = 0;
    for (int i = 0; i < FILTER_SIZE; ++i) {
        rawValueBuffer[i] = 0;
    }
}

long LoadCellAmplifier::readRawValue() const {
    // Wait for the HX711 to become ready (data line goes low).
    const unsigned long startTime = millis();
    while (digitalRead(pin) != LOW) {
        if (millis() - startTime > READ_TIMEOUT_MS) {
            return -1; // Timeout
        }
    }

    long value = 0;
    for (int i = 0; i < HX711_BIT_COUNT; ++i) {
        digitalWrite(clockGpioPin, HIGH);
        delayMicroseconds(HX711_CLOCK_PULSE_DELAY_US);
        value <<= 1;
        digitalWrite(clockGpioPin, LOW);
        if (digitalRead(pin) == HIGH) {
            value++;
        }
        delayMicroseconds(HX711_CLOCK_PULSE_DELAY_US);
    }

    // HX711 pulse for gain/channel selection (here: gain 128)
    digitalWrite(clockGpioPin, HIGH);
    delayMicroseconds(HX711_CLOCK_PULSE_DELAY_US);
    digitalWrite(clockGpioPin, LOW);

    // Sign extension for 24-bit to 32-bit (if negative)
    if (value & HX711_SIGN_BIT_MASK) {
        value |= HX711_SIGN_EXTENSION_MASK;
    }
    
    return value;
}

float LoadCellAmplifier::getWeightInGrams() const {
    return cachedWeightInGrams;
}

float LoadCellAmplifier::getMinWeightInGrams() const {
    return minWeightInGrams;
}

float LoadCellAmplifier::getMaxWeightInGrams() const {
    return maxWeightInGrams;
}
