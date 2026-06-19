#ifndef LOAD_CELL_CONTROLLER_H
#define LOAD_CELL_CONTROLLER_H

#include "Sensor.h"
#include <HX711.h>

/**
 * @brief Alternative sensor adapter for HX711-based load cell amplifiers using the standard HX711 library.
 * @ingroup sensors_module
 */
class LoadCellController : public Sensor {
private:
    HX711 hx711;
    float cachedWeightInGrams;
    float lastNotifiedWeightInGrams;
    float minWeightInGrams;
    float maxWeightInGrams;
    long minRawValue;
    long maxRawValue;
    int filterDepth;

    static constexpr int MAX_FILTER_SIZE = 10;
    static constexpr float WEIGHT_CHANGE_EPSILON = 0.1f;
    static constexpr float INITIAL_CACHED_WEIGHT = 0.0f;
    static constexpr float INITIAL_LAST_NOTIFIED_WEIGHT = -1.0f;
    static constexpr int PROPAGATION_SUPPRESSION_IDENTIFIER = -1; ///< Identifier for propagation suppression.
    [[nodiscard]] float calculateWeight(long rawValue) const;
    bool changeDetected();

protected:
    void processEvent(Event& event) override;

public:
    LoadCellController(int dataGpioPin, int clockGpioPin, float minWeightInGrams, float maxWeightInGrams, long minRawValue, long maxRawValue, int filterDepth, EventHandler* parentHandler = nullptr);

    [[nodiscard]] float getWeightInGrams() const;
    [[nodiscard]] float getMinWeightInGrams() const;
    [[nodiscard]] float getMaxWeightInGrams() const;

    void tare();
    static bool calibrate(float knownWeightInGrams);
    void powerDown();
    void powerUp();
    void setGain(int gain);
};

#endif // LOAD_CELL_CONTROLLER_H
