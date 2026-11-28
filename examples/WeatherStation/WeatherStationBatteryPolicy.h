#ifndef WEATHER_STATION_BATTERY_POLICY_H
#define WEATHER_STATION_BATTERY_POLICY_H

#include "PowerManager.h"

namespace WeatherStationBatteryPolicy {

enum class LatchTransition {
    None,
    LowBatteryDetected,
    BatteryRecovered
};

inline LatchTransition updateLowBatteryLatch(bool& latched,
                                            const BatteryMeasurement& measurement,
                                            const uint32_t lowThresholdInMillivolts,
                                            const uint32_t recoveryThresholdInMillivolts) {
    if (!measurement.valid || lowThresholdInMillivolts == 0 ||
        recoveryThresholdInMillivolts <= lowThresholdInMillivolts) {
        return LatchTransition::None;
    }

    if (measurement.voltageInMillivolts < lowThresholdInMillivolts) {
        const bool newlyLatched = !latched;
        latched = true;
        return newlyLatched ? LatchTransition::LowBatteryDetected : LatchTransition::None;
    }

    if (latched && measurement.voltageInMillivolts >= recoveryThresholdInMillivolts) {
        latched = false;
        return LatchTransition::BatteryRecovered;
    }

    return LatchTransition::None;
}

} // namespace WeatherStationBatteryPolicy

#endif // WEATHER_STATION_BATTERY_POLICY_H