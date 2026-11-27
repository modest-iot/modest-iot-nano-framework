/**
 * @file PthSensor.cpp
 * @brief Implements the BME280-compatible pressure, temperature, and humidity sensor adapter.
 * @ingroup sensors_module
 * @copyright (c) 2025-2026 Angel Velasquez
 *
 * @author Angel Velasquez
 * @date 2026-11-10
 * @version 1.0
 */

#include "PthSensor.h"
#include <cmath>
#include <cstddef>

PthSensor::PthSensor(const int signalGpioPin, const uint8_t i2cAddress, EventHandler* parentHandler)
    : BmeEnvironmentalSensor(signalGpioPin, i2cAddress, parentHandler)
{
    initialize();
}

PthSensor::PthSensor(I2cBus& bus, const int signalGpioPin, const uint8_t i2cAddress,
                     EventHandler* parentHandler)
    : BmeEnvironmentalSensor(bus, signalGpioPin, i2cAddress, parentHandler) {}

bool PthSensor::initialize() {
    initialized = false;
    uint8_t chipId = 0;
    if (!readRegister8(BME_REGISTER_CHIP_ID, chipId) || chipId != BME280_CHIP_ID_VALUE) {
        return false;
    }

    if (!readCalibrationData()) {
        return false;
    }

    // Configure sensor for normal mode with high resolution
    const uint8_t configuration[] = {
        BME_REGISTER_TEMP_PRESS_X4,
        static_cast<uint8_t>((BME_OSRS_TEMP_16 << 5) | (BME_OSRS_PRESS_16 << 2) | BME280_MODE_NORMAL),
        static_cast<uint8_t>((BME_OSRS_HUMID_16 << 4) | (BME_STANDBY_0_5_MS << 1) | BME_FILTER_COEFF_16)
    };
    if (writeBytes(configuration, sizeof(configuration)).isInvalidOrIncomplete()) {
        return false;
    }

    initialized = true;
    return true;
}

bool PthSensor::start() {
    return initialize();
}

void PthSensor::processEvent(Event& event) {
    if (event.identifier == MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER) {
        if (!initialized) {
            event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
            return;
        }

        int32_t rawTemperature, rawPressure;
        uint16_t rawHumidity;

        if (!readRawData(rawTemperature, rawPressure, rawHumidity)) {
            reportReadFailure(event);
            if (event.identifier != SENSOR_FAULT_EVENT_IDENTIFIER) {
                event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
            }
            return;
        }

        const float newTemperature = compensateTemperature(rawTemperature);
        const float newPressure = compensatePressure(rawPressure);
        const float newHumidity = compensateHumidity(rawHumidity);

        cachedTemperatureInCelsius = newTemperature;
        cachedPressureInHectopascals = newPressure;
        cachedHumidityInPercentage = newHumidity;

        const bool temperatureChanged = std::fabs(newTemperature - lastNotifiedTemperatureInCelsius) > TEMPERATURE_CHANGE_EPSILON;
        const bool pressureChanged = std::fabs(newPressure - lastNotifiedPressureInHectopascals) > PRESSURE_CHANGE_EPSILON;
        // ReSharper disable once CppTooWideScopeInitStatement
        const bool humidityChanged = std::fabs(newHumidity - lastNotifiedHumidityInPercentage) > HUMIDITY_CHANGE_EPSILON;

        if (temperatureChanged || pressureChanged || humidityChanged) {
            reportReadSuccess(event);
            if (temperatureChanged) {
                lastNotifiedTemperatureInCelsius = newTemperature;
            }
            if (pressureChanged) {
                lastNotifiedPressureInHectopascals = newPressure;
            }
            if (humidityChanged) {
                lastNotifiedHumidityInPercentage = newHumidity;
            }
            event.identifier = DATA_READ_EVENT_IDENTIFIER;
            event.sourceId = pin;
        } else {
            reportReadSuccess(event);
            event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
        }
        return;
    }
    // Non-measurement events pass through unchanged
}

uint32_t PthSensor::getDefaultSamplingIntervalInMilliseconds() const {
    return DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS;
}

uint32_t PthSensor::getMinimalSamplingIntervalInMilliseconds() const {
    return MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS;
}
