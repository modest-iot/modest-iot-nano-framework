#include "LoadCellAmplifier.h"
#include <Arduino.h>
#include <cmath>

LoadCellAmplifier::LoadCellAmplifier(int dataGpioPin, int clockGpioPin, float minWeightInGrams, float maxWeightInGrams, long minRawValue, long maxRawValue, int filterDepth, EventHandler* parentHandler)
    : Sensor(dataGpioPin, parentHandler),
      clockGpioPin(clockGpioPin),
      cachedWeightInGrams(INITIAL_CACHED_WEIGHT),
      lastNotifiedWeightInGrams(INITIAL_LAST_NOTIFIED_WEIGHT),
      minWeightInGrams(minWeightInGrams),
      maxWeightInGrams(maxWeightInGrams),
      minRawValue(minRawValue),
      maxRawValue(maxRawValue),
      tareOffsetRaw(0),
      gainPulses(1),
      filterDepth((filterDepth > MAX_FILTER_SIZE) ? MAX_FILTER_SIZE : (filterDepth < 1 ? 1 : filterDepth)) {
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
    bufferIndex = (bufferIndex + 1) % filterDepth;
    long sum = 0;
    for (int i = 0; i < filterDepth; ++i) {
        sum += rawValueBuffer[i];
    }
    return sum / filterDepth;
}

void LoadCellAmplifier::reset() {
    cachedWeightInGrams = INITIAL_CACHED_WEIGHT;
    lastNotifiedWeightInGrams = INITIAL_LAST_NOTIFIED_WEIGHT;
    bufferIndex = 0;
    for (int i = 0; i < filterDepth; ++i) {
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

    // HX711 pulse for gain/channel selection
    for (int i = 0; i < gainPulses; ++i) {
        digitalWrite(clockGpioPin, HIGH);
        delayMicroseconds(HX711_CLOCK_PULSE_DELAY_US);
        digitalWrite(clockGpioPin, LOW);
        delayMicroseconds(HX711_CLOCK_PULSE_DELAY_US);
    }

    // Sign extension for 24-bit to 32-bit (if negative)
    if (value & HX711_SIGN_BIT_MASK) {
        value |= HX711_SIGN_EXTENSION_MASK;
    }
    
    return value - tareOffsetRaw;
}

void LoadCellAmplifier::setGain(int gain) {
    if (gain == 128) gainPulses = 1;
    else if (gain == 32) gainPulses = 2;
    reset();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void LoadCellAmplifier::powerDown()  {
    digitalWrite(clockGpioPin, HIGH);
    delayMicroseconds(100); // Datasheet says > 60us
}

// ReSharper disable once CppMemberFunctionMayBeConst
void LoadCellAmplifier::powerUp() {
    digitalWrite(clockGpioPin, LOW);
}

void LoadCellAmplifier::tare() {
    // Current raw value (already tared by offset)
    // To set current weight to 0, I need to add current reading as offset
    long currentRaw = readRawValue() + tareOffsetRaw;
    if (currentRaw != -1) {
        tareOffsetRaw = currentRaw;
        reset();
    }
}

bool LoadCellAmplifier::calibrate(float knownWeightInGrams) {
    if (!isValidWeight(knownWeightInGrams)) {
        return false;
    }

    int successfulSamples = 0;
    const long averageRaw = calculateAverageRawValue(successfulSamples);
    if (successfulSamples == 0) {
        return false;
    }

    if (!isCalibrationStable(averageRaw)) {
        return false;
    }

    applyCalibration(knownWeightInGrams, averageRaw);
    reset();
    return true;
}

bool LoadCellAmplifier::isValidWeight(float weightInGrams) {
    return weightInGrams > 0;
}

long LoadCellAmplifier::calculateAverageRawValue(int& successfulSamples) const {
    long totalRaw = 0;
    successfulSamples = 0;
    for (int i = 0; i < CALIBRATION_SAMPLES; ++i) {
        const long currentRaw = readRawValue() + tareOffsetRaw;
        if (currentRaw != -1) {
            totalRaw += currentRaw;
            successfulSamples++;
        }
    }

    if (successfulSamples == 0) {
        return 0;
    }
    return totalRaw / successfulSamples;
}

bool LoadCellAmplifier::isCalibrationStable(long averageRawValue) const {
    return std::abs(averageRawValue - tareOffsetRaw) > CALIBRATION_STABILITY_THRESHOLD;
}

void LoadCellAmplifier::applyCalibration(float knownWeightInGrams, long rawValue) {
    maxRawValue = rawValue;
    maxWeightInGrams = knownWeightInGrams;
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
