#include <time.h>
/**
 * @file test.cpp
 * @brief Host tests for DhtSensor (native single-wire protocol).
 * @date 2026-11-10
 * @version 1.0
 */

#include "DhtSensor.h"
#include <cassert>
#include <cstdio>

// Test helper: captures events from the sensor
class EventCapture : public EventHandler {
public:
    Event lastEvent{-1};
    void on(Event event) override { lastEvent = event; }
};

// Testable subclass exposes protected methods for testing
class TestableDhtSensor : public DhtSensor {
public:
    using DhtSensor::DhtSensor;
    uint32_t getSamplingIntervalDefault() const { return getDefaultSamplingIntervalInMilliseconds(); }
    uint32_t getSamplingIntervalMinimal() const { return getMinimalSamplingIntervalInMilliseconds(); }
};

void testConstructorInitializesState() {
    EventCapture capture;
    DhtSensor sensor(2, DhtSensor::SENSOR_MODEL_DHT22, &capture);
    assert(sensor.getTemperatureInCelsius() == -999.0f);
    assert(sensor.getRelativeHumidityInPercentage() == -999.0f);
    printf("  Constructor initializes state correctly\n");
}

void testConstructorConfiguresPinAsInputPullup() {
    EventCapture capture;
    DhtSensor sensor(5, DhtSensor::SENSOR_MODEL_DHT22, &capture);
    // pinMode was called with INPUT_PULLUP
    assert(fakePinModeCallCount >= 1);
    printf("  Constructor configures pin as INPUT_PULLUP\n");
}

void testValidateChecksumAcceptsValidData() {
    uint8_t data[] = {50, 0, 25, 0, 75}; // 50+0+25+0 = 75
    assert(DhtSensor::validateChecksum(data) == true);
    printf("  validateChecksum accepts valid data\n");
}

void testValidateChecksumRejectsInvalidData() {
    uint8_t data[] = {50, 0, 25, 0, 99}; // 50+0+25+0 = 75 != 99
    assert(DhtSensor::validateChecksum(data) == false);
    printf("  validateChecksum rejects invalid data\n");
}

void testMinimumSamplingIntervalForDht11() {
    assert(DhtSensor::minimumSamplingIntervalFor(DhtSensor::SENSOR_MODEL_DHT11) == 1000);
    printf("  minimumSamplingIntervalFor returns 1000 for DHT11\n");
}

void testMinimumSamplingIntervalForDht22() {
    assert(DhtSensor::minimumSamplingIntervalFor(DhtSensor::SENSOR_MODEL_DHT22) == 2000);
    printf("  minimumSamplingIntervalFor returns 2000 for DHT22\n");
}

void testMinimumSamplingIntervalForUnknownModel() {
    assert(DhtSensor::minimumSamplingIntervalFor(99) == 2000);
    printf("  minimumSamplingIntervalFor returns 2000 for unknown model\n");
}

void testDefaultSamplingInterval() {
    EventCapture capture;
    TestableDhtSensor sensor(2, DhtSensor::SENSOR_MODEL_DHT22, &capture);
    assert(sensor.getSamplingIntervalDefault() == 2000);
    printf("  Default sampling interval is 2000 ms\n");
}

void testMinimalSamplingIntervalDht22() {
    EventCapture capture;
    TestableDhtSensor sensor(2, DhtSensor::SENSOR_MODEL_DHT22, &capture);
    assert(sensor.getSamplingIntervalMinimal() == 2000);
    printf("  Minimal sampling interval for DHT22 is 2000 ms\n");
}

void testMinimalSamplingIntervalDht11() {
    EventCapture capture;
    TestableDhtSensor sensor(2, DhtSensor::SENSOR_MODEL_DHT11, &capture);
    assert(sensor.getSamplingIntervalMinimal() == 1000);
    printf("  Minimal sampling interval for DHT11 is 1000 ms\n");
}

void testSensorModelConstants() {
    assert(DhtSensor::SENSOR_MODEL_DHT11 == 11);
    assert(DhtSensor::SENSOR_MODEL_DHT22 == 22);
    assert(DhtSensor::SENSOR_MODEL_DHT21 == 21);
    assert(DhtSensor::SENSOR_MODEL_AM2301 == 21);
    printf("  Sensor model constants are correct\n");
}

int main() {
    printf("Running DhtSensor tests...\n");
    testConstructorInitializesState();
    testConstructorConfiguresPinAsInputPullup();
    testValidateChecksumAcceptsValidData();
    testValidateChecksumRejectsInvalidData();
    testMinimumSamplingIntervalForDht11();
    testMinimumSamplingIntervalForDht22();
    testMinimumSamplingIntervalForUnknownModel();
    testDefaultSamplingInterval();
    testMinimalSamplingIntervalDht22();
    testMinimalSamplingIntervalDht11();
    testSensorModelConstants();
    printf("All DhtSensor tests passed!\n");
    return 0;
}
