/**
 * @file test.cpp
 * @brief Host tests for PthSensor (BME280-compatible environmental sensor).
 * @ingroup sensors_module
 */

#include "PthSensor.h"
#include "Event.h"
#include "Sensor.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
#include "../fakes/Arduino.h"
#include "../fakes/Wire.h"

class TestablePthSensor : public PthSensor {
public:
    using PthSensor::lastNotifiedTemperatureInCelsius;
    using PthSensor::lastNotifiedHumidityInPercentage;
    using PthSensor::lastNotifiedPressureInHectopascals;
    using PthSensor::cachedTemperatureInCelsius;
    using PthSensor::cachedHumidityInPercentage;
    using PthSensor::cachedPressureInHectopascals;
    using PthSensor::initialized;
    using PthSensor::TEMPERATURE_CHANGE_EPSILON;
    using PthSensor::HUMIDITY_CHANGE_EPSILON;
    using PthSensor::PRESSURE_CHANGE_EPSILON;
    using PthSensor::INITIAL_LAST_NOTIFIED_TEMPERATURE_IN_CELSIUS;
    using PthSensor::INITIAL_LAST_NOTIFIED_HUMIDITY_IN_PERCENTAGE;
    using PthSensor::INITIAL_LAST_NOTIFIED_PRESSURE_IN_HECTOPASCALS;
    using PthSensor::tFine;
    using PthSensor::digT1;
    using PthSensor::digT2;
    using PthSensor::digT3;
    using PthSensor::digP1;
    using PthSensor::digP2;
    using PthSensor::digP3;
    using PthSensor::digP4;
    using PthSensor::digP5;
    using PthSensor::digP6;
    using PthSensor::digP7;
    using PthSensor::digP8;
    using PthSensor::digP9;
    using PthSensor::digH1;
    using PthSensor::digH2;
    using PthSensor::digH3;
    using PthSensor::digH4;
    using PthSensor::digH5;
    using PthSensor::digH6;
    using PthSensor::processEvent;
    using PthSensor::getDefaultSamplingIntervalInMilliseconds;
    using PthSensor::getMinimalSamplingIntervalInMilliseconds;
    using PthSensor::compensateTemperature;
    using PthSensor::compensatePressure;
    using PthSensor::compensateHumidity;

    TestablePthSensor(int signalGpioPin, uint8_t i2cAddress = PthSensor::DEFAULT_I2C_ADDRESS,
                      EventHandler* parentHandler = nullptr)
        : PthSensor(signalGpioPin, i2cAddress, parentHandler) {}
};

void testConstructorInitializesState() {
    std::cout << "Test: Constructor initializes state correctly... ";
    Wire.readBuffer.clear();
    Wire.useReadBuffer = false;
    Wire.raw = 0;
    Wire.byteCount = 1;
    Wire.readBuffer.push_back(0x60); // chip ID
    Wire.useReadBuffer = true;
    
    TestablePthSensor sensor(23);
    
    assert(sensor.cachedTemperatureInCelsius == 0.0f);
    assert(sensor.cachedHumidityInPercentage == 0.0f);
    assert(sensor.cachedPressureInHectopascals == 0.0f);
    assert(sensor.lastNotifiedTemperatureInCelsius == -1.0f);
    assert(sensor.lastNotifiedHumidityInPercentage == -1.0f);
    assert(sensor.lastNotifiedPressureInHectopascals == -1.0f);
    assert(sensor.getPin() == 23);
    std::cout << "PASSED" << std::endl;
}

void testInitializeReadsCalibrationData() {
    std::cout << "Test: initialize() reads calibration data... ";
    Wire.readBuffer.clear();
    Wire.useReadBuffer = true;
    Wire.raw = 0;
    Wire.byteCount = 1;
    Wire.commands.clear();
    
    // Chip ID for constructor's initialize() call
    Wire.readBuffer.push_back(0x60);
    // Calibration data: 26 bytes (temp/pressure) + 7 bytes (humidity)
    for (int i = 0; i < 26; i++) {
        Wire.readBuffer.push_back(static_cast<uint8_t>(i));
    }
    for (int i = 0; i < 7; i++) {
        Wire.readBuffer.push_back(static_cast<uint8_t>(i));
    }
    
    TestablePthSensor sensor(23);
    
    assert(sensor.isInitialized() == true);
    std::cout << "PASSED" << std::endl;
}

void testInitializeFailsOnBadChipId() {
    std::cout << "Test: initialize() fails on bad chip ID... ";
    Wire.readBuffer.clear();
    Wire.useReadBuffer = true;
    Wire.raw = 0;
    Wire.byteCount = 1;
    Wire.readBuffer.push_back(0x00); // bad chip ID
    
    TestablePthSensor sensor(23);
    Wire.commands.clear();
    
    bool result = sensor.initialize();
    assert(result == false);
    assert(sensor.isInitialized() == false);
    std::cout << "PASSED" << std::endl;
}

void testProcessEventSuppressesWhenUninitialized() {
    std::cout << "Test: processEvent() suppresses when uninitialized... ";
    Wire.readBuffer.clear();
    Wire.useReadBuffer = true;
    Wire.raw = 0;
    Wire.byteCount = 1;
    Wire.readBuffer.push_back(0x00); // bad chip ID
    
    TestablePthSensor sensor(23);
    assert(sensor.isInitialized() == false);
    
    Event event(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    sensor.processEvent(event);
    
    assert(event.identifier == -1); // propagation suppression
    std::cout << "PASSED" << std::endl;
}

void testFirstReadingEmitsDataReadEvent() {
    std::cout << "Test: First reading emits DATA_READ_EVENT... ";
    Wire.readBuffer.clear();
    Wire.useReadBuffer = true;
    Wire.raw = 0;
    Wire.byteCount = 1;
    Wire.commands.clear();
    
    // Chip ID for constructor's initialize() call
    Wire.readBuffer.push_back(0x60);
    // Calibration data: 26 bytes (temp/pressure) + 7 bytes (humidity)
    for (int i = 0; i < 26; i++) {
        Wire.readBuffer.push_back(static_cast<uint8_t>(i));
    }
    for (int i = 0; i < 7; i++) {
        Wire.readBuffer.push_back(static_cast<uint8_t>(i));
    }
    // Raw data: 8 bytes (temp, pressure, humidity)
    Wire.readBuffer.push_back(0x80); Wire.readBuffer.push_back(0x00); Wire.readBuffer.push_back(0x00);
    Wire.readBuffer.push_back(0x80); Wire.readBuffer.push_back(0x00); Wire.readBuffer.push_back(0x00);
    Wire.readBuffer.push_back(0x80); Wire.readBuffer.push_back(0x00);
    
    TestablePthSensor sensor(23);
    assert(sensor.isInitialized() == true);
    
    Event event(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    sensor.processEvent(event);
    
    assert(event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER);
    assert(event.sourceId == 23);
    std::cout << "PASSED" << std::endl;
}

void testUnchangedValuesSuppressPropagation() {
    std::cout << "Test: Unchanged values suppress propagation... ";
    Wire.readBuffer.clear();
    Wire.useReadBuffer = true;
    Wire.raw = 0;
    Wire.byteCount = 1;
    Wire.commands.clear();
    
    // Chip ID for constructor's initialize() call
    Wire.readBuffer.push_back(0x60);
    // Realistic BME280 calibration data
    Wire.readBuffer.push_back(0x40); Wire.readBuffer.push_back(0x6B);
    Wire.readBuffer.push_back(0x2B); Wire.readBuffer.push_back(0x67);
    Wire.readBuffer.push_back(0xF0); Wire.readBuffer.push_back(0xFC);
    Wire.readBuffer.push_back(0x11); Wire.readBuffer.push_back(0x8E);
    Wire.readBuffer.push_back(0x52); Wire.readBuffer.push_back(0xD6);
    Wire.readBuffer.push_back(0x80); Wire.readBuffer.push_back(0x0B);
    Wire.readBuffer.push_back(0x9B); Wire.readBuffer.push_back(0xBD);
    Wire.readBuffer.push_back(0xD1); Wire.readBuffer.push_back(0xFF);
    Wire.readBuffer.push_back(0x0B); Wire.readBuffer.push_back(0x00);
    Wire.readBuffer.push_back(0x95); Wire.readBuffer.push_back(0xC6);
    Wire.readBuffer.push_back(0x09); Wire.readBuffer.push_back(0x08);
    Wire.readBuffer.push_back(0x78); Wire.readBuffer.push_back(0xEE);
    Wire.readBuffer.push_back(0x00); Wire.readBuffer.push_back(0x00); Wire.readBuffer.push_back(0x00);
    Wire.readBuffer.push_back(0x2C);
    Wire.readBuffer.push_back(0x00); Wire.readBuffer.push_back(0x13);
    Wire.readBuffer.push_back(0x00);
    Wire.readBuffer.push_back(0x00); Wire.readBuffer.push_back(0x00);
    Wire.readBuffer.push_back(0x00); Wire.readBuffer.push_back(0x00);
    Wire.readBuffer.push_back(0x00);
    
    // First reading raw data
    Wire.readBuffer.push_back(0x57); Wire.readBuffer.push_back(0x85); Wire.readBuffer.push_back(0x00);
    Wire.readBuffer.push_back(0x6A); Wire.readBuffer.push_back(0x46); Wire.readBuffer.push_back(0x40);
    Wire.readBuffer.push_back(0x5B); Wire.readBuffer.push_back(0x30);
    
    // Second reading raw data (same values, appended for second processEvent)
    Wire.readBuffer.push_back(0x57); Wire.readBuffer.push_back(0x85); Wire.readBuffer.push_back(0x00);
    Wire.readBuffer.push_back(0x6A); Wire.readBuffer.push_back(0x46); Wire.readBuffer.push_back(0x40);
    Wire.readBuffer.push_back(0x5B); Wire.readBuffer.push_back(0x30);
    
    TestablePthSensor sensor(23);
    assert(sensor.isInitialized() == true);
    
    Event event1(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    sensor.processEvent(event1);
    assert(event1.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER);
    
    float temp1 = sensor.getTemperatureInCelsius();
    float humidity1 = sensor.getHumidityInPercentage();
    float pressure1 = sensor.getPressureInHectopascals();
    
    // Second reading - reads from the appended raw data
    Event event2(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    sensor.processEvent(event2);
    
    float temp2 = sensor.getTemperatureInCelsius();
    float humidity2 = sensor.getHumidityInPercentage();
    float pressure2 = sensor.getPressureInHectopascals();
    
    assert(std::fabs(temp1 - temp2) < 0.001f);
    assert(std::fabs(humidity1 - humidity2) < 0.001f);
    assert(std::fabs(pressure1 - pressure2) < 0.001f);
    assert(event2.identifier == -1);
    std::cout << "PASSED" << std::endl;
}

void testValueChangedEmitsDataReadEvent() {
    std::cout << "Test: Value change > epsilon emits DATA_READ_EVENT... ";
    Wire.readBuffer.clear();
    Wire.useReadBuffer = true;
    Wire.raw = 0;
    Wire.byteCount = 1;
    Wire.commands.clear();
    
    // Chip ID for constructor's initialize() call
    Wire.readBuffer.push_back(0x60);
    // Calibration data: 26 bytes (temp/pressure) + 7 bytes (humidity)
    for (int i = 0; i < 26; i++) {
        Wire.readBuffer.push_back(static_cast<uint8_t>(i));
    }
    for (int i = 0; i < 7; i++) {
        Wire.readBuffer.push_back(static_cast<uint8_t>(i));
    }
    // First reading
    Wire.readBuffer.push_back(0x80); Wire.readBuffer.push_back(0x00); Wire.readBuffer.push_back(0x00);
    Wire.readBuffer.push_back(0x80); Wire.readBuffer.push_back(0x00); Wire.readBuffer.push_back(0x00);
    Wire.readBuffer.push_back(0x80); Wire.readBuffer.push_back(0x00);
    
    TestablePthSensor sensor(23);
    assert(sensor.isInitialized() == true);
    
    Event event1(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    sensor.processEvent(event1);
    assert(event1.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER);
    
    // Second reading with different temperature
    Wire.readBuffer.clear();
    Wire.readBuffer.push_back(0x90); Wire.readBuffer.push_back(0x00); Wire.readBuffer.push_back(0x00); // different temp
    Wire.readBuffer.push_back(0x80); Wire.readBuffer.push_back(0x00); Wire.readBuffer.push_back(0x00);
    Wire.readBuffer.push_back(0x80); Wire.readBuffer.push_back(0x00);
    
    Event event2(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    sensor.processEvent(event2);
    
    assert(event2.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER);
    std::cout << "PASSED" << std::endl;
}

void testGettersReturnCachedValues() {
    std::cout << "Test: Getters return cached values... ";
    Wire.readBuffer.clear();
    Wire.useReadBuffer = true;
    Wire.raw = 0;
    Wire.byteCount = 1;
    Wire.commands.clear();
    
    // Chip ID for constructor's initialize() call
    Wire.readBuffer.push_back(0x60);
    // Calibration data: 26 bytes (temp/pressure) + 7 bytes (humidity)
    for (int i = 0; i < 26; i++) {
        Wire.readBuffer.push_back(static_cast<uint8_t>(i));
    }
    for (int i = 0; i < 7; i++) {
        Wire.readBuffer.push_back(static_cast<uint8_t>(i));
    }
    // Raw data
    Wire.readBuffer.push_back(0x80); Wire.readBuffer.push_back(0x00); Wire.readBuffer.push_back(0x00);
    Wire.readBuffer.push_back(0x80); Wire.readBuffer.push_back(0x00); Wire.readBuffer.push_back(0x00);
    Wire.readBuffer.push_back(0x80); Wire.readBuffer.push_back(0x00);
    
    TestablePthSensor sensor(23);
    assert(sensor.isInitialized() == true);
    
    Event event(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    sensor.processEvent(event);
    
    // Getters should return the cached values (not zero after a successful read)
    float temp = sensor.getTemperatureInCelsius();
    float humidity = sensor.getHumidityInPercentage();
    float pressure = sensor.getPressureInHectopascals();
    
    // Values depend on calibration coefficients, but should not be zero
    assert(std::fabs(temp) > 0.0f || std::fabs(humidity) > 0.0f || std::fabs(pressure) > 0.0f);
    std::cout << "PASSED" << std::endl;
}

void testSamplingIntervalDefaults() {
    std::cout << "Test: Sampling interval defaults are correct... ";
    Wire.readBuffer.clear();
    Wire.useReadBuffer = true;
    Wire.raw = 0;
    Wire.byteCount = 1;
    Wire.commands.clear();
    
    // Chip ID for constructor's initialize() call
    Wire.readBuffer.push_back(0x60);
    // Calibration data: 26 bytes (temp/pressure) + 7 bytes (humidity)
    for (int i = 0; i < 26; i++) {
        Wire.readBuffer.push_back(static_cast<uint8_t>(i));
    }
    for (int i = 0; i < 7; i++) {
        Wire.readBuffer.push_back(static_cast<uint8_t>(i));
    }
    
    TestablePthSensor sensor(23);
    
    assert(sensor.getDefaultSamplingIntervalInMilliseconds() == 2000);
    assert(sensor.getMinimalSamplingIntervalInMilliseconds() == 250);
    std::cout << "PASSED" << std::endl;
}

void testAltitudeCalculation() {
    std::cout << "Test: getAltitudeInMeters() computes altitude from pressure... ";
    Wire.readBuffer.clear();
    Wire.useReadBuffer = true;
    Wire.raw = 0;
    Wire.byteCount = 1;
    Wire.commands.clear();
    
    // Chip ID for constructor's initialize() call
    Wire.readBuffer.push_back(0x60);
    // Realistic BME280 calibration data
    Wire.readBuffer.push_back(0x40); Wire.readBuffer.push_back(0x6B);
    Wire.readBuffer.push_back(0x2B); Wire.readBuffer.push_back(0x67);
    Wire.readBuffer.push_back(0xF0); Wire.readBuffer.push_back(0xFC);
    Wire.readBuffer.push_back(0x11); Wire.readBuffer.push_back(0x8E);
    Wire.readBuffer.push_back(0x52); Wire.readBuffer.push_back(0xD6);
    Wire.readBuffer.push_back(0x80); Wire.readBuffer.push_back(0x0B);
    Wire.readBuffer.push_back(0x9B); Wire.readBuffer.push_back(0xBD);
    Wire.readBuffer.push_back(0xD1); Wire.readBuffer.push_back(0xFF);
    Wire.readBuffer.push_back(0x0B); Wire.readBuffer.push_back(0x00);
    Wire.readBuffer.push_back(0x95); Wire.readBuffer.push_back(0xC6);
    Wire.readBuffer.push_back(0x09); Wire.readBuffer.push_back(0x08);
    Wire.readBuffer.push_back(0x78); Wire.readBuffer.push_back(0xEE);
    Wire.readBuffer.push_back(0x00); Wire.readBuffer.push_back(0x00); Wire.readBuffer.push_back(0x00);
    Wire.readBuffer.push_back(0x2C);
    Wire.readBuffer.push_back(0x00); Wire.readBuffer.push_back(0x13);
    Wire.readBuffer.push_back(0x00);
    Wire.readBuffer.push_back(0x00); Wire.readBuffer.push_back(0x00);
    Wire.readBuffer.push_back(0x00); Wire.readBuffer.push_back(0x00);
    Wire.readBuffer.push_back(0x00);
    
    // Raw data that produces ~1013.25 hPa (sea level pressure)
    Wire.readBuffer.push_back(0x57); Wire.readBuffer.push_back(0x85); Wire.readBuffer.push_back(0x00);
    Wire.readBuffer.push_back(0x6A); Wire.readBuffer.push_back(0x46); Wire.readBuffer.push_back(0x40);
    Wire.readBuffer.push_back(0x5B); Wire.readBuffer.push_back(0x30);
    
    TestablePthSensor sensor(23);
    assert(sensor.isInitialized() == true);
    
    Event event(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    sensor.processEvent(event);
    
    float pressure = sensor.getPressureInHectopascals();
    float altitude = sensor.getAltitudeInMeters(pressure);
    
    // When sea level pressure equals measured pressure, altitude should be ~0
    assert(std::fabs(altitude) < 0.01f);
    std::cout << "PASSED" << std::endl;
}

void testAltitudeWithStandardSeaLevelPressure() {
    std::cout << "Test: getAltitudeInMeters() with standard sea level pressure... ";
    Wire.readBuffer.clear();
    Wire.useReadBuffer = true;
    Wire.raw = 0;
    Wire.byteCount = 1;
    Wire.commands.clear();
    
    // Chip ID for constructor's initialize() call
    Wire.readBuffer.push_back(0x60);
    // Calibration data: 26 bytes (temp/pressure) + 7 bytes (humidity)
    for (int i = 0; i < 26; i++) {
        Wire.readBuffer.push_back(static_cast<uint8_t>(i));
    }
    for (int i = 0; i < 7; i++) {
        Wire.readBuffer.push_back(static_cast<uint8_t>(i));
    }
    // Raw data
    Wire.readBuffer.push_back(0x80); Wire.readBuffer.push_back(0x00); Wire.readBuffer.push_back(0x00);
    Wire.readBuffer.push_back(0x80); Wire.readBuffer.push_back(0x00); Wire.readBuffer.push_back(0x00);
    Wire.readBuffer.push_back(0x80); Wire.readBuffer.push_back(0x00);
    
    TestablePthSensor sensor(23);
    assert(sensor.isInitialized() == true);
    
    Event event(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    sensor.processEvent(event);
    
    float pressure = sensor.getPressureInHectopascals();
    const float standardSeaLevelPressure = 1013.25f;
    float altitude = sensor.getAltitudeInMeters(standardSeaLevelPressure);
    
    // Altitude should be positive if pressure < sea level, negative if pressure > sea level
    if (pressure < standardSeaLevelPressure) {
        assert(altitude > 0.0f);
    } else if (pressure > standardSeaLevelPressure) {
        assert(altitude < 0.0f);
    }
    std::cout << "PASSED" << std::endl;
}

void testAltitudeWithInvalidSeaLevelPressure() {
    std::cout << "Test: getAltitudeInMeters() returns 0 for invalid sea level pressure... ";
    Wire.readBuffer.clear();
    Wire.useReadBuffer = true;
    Wire.raw = 0;
    Wire.byteCount = 1;
    Wire.commands.clear();
    
    // Chip ID for constructor's initialize() call
    Wire.readBuffer.push_back(0x60);
    // Calibration data: 26 bytes (temp/pressure) + 7 bytes (humidity)
    for (int i = 0; i < 26; i++) {
        Wire.readBuffer.push_back(static_cast<uint8_t>(i));
    }
    for (int i = 0; i < 7; i++) {
        Wire.readBuffer.push_back(static_cast<uint8_t>(i));
    }
    
    TestablePthSensor sensor(23);
    
    // Before any reading, cached pressure is 0, so altitude should be 0
    assert(sensor.getAltitudeInMeters(1013.25f) == 0.0f);
    
    // Invalid sea level pressure should return 0
    assert(sensor.getAltitudeInMeters(0.0f) == 0.0f);
    assert(sensor.getAltitudeInMeters(-100.0f) == 0.0f);
    std::cout << "PASSED" << std::endl;
}

int main() {
    std::cout << "=== PthSensor Test Suite ===" << std::endl;
    
    testConstructorInitializesState();
    testInitializeReadsCalibrationData();
    testInitializeFailsOnBadChipId();
    testProcessEventSuppressesWhenUninitialized();
    testFirstReadingEmitsDataReadEvent();
    testUnchangedValuesSuppressPropagation();
    testValueChangedEmitsDataReadEvent();
    testGettersReturnCachedValues();
    testSamplingIntervalDefaults();
    testAltitudeCalculation();
    testAltitudeWithStandardSeaLevelPressure();
    testAltitudeWithInvalidSeaLevelPressure();
    
    std::cout << "=== All 12 PthSensor tests PASSED ===" << std::endl;
    return 0;
}
