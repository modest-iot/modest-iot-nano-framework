#include <time.h>
/**
 * @file test.cpp
 * @brief Host tests for GasSensor (MQ-series analog gas detection).
 * @date 2026-11-13
 * @version 1.0
 */

#include "GasSensor.h"
#include <cassert>
#include <cstdio>
#include <cmath>

// Test helper: captures events from the sensor
class EventCapture : public EventHandler {
public:
    Event lastEvent{-1};
    void on(Event event) override { lastEvent = event; }
};

// Testable subclass exposes protected methods for testing
class TestableGasSensor : public GasSensor {
public:
    using GasSensor::GasSensor;
    using GasSensor::processEvent;
    using GasSensor::calculateSensorResistanceInKiloOhm;
    using GasSensor::loadDefaultGasCalibrationParameters;
    uint32_t getSamplingIntervalDefault() const { return getDefaultSamplingIntervalInMilliseconds(); }
    uint32_t getSamplingIntervalMinimal() const { return getMinimalSamplingIntervalInMilliseconds(); }
};

void resetMocks() {
    fakeMillis = 0;
    fakeDigitalReadValue = HIGH;
    for (int i = 0; i < 64; i++) {
        fakeAnalogRead[i] = 0;
    }
    fakePinModeCallCount = 0;
}

void testConstructorInitializesState() {
    resetMocks();
    EventCapture capture;
    TestableGasSensor sensor(34, -1, &capture);
    assert(sensor.getRawAnalogValue() == -1);
    assert(sensor.getSensorResistanceInKiloOhm() == -1.0f);
    assert(sensor.getRsR0Ratio() == -1.0f);
    assert(sensor.getOperatingMode() == GasSensor::OperatingMode::RawMode);
    assert(sensor.getTargetGasType() == GasSensor::GasType::None);
    assert(sensor.isGasDetected() == false);
    printf("  Constructor initializes state correctly\n");
}

void testConstructorWithDigitalPin() {
    resetMocks();
    EventCapture capture;
    TestableGasSensor sensor(34, 32, &capture);
    assert(sensor.getOperatingMode() == GasSensor::OperatingMode::RawMode);
    // Both pins should be configured
    assert(fakePinModeCallCount >= 2);
    printf("  Constructor configures analog and digital pins\n");
}

void testConstructorWithCalibratingMode() {
    resetMocks();
    EventCapture capture;
    TestableGasSensor sensor(34, -1, &capture, GasSensor::OperatingMode::CalibratingMode);
    assert(sensor.getOperatingMode() == GasSensor::OperatingMode::CalibratingMode);
    printf("  Constructor accepts CalibratingMode\n");
}

void testConstructorWithAccurateMode() {
    resetMocks();
    EventCapture capture;
    TestableGasSensor sensor(34, -1, &capture, GasSensor::OperatingMode::AccurateMode);
    assert(sensor.getOperatingMode() == GasSensor::OperatingMode::AccurateMode);
    printf("  Constructor accepts AccurateMode\n");
}

void testCalculateSensorResistance() {
    // Rs = (Vcc / Vout - 1) * RL
    // Vout = rawValue * (3.3 / 4095)
    // For rawValue = 2048 (mid-scale): Vout = 1.65V
    // Rs = (3.3 / 1.65 - 1) * 1.0 = 1.0 kOhm
    float rs = TestableGasSensor::calculateSensorResistanceInKiloOhm(2048);
    assert(rs > 0.9f && rs < 1.1f);
    printf("  calculateSensorResistance computes correctly for mid-scale\n");
}

void testCalculateSensorResistanceZero() {
    // rawValue = 0 means Vout = 0, which is invalid
    float rs = TestableGasSensor::calculateSensorResistanceInKiloOhm(0);
    assert(rs == -1.0f);
    printf("  calculateSensorResistance returns -1 for zero input\n");
}

void testCalculateSensorResistanceMax() {
    // rawValue = 4095 means Vout = 3.3V, Rs approaches 0
    float rs = TestableGasSensor::calculateSensorResistanceInKiloOhm(4095);
    assert(rs >= 0.0f && rs < 0.01f);
    printf("  calculateSensorResistance returns near-zero for max input\n");
}

void testLoadDefaultCalibrationLPG() {
    GasSensor::GasCalibrationParameters params;
    assert(TestableGasSensor::loadDefaultGasCalibrationParameters(GasSensor::GasType::LPG, params) == true);
    assert(params.slope != 0.0f);
    assert(params.intercept != 0.0f);
    printf("  loadDefaultGasCalibrationParameters loads LPG parameters\n");
}

void testLoadDefaultCalibrationNone() {
    GasSensor::GasCalibrationParameters params;
    assert(TestableGasSensor::loadDefaultGasCalibrationParameters(GasSensor::GasType::None, params) == false);
    printf("  loadDefaultGasCalibrationParameters rejects None gas type\n");
}

void testSetR0() {
    resetMocks();
    EventCapture capture;
    TestableGasSensor sensor(34, -1, &capture);
    sensor.setR0(9.0f);
    assert(sensor.getOperatingMode() == GasSensor::OperatingMode::AccurateMode);
    printf("  setR0 switches to AccurateMode\n");
}

void testSetR0Invalid() {
    resetMocks();
    EventCapture capture;
    TestableGasSensor sensor(34, -1, &capture, GasSensor::OperatingMode::RawMode);
    sensor.setR0(0.0f);
    assert(sensor.getOperatingMode() == GasSensor::OperatingMode::RawMode);
    sensor.setR0(-1.0f);
    assert(sensor.getOperatingMode() == GasSensor::OperatingMode::RawMode);
    printf("  setR0 rejects invalid values\n");
}

void testSetTargetGas() {
    resetMocks();
    EventCapture capture;
    TestableGasSensor sensor(34, -1, &capture);
    assert(sensor.setTargetGas(GasSensor::GasType::LPG) == true);
    assert(sensor.getTargetGasType() == GasSensor::GasType::LPG);
    assert(sensor.setTargetGas(GasSensor::GasType::CarbonMonoxide) == true);
    assert(sensor.getTargetGasType() == GasSensor::GasType::CarbonMonoxide);
    printf("  setTargetGas changes the target gas type\n");
}

void testSetTargetGasInvalid() {
    resetMocks();
    EventCapture capture;
    TestableGasSensor sensor(34, -1, &capture);
    assert(sensor.setTargetGas(GasSensor::GasType::None) == false);
    assert(sensor.getTargetGasType() == GasSensor::GasType::None);
    printf("  setTargetGas rejects None gas type\n");
}

void testSetGasCalibrationParameters() {
    resetMocks();
    EventCapture capture;
    TestableGasSensor sensor(34, -1, &capture);
    assert(sensor.setGasCalibrationParameters(GasSensor::GasType::LPG, -0.5f, 3.0f) == true);
    printf("  setGasCalibrationParameters stores custom parameters\n");
}

void testSetGasCalibrationParametersInvalid() {
    resetMocks();
    EventCapture capture;
    TestableGasSensor sensor(34, -1, &capture);
    assert(sensor.setGasCalibrationParameters(GasSensor::GasType::None, -0.5f, 3.0f) == false);
    printf("  setGasCalibrationParameters rejects None gas type\n");
}

void testStartCalibration() {
    resetMocks();
    fakeMillis = 0;
    EventCapture capture;
    TestableGasSensor sensor(34, -1, &capture, GasSensor::OperatingMode::RawMode);
    sensor.startCalibration();
    assert(sensor.getOperatingMode() == GasSensor::OperatingMode::CalibratingMode);
    printf("  startCalibration switches to CalibratingMode\n");
}

void testRawModeAnalogReading() {
    resetMocks();
    EventCapture capture;
    TestableGasSensor sensor(34, -1, &capture);
    fakeAnalogRead[34] = 2048; // Mid-scale reading
    Event request(GasSensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, 34);
    sensor.processEvent(request);
    assert(sensor.getRawAnalogValue() == 2048);
    assert(sensor.getSensorResistanceInKiloOhm() > 0.0f);
    printf("  RawMode processes analog reading correctly\n");
}

void testRawModeDigitalGasDetected() {
    resetMocks();
    EventCapture capture;
    TestableGasSensor sensor(34, 32, &capture);
    fakeAnalogRead[34] = 2048;
    fakeDigitalReadValue = HIGH; // Gas detected
    Event request(GasSensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, 34);
    sensor.processEvent(request);
    assert(sensor.isGasDetected() == true);
    printf("  RawMode detects gas via digital pin\n");
}

void testRawModeDigitalGasNotDetected() {
    resetMocks();
    EventCapture capture;
    TestableGasSensor sensor(34, 32, &capture);
    fakeAnalogRead[34] = 2048;
    fakeDigitalReadValue = LOW; // No gas
    Event request(GasSensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, 34);
    sensor.processEvent(request);
    assert(sensor.isGasDetected() == false);
    printf("  RawMode reports no gas when digital pin is LOW\n");
}

void testAccurateModeRsR0Ratio() {
    resetMocks();
    EventCapture capture;
    TestableGasSensor sensor(34, -1, &capture);
    sensor.setR0(9.0f);
    fakeAnalogRead[34] = 2048; // Rs ~ 1.0 kOhm
    Event request(GasSensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, 34);
    sensor.processEvent(request);
    float ratio = sensor.getRsR0Ratio();
    assert(ratio > 0.0f);
    printf("  AccurateMode calculates Rs/R0 ratio\n");
}

void testAccurateModeGasDetected() {
    resetMocks();
    EventCapture capture;
    TestableGasSensor sensor(34, -1, &capture);
    sensor.setR0(9.0f);
    fakeAnalogRead[34] = 1000; // Low reading = high resistance = gas present
    Event request(GasSensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, 34);
    sensor.processEvent(request);
    // Rs/R0 ratio deviates significantly from 1.0
    assert(sensor.isGasDetected() == true);
    printf("  AccurateMode detects gas when Rs/R0 deviates from 1.0\n");
}

void testCalibratingModeSuppressesEvents() {
    resetMocks();
    fakeMillis = 0;
    EventCapture capture;
    TestableGasSensor sensor(34, -1, &capture, GasSensor::OperatingMode::CalibratingMode);
    sensor.startCalibration();
    fakeAnalogRead[34] = 2048;
    Event request(GasSensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, 34);
    sensor.processEvent(request);
    // Event should be suppressed during calibration (not enough time/readings)
    assert(capture.lastEvent.identifier == -1);
    printf("  CalibratingMode suppresses events\n");
}

void testCalibratingModeCompletes() {
    resetMocks();
    fakeMillis = 0;
    EventCapture capture;
    TestableGasSensor sensor(34, -1, &capture, GasSensor::OperatingMode::CalibratingMode);
    sensor.startCalibration();
    fakeAnalogRead[34] = 2048;
    
    // Simulate calibration duration passing
    fakeMillis = 35000; // Past CALIBRATION_DURATION_IN_MILLISECONDS (30000)
    
    // Feed enough readings
    for (int i = 0; i < 15; i++) {
        Event request(GasSensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, 34);
        sensor.processEvent(request);
    }
    assert(sensor.getOperatingMode() == GasSensor::OperatingMode::AccurateMode);
    printf("  CalibratingMode transitions to AccurateMode after sufficient readings\n");
}

void testGetGasConcentrationInPpm() {
    resetMocks();
    EventCapture capture;
    TestableGasSensor sensor(34, -1, &capture);
    sensor.setR0(9.0f);
    sensor.setTargetGas(GasSensor::GasType::LPG);
    fakeAnalogRead[34] = 2048;
    Event request(GasSensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, 34);
    sensor.processEvent(request);
    float ppm = sensor.getGasConcentrationInPpm();
    assert(ppm > 0.0f);
    printf("  getGasConcentrationInPpm returns valid value\n");
}

void testGetGasConcentrationInPpmNoTargetGas() {
    resetMocks();
    EventCapture capture;
    TestableGasSensor sensor(34, -1, &capture);
    sensor.setR0(9.0f);
    // No target gas set
    fakeAnalogRead[34] = 2048;
    Event request(GasSensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, 34);
    sensor.processEvent(request);
    float ppm = sensor.getGasConcentrationInPpm();
    assert(ppm == -1.0f);
    printf("  getGasConcentrationInPpm returns -1 without target gas\n");
}

void testGetGasConcentrationInPpmRawMode() {
    resetMocks();
    EventCapture capture;
    TestableGasSensor sensor(34, -1, &capture);
    sensor.setTargetGas(GasSensor::GasType::LPG);
    // Not in AccurateMode
    float ppm = sensor.getGasConcentrationInPpm();
    assert(ppm == -1.0f);
    printf("  getGasConcentrationInPpm returns -1 in RawMode\n");
}

void testGetRsR0RatioNotAccurateMode() {
    resetMocks();
    EventCapture capture;
    TestableGasSensor sensor(34, -1, &capture);
    assert(sensor.getRsR0Ratio() == -1.0f);
    printf("  getRsR0Ratio returns -1 when not in AccurateMode\n");
}

void testDefaultSamplingInterval() {
    resetMocks();
    EventCapture capture;
    TestableGasSensor sensor(34, -1, &capture);
    assert(sensor.getSamplingIntervalDefault() == 1000);
    printf("  Default sampling interval is 1000 ms\n");
}

void testMinimalSamplingInterval() {
    resetMocks();
    EventCapture capture;
    TestableGasSensor sensor(34, -1, &capture);
    assert(sensor.getSamplingIntervalMinimal() == 100);
    printf("  Minimal sampling interval is 100 ms\n");
}

void testGasTypeConstants() {
    assert(GasSensor::GasType::None == GasSensor::GasType::None);
    assert(static_cast<int>(GasSensor::GasType::LPG) == 1);
    assert(static_cast<int>(GasSensor::GasType::Methane) == 2);
    assert(static_cast<int>(GasSensor::GasType::CarbonMonoxide) == 3);
    printf("  GasType enum constants are correct\n");
}

void testOperatingModeConstants() {
    assert(static_cast<int>(GasSensor::OperatingMode::RawMode) == 0);
    assert(static_cast<int>(GasSensor::OperatingMode::CalibratingMode) == 1);
    assert(static_cast<int>(GasSensor::OperatingMode::AccurateMode) == 2);
    printf("  OperatingMode enum constants are correct\n");
}

void testDigitalGasDetectedEventIdentifier() {
    assert(GasSensor::DIGITAL_GAS_DETECTED_EVENT_IDENTIFIER == 40);
    assert(GasSensor::DIGITAL_GAS_DETECTED_EVENT.identifier == 40);
    printf("  DIGITAL_GAS_DETECTED_EVENT identifier is correct\n");
}

void testNonMeasureEventSuppressed() {
    resetMocks();
    EventCapture capture;
    TestableGasSensor sensor(34, -1, &capture);
    fakeAnalogRead[34] = 2048;
    Event other(99, 34); // Not MEASURE_DATA_REQUESTED
    sensor.processEvent(other);
    assert(other.identifier == -1);
    printf("  Non-measure events are suppressed\n");
}

int main() {
    printf("Running GasSensor tests...\n");
    testConstructorInitializesState();
    testConstructorWithDigitalPin();
    testConstructorWithCalibratingMode();
    testConstructorWithAccurateMode();
    testCalculateSensorResistance();
    testCalculateSensorResistanceZero();
    testCalculateSensorResistanceMax();
    testLoadDefaultCalibrationLPG();
    testLoadDefaultCalibrationNone();
    testSetR0();
    testSetR0Invalid();
    testSetTargetGas();
    testSetTargetGasInvalid();
    testSetGasCalibrationParameters();
    testSetGasCalibrationParametersInvalid();
    testStartCalibration();
    testRawModeAnalogReading();
    testRawModeDigitalGasDetected();
    testRawModeDigitalGasNotDetected();
    testAccurateModeRsR0Ratio();
    testAccurateModeGasDetected();
    testCalibratingModeSuppressesEvents();
    testCalibratingModeCompletes();
    testGetGasConcentrationInPpm();
    testGetGasConcentrationInPpmNoTargetGas();
    testGetGasConcentrationInPpmRawMode();
    testGetRsR0RatioNotAccurateMode();
    testDefaultSamplingInterval();
    testMinimalSamplingInterval();
    testGasTypeConstants();
    testOperatingModeConstants();
    testDigitalGasDetectedEventIdentifier();
    testNonMeasureEventSuppressed();
    printf("All GasSensor tests passed!\n");
    return 0;
}
