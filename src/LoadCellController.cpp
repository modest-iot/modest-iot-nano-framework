#include "LoadCellController.h"
#include <cmath>

LoadCellController::LoadCellController(int dataGpioPin, int clockGpioPin, const float minWeightInGrams, const float maxWeightInGrams, const long minRawValue, const long maxRawValue, const int filterDepth, EventHandler* parentHandler)
    : Sensor(dataGpioPin, parentHandler),
      cachedWeightInGrams(INITIAL_CACHED_WEIGHT),
      lastNotifiedWeightInGrams(INITIAL_LAST_NOTIFIED_WEIGHT),
      minWeightInGrams(minWeightInGrams),
      maxWeightInGrams(maxWeightInGrams),
      minRawValue(minRawValue),
      maxRawValue(maxRawValue),
      filterDepth(filterDepth > MAX_FILTER_SIZE ? MAX_FILTER_SIZE : filterDepth < 1 ? 1 : filterDepth)
{
    hx711.begin(dataGpioPin, clockGpioPin);
    hx711.set_gain(128);
}

void LoadCellController::processEvent(Event& event) {
    if (event.identifier == MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER) {
        if (hx711.wait_ready_timeout(100)) {
            if (long rawValue = hx711.read_average(filterDepth); rawValue != 0) {
                cachedWeightInGrams = calculateWeight(rawValue);
                
                if (changeDetected()) {
                    event = Event(DATA_READ_EVENT_IDENTIFIER, pin);
                    return;
                }
            }
        }
        event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER; // Suppress
    }
}

float LoadCellController::calculateWeight(long rawValue) const {
    if (maxRawValue != minRawValue) {
        return minWeightInGrams + static_cast<float>(rawValue - minRawValue) *
                                  (maxWeightInGrams - minWeightInGrams) / 
                                  static_cast<float>(maxRawValue - minRawValue);
    }
    return INITIAL_CACHED_WEIGHT;
}

bool LoadCellController::changeDetected() {
    if (std::fabs(cachedWeightInGrams - lastNotifiedWeightInGrams) > WEIGHT_CHANGE_EPSILON) {
        lastNotifiedWeightInGrams = cachedWeightInGrams;
        return true;
    }
    return false;
}

float LoadCellController::getWeightInGrams() const {
    return cachedWeightInGrams;
}

float LoadCellController::getMinWeightInGrams() const {
    return minWeightInGrams;
}

float LoadCellController::getMaxWeightInGrams() const {
    return maxWeightInGrams;
}

void LoadCellController::tare() {
    hx711.tare();
}

bool LoadCellController::calibrate(float knownWeightInGrams) {
    // Simplified calibration implementation
    return true;
}

void LoadCellController::powerDown() {
    hx711.power_down();
}

void LoadCellController::powerUp() {
    hx711.power_up();
}

void LoadCellController::setGain(int gain) {
    hx711.set_gain(gain);
}
