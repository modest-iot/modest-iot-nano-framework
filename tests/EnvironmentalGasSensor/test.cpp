/**
 * @file test.cpp
 * @brief Host tests for EnvironmentalGasSensor (BME680).
 *
 * Tests cover constructor, calibration data, all four compensation formulas,
 * change detection, and event emission.
 */

#include "EnvironmentalGasSensor.h"
#include "Event.h"
#include "Sensor.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
#include "../fakes/Arduino.h"
#include "../fakes/Wire.h"

class TestableEnvironmentalGasSensor : public EnvironmentalGasSensor {
public:
    using EnvironmentalGasSensor::cachedTemperatureInCelsius;
    using EnvironmentalGasSensor::cachedHumidityInPercentage;
    using EnvironmentalGasSensor::cachedPressureInHectopascals;
    using EnvironmentalGasSensor::cachedGasResistanceInKOhms;
    using EnvironmentalGasSensor::lastNotifiedTemperatureInCelsius;
    using EnvironmentalGasSensor::lastNotifiedHumidityInPercentage;
    using EnvironmentalGasSensor::lastNotifiedPressureInHectopascals;
    using EnvironmentalGasSensor::lastNotifiedGasResistanceInKOhms;
    using EnvironmentalGasSensor::initialized;
    using EnvironmentalGasSensor::tFine;
    using EnvironmentalGasSensor::digT1;
    using EnvironmentalGasSensor::digT2;
    using EnvironmentalGasSensor::digT3;
    using EnvironmentalGasSensor::digP1;
    using EnvironmentalGasSensor::digP2;
    using EnvironmentalGasSensor::digP3;
    using EnvironmentalGasSensor::digP4;
    using EnvironmentalGasSensor::digP5;
    using EnvironmentalGasSensor::digP6;
    using EnvironmentalGasSensor::digP7;
    using EnvironmentalGasSensor::digP8;
    using EnvironmentalGasSensor::digP9;
    using EnvironmentalGasSensor::digH1;
    using EnvironmentalGasSensor::digH2;
    using EnvironmentalGasSensor::digH3;
    using EnvironmentalGasSensor::digH4;
    using EnvironmentalGasSensor::digH5;
    using EnvironmentalGasSensor::digH6;
    using EnvironmentalGasSensor::digG1;
    using EnvironmentalGasSensor::digG2;
    using EnvironmentalGasSensor::digG3;

    using EnvironmentalGasSensor::TEMPERATURE_CHANGE_EPSILON;
    using EnvironmentalGasSensor::HUMIDITY_CHANGE_EPSILON;
    using EnvironmentalGasSensor::PRESSURE_CHANGE_EPSILON;
    using EnvironmentalGasSensor::GAS_RESISTANCE_CHANGE_EPSILON;
    using EnvironmentalGasSensor::INITIAL_CACHED_TEMPERATURE_IN_CELSIUS;
    using EnvironmentalGasSensor::INITIAL_CACHED_HUMIDITY_IN_PERCENTAGE;
    using EnvironmentalGasSensor::INITIAL_CACHED_PRESSURE_IN_HECTOPASCALS;
    using EnvironmentalGasSensor::INITIAL_CACHED_GAS_RESISTANCE_IN_KOHMS;
    using EnvironmentalGasSensor::INITIAL_LAST_NOTIFIED_TEMPERATURE_IN_CELSIUS;
    using EnvironmentalGasSensor::INITIAL_LAST_NOTIFIED_HUMIDITY_IN_PERCENTAGE;
    using EnvironmentalGasSensor::INITIAL_LAST_NOTIFIED_PRESSURE_IN_HECTOPASCALS;
    using EnvironmentalGasSensor::INITIAL_LAST_NOTIFIED_GAS_RESISTANCE_IN_KOHMS;
    using EnvironmentalGasSensor::processEvent;
    using EnvironmentalGasSensor::getDefaultSamplingIntervalInMilliseconds;
    using EnvironmentalGasSensor::getMinimalSamplingIntervalInMilliseconds;
    using EnvironmentalGasSensor::compensateTemperature;
    using EnvironmentalGasSensor::compensatePressure;
    using EnvironmentalGasSensor::compensateHumidity;
    using EnvironmentalGasSensor::compensateGasResistance;

    TestableEnvironmentalGasSensor(int signalGpioPin, uint8_t i2cAddress = 
        EnvironmentalGasSensor::DEFAULT_I2C_ADDRESS, EventHandler* parentHandler = nullptr)
        : EnvironmentalGasSensor(signalGpioPin, i2cAddress, parentHandler) {}
};

void testConstructorInitializesState() {
    std::cout << "Test: Constructor initializes state correctly... ";
    Wire.readBuffer.clear();
    Wire.useReadBuffer = false;
    Wire.raw = 0;
    Wire.byteCount = 1;
    Wire.readBuffer.push_back(0x61); // BME680 chip ID
    Wire.useReadBuffer = true;
    
    // Calibration data: 26 bytes (temp/pressure) + 7 bytes (humidity) + 3 bytes (gas)
    for (int i = 0; i < 36; i++) {
        Wire.readBuffer.push_back(static_cast<uint8_t>(i));
    }
    
    TestableEnvironmentalGasSensor sensor(4);
    
    assert(sensor.cachedTemperatureInCelsius == sensor.INITIAL_CACHED_TEMPERATURE_IN_CELSIUS);
    assert(sensor.cachedHumidityInPercentage == sensor.INITIAL_CACHED_HUMIDITY_IN_PERCENTAGE);
    assert(sensor.cachedPressureInHectopascals == sensor.INITIAL_CACHED_PRESSURE_IN_HECTOPASCALS);
    assert(sensor.cachedGasResistanceInKOhms == sensor.INITIAL_CACHED_GAS_RESISTANCE_IN_KOHMS);
    assert(sensor.lastNotifiedTemperatureInCelsius == sensor.INITIAL_LAST_NOTIFIED_TEMPERATURE_IN_CELSIUS);
    assert(sensor.lastNotifiedHumidityInPercentage == sensor.INITIAL_LAST_NOTIFIED_HUMIDITY_IN_PERCENTAGE);
    assert(sensor.lastNotifiedPressureInHectopascals == sensor.INITIAL_LAST_NOTIFIED_PRESSURE_IN_HECTOPASCALS);
    assert(sensor.lastNotifiedGasResistanceInKOhms == sensor.INITIAL_LAST_NOTIFIED_GAS_RESISTANCE_IN_KOHMS);
    std::cout << "PASSED" << std::endl;
}

void testInitializeFailsOnBadChipId() {
    std::cout << "Test: initialize() fails on bad chip ID... ";
    Wire.readBuffer.clear();
    Wire.useReadBuffer = true;
    Wire.raw = 0;
    Wire.byteCount = 1;
    Wire.readBuffer.push_back(0x00); // bad chip ID
    
    TestableEnvironmentalGasSensor sensor(4);
    
    assert(sensor.isInitialized() == false);
    std::cout << "PASSED" << std::endl;
}

void testSamplingIntervals() {
    std::cout << "Test: Sampling intervals are correct... ";
    Wire.readBuffer.clear();
    Wire.useReadBuffer = true;
    Wire.raw = 0;
    Wire.byteCount = 1;
    Wire.readBuffer.push_back(0x00); // bad chip ID
    
    TestableEnvironmentalGasSensor sensor(4);
    
    assert(sensor.getDefaultSamplingIntervalInMilliseconds() == 2000);
    assert(sensor.getMinimalSamplingIntervalInMilliseconds() == 500);
    std::cout << "PASSED" << std::endl;
}

void testTemperatureCompensation() {
    std::cout << "Test: Temperature compensation produces valid result... ";
    Wire.readBuffer.clear();
    Wire.useReadBuffer = true;
    Wire.raw = 0;
    Wire.byteCount = 1;
    Wire.readBuffer.push_back(0x00); // bad chip ID
    
    TestableEnvironmentalGasSensor sensor(4);
    
    // Realistic BME680 calibration data from Bosch datasheet
    sensor.digT1 = 51284;
    sensor.digT2 = 6518;
    sensor.digT3 = -4;
    
    // Raw temperature for ~25°C
    float temp = sensor.compensateTemperature(51284);
    
    // Verify tFine was updated and result is finite
    assert(sensor.tFine != 0);
    assert(std::isfinite(temp));
    std::cout << "PASSED" << std::endl;
}

void testPressureCompensation() {
    std::cout << "Test: Pressure compensation produces valid result... ";
    Wire.readBuffer.clear();
    Wire.useReadBuffer = true;
    Wire.raw = 0;
    Wire.byteCount = 1;
    Wire.readBuffer.push_back(0x00); // bad chip ID
    
    TestableEnvironmentalGasSensor sensor(4);
    
    // Realistic BME680 calibration data
    sensor.digT1 = 51284;
    sensor.digT2 = 6518;
    sensor.digT3 = -4;
    sensor.digP1 = 36401;
    sensor.digP2 = -10686;
    sensor.digP3 = -12;
    sensor.digP4 = 11000;
    sensor.digP5 = -57;
    sensor.digP6 = -19;
    sensor.digP7 = 17000;
    sensor.digP8 = -6;
    sensor.digP9 = 2700;
    
    sensor.compensateTemperature(51284);
    float pressure = sensor.compensatePressure(823142);
    
    assert(std::isfinite(pressure));
    std::cout << "PASSED" << std::endl;
}

void testHumidityCompensation() {
    std::cout << "Test: Humidity compensation produces valid result... ";
    Wire.readBuffer.clear();
    Wire.useReadBuffer = true;
    Wire.raw = 0;
    Wire.byteCount = 1;
    Wire.readBuffer.push_back(0x00); // bad chip ID
    
    TestableEnvironmentalGasSensor sensor(4);
    
    // Realistic BME680 calibration data
    sensor.digT1 = 51284;
    sensor.digT2 = 6518;
    sensor.digT3 = -4;
    sensor.digH1 = 41;
    sensor.digH2 = 6726;
    sensor.digH3 = 0;
    sensor.digH4 = 2668;
    sensor.digH5 = 256;
    sensor.digH6 = -44;
    
    sensor.compensateTemperature(51284);
    float humidity = sensor.compensateHumidity(25000);
    
    assert(humidity >= 0.0f && humidity <= 100.0f);
    std::cout << "PASSED" << std::endl;
}

void testGasResistanceCompensation() {
    std::cout << "Test: Gas resistance compensation produces valid result... ";
    Wire.readBuffer.clear();
    Wire.useReadBuffer = true;
    Wire.raw = 0;
    Wire.byteCount = 1;
    Wire.readBuffer.push_back(0x00); // bad chip ID
    
    TestableEnvironmentalGasSensor sensor(4);
    
    sensor.digG1 = -2;
    sensor.digG2 = 200;
    sensor.digG3 = -50;
    
    float gasResistance = sensor.compensateGasResistance(200, 0);
    
    assert(gasResistance >= 0.0f);
    std::cout << "PASSED" << std::endl;
}

void testProcessEventSuppressesWhenUninitialized() {
    std::cout << "Test: processEvent() suppresses when uninitialized... ";
    Wire.readBuffer.clear();
    Wire.useReadBuffer = true;
    Wire.raw = 0;
    Wire.byteCount = 1;
    Wire.readBuffer.push_back(0x00); // bad chip ID
    
    TestableEnvironmentalGasSensor sensor(4);
    
    Event event(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, 4);
    sensor.processEvent(event);
    
    assert(event.identifier == -1);
    std::cout << "PASSED" << std::endl;
}

void testGettersReturnCachedValues() {
    std::cout << "Test: Getters return cached values correctly... ";
    Wire.readBuffer.clear();
    Wire.useReadBuffer = true;
    Wire.raw = 0;
    Wire.byteCount = 1;
    Wire.readBuffer.push_back(0x00); // bad chip ID
    
    TestableEnvironmentalGasSensor sensor(4);
    
    assert(sensor.getTemperatureInCelsius() == 0.0f);
    assert(sensor.getHumidityInPercentage() == 0.0f);
    assert(sensor.getPressureInHectopascals() == 0.0f);
    assert(sensor.getGasResistanceInKOhms() == 0.0f);
    
    sensor.cachedTemperatureInCelsius = 25.5f;
    sensor.cachedHumidityInPercentage = 65.0f;
    sensor.cachedPressureInHectopascals = 1013.25f;
    sensor.cachedGasResistanceInKOhms = 100.0f;
    
    assert(sensor.getTemperatureInCelsius() == 25.5f);
    assert(sensor.getHumidityInPercentage() == 65.0f);
    assert(sensor.getPressureInHectopascals() == 1013.25f);
    assert(sensor.getGasResistanceInKOhms() == 100.0f);
    std::cout << "PASSED" << std::endl;
}

void testFirstReadingEmitsDataReadEvent() {
    std::cout << "Test: First reading emits DATA_READ_EVENT... ";
    Wire.readBuffer.clear();
    Wire.useReadBuffer = true;
    Wire.raw = 0;
    Wire.byteCount = 1;
    Wire.readBuffer.push_back(0x61); // BME680 chip ID
    for (int i = 0; i < 36; i++) {
        Wire.readBuffer.push_back(static_cast<uint8_t>(i));
    }
    
    TestableEnvironmentalGasSensor sensor(4);
    assert(sensor.isInitialized() == true);
    
    // Set calibration data for deterministic results
    sensor.digT1 = 51284;
    sensor.digT2 = 6518;
    sensor.digT3 = -4;
    sensor.digP1 = 36401;
    sensor.digP2 = -10686;
    sensor.digP3 = -12;
    sensor.digP4 = 11000;
    sensor.digP5 = -57;
    sensor.digP6 = -19;
    sensor.digP7 = 17000;
    sensor.digP8 = -6;
    sensor.digP9 = 2700;
    sensor.digH1 = 41;
    sensor.digH2 = 6726;
    sensor.digH3 = 0;
    sensor.digH4 = 2668;
    sensor.digH5 = 256;
    sensor.digH6 = -44;
    sensor.digG1 = -2;
    sensor.digG2 = 200;
    sensor.digG3 = -50;
    
    // Set up raw data: 8 bytes (temp/press/hum) + 3 bytes (gas)
    Wire.readBuffer.clear();
    Wire.readBuffer.push_back(0x6C); Wire.readBuffer.push_back(0x3C); Wire.readBuffer.push_back(0x00); // temp
    Wire.readBuffer.push_back(0x6C); Wire.readBuffer.push_back(0x3C); Wire.readBuffer.push_back(0x00); // press
    Wire.readBuffer.push_back(0x6C); Wire.readBuffer.push_back(0x00); // humidity
    Wire.readBuffer.push_back(0x00); Wire.readBuffer.push_back(0x00); Wire.readBuffer.push_back(0x00); // gas
    
    Event event(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, 4);
    sensor.processEvent(event);
    
    assert(event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER);
    std::cout << "PASSED" << std::endl;
}

void testUnchangedValuesSuppressPropagation() {
    std::cout << "Test: Unchanged values suppress propagation... ";
    Wire.readBuffer.clear();
    Wire.useReadBuffer = true;
    Wire.raw = 0;
    Wire.byteCount = 1;
    Wire.readBuffer.push_back(0x61); // BME680 chip ID
    for (int i = 0; i < 36; i++) {
        Wire.readBuffer.push_back(static_cast<uint8_t>(i));
    }
    
    TestableEnvironmentalGasSensor sensor(4);
    assert(sensor.isInitialized() == true);
    
    // Set calibration data
    sensor.digT1 = 51284;
    sensor.digT2 = 6518;
    sensor.digT3 = -4;
    sensor.digP1 = 36401;
    sensor.digP2 = -10686;
    sensor.digP3 = -12;
    sensor.digP4 = 11000;
    sensor.digP5 = -57;
    sensor.digP6 = -19;
    sensor.digP7 = 17000;
    sensor.digP8 = -6;
    sensor.digP9 = 2700;
    sensor.digH1 = 41;
    sensor.digH2 = 6726;
    sensor.digH3 = 0;
    sensor.digH4 = 2668;
    sensor.digH5 = 256;
    sensor.digH6 = -44;
    sensor.digG1 = -2;
    sensor.digG2 = 200;
    sensor.digG3 = -50;
    
    // First reading
    Wire.readBuffer.clear();
    Wire.readBuffer.push_back(0x6C); Wire.readBuffer.push_back(0x3C); Wire.readBuffer.push_back(0x00);
    Wire.readBuffer.push_back(0x6C); Wire.readBuffer.push_back(0x3C); Wire.readBuffer.push_back(0x00);
    Wire.readBuffer.push_back(0x6C); Wire.readBuffer.push_back(0x00);
    Wire.readBuffer.push_back(0x00); Wire.readBuffer.push_back(0x00); Wire.readBuffer.push_back(0x00);
    
    Event event1(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, 4);
    sensor.processEvent(event1);
    
    // Second reading with same data
    Wire.readBuffer.clear();
    Wire.readBuffer.push_back(0x6C); Wire.readBuffer.push_back(0x3C); Wire.readBuffer.push_back(0x00);
    Wire.readBuffer.push_back(0x6C); Wire.readBuffer.push_back(0x3C); Wire.readBuffer.push_back(0x00);
    Wire.readBuffer.push_back(0x6C); Wire.readBuffer.push_back(0x00);
    Wire.readBuffer.push_back(0x00); Wire.readBuffer.push_back(0x00); Wire.readBuffer.push_back(0x00);
    
    Event event2(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, 4);
    sensor.processEvent(event2);
    
    assert(event2.identifier == -1);
    std::cout << "PASSED" << std::endl;
}

int main() {
    std::cout << "Running EnvironmentalGasSensor tests..." << std::endl;
    
    testConstructorInitializesState();
    testInitializeFailsOnBadChipId();
    testSamplingIntervals();
    testTemperatureCompensation();
    testPressureCompensation();
    testHumidityCompensation();
    testGasResistanceCompensation();
    testProcessEventSuppressesWhenUninitialized();
    testGettersReturnCachedValues();
    testFirstReadingEmitsDataReadEvent();
    testUnchangedValuesSuppressPropagation();
    
    std::cout << "All EnvironmentalGasSensor tests passed!" << std::endl;
    return 0;
}
