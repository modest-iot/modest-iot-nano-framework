#include <time.h>
#include "DigitalLightIntensitySensor.h"
#include "FakeI2cBus.h"
#include "EventAggregator.h"
#include "RulesEngine.h"
#include "ServoMotor.h"
#include <Arduino.h>
#include <Wire.h>
#include <cassert>
#include <cmath>
#include <limits>

namespace {
class Recorder : public EventHandler {
public:
    static constexpr size_t MAX_EVENTS = 16;
    int eventIds[MAX_EVENTS];
    int sourceIds[MAX_EVENTS];
    size_t count = 0;
    void on(const Event event) override {
        if (count < MAX_EVENTS) {
            eventIds[count] = event.identifier;
            sourceIds[count] = event.sourceId;
            count++;
        }
    }
};

class CommandRecorder : public CommandHandler {
public:
    int count = 0;

    void handle(const Command) override {
        ++count;
    }
};

class RulesOutputRouter : public EventHandler {
public:
    RulesOutputRouter(Recorder& recorder, RulesEngine& rulesEngine)
        : recorder(recorder), rulesEngine(rulesEngine) {}

    void on(const Event event) override {
        recorder.on(event);
        rulesEngine.on(event);
    }

private:
    Recorder& recorder;
    RulesEngine& rulesEngine;
};

struct ActionResultCapture {
    int eventId = -1;
    CommandExecutionStatus status = CommandExecutionStatus::Unknown;
    int count = 0;
};

void captureActionResult(const int eventId, CommandHandler*, Command,
                         const CommandExecutionResult result, void* context) {
    auto* capture = static_cast<ActionResultCapture*>(context);
    capture->eventId = eventId;
    capture->status = result.status;
    ++capture->count;
}
} // namespace

void testInjectedBusStartupSamplingAndFailure() {
    Recorder recorder;
    FakeI2cBus bus;
    DigitalLightIntensitySensor sensor(bus, 21, 0x23, 10.0f, &recorder);
    assert(bus.writeCount == 0);
    assert(bus.writeReadCount == 0);
    assert(!sensor.isInitialized());
    assert(!sensor.hasValidReading());
    assert(sensor.getDataValidity(20) == Sensor::DataValidity::NoValidSample);
    assert(sensor.getLastSuccessfulReadAgeInMilliseconds() == std::numeric_limits<uint32_t>::max());
    assert(!sensor.isReadFaultActive());

    assert(bus.start({21, 22, 400000}) == I2cStatus::Ok);
    bus.nextWriteStatus = I2cStatus::AddressNack;
    assert(!sensor.start());
    assert(!sensor.isInitialized());
    assert(sensor.getLastI2cResult().status == I2cStatus::AddressNack);

    assert(sensor.start());
    assert(sensor.isInitialized());
    assert(bus.writeCount == 4);

    fakeMillis = 180;
    bus.queueRead({0, 120});
    sensor.on(Sensor::MEASURE_DATA_REQUESTED_EVENT);
    assert(sensor.hasValidReading());
    assert(sensor.getLastSuccessfulReadAgeInMilliseconds() == 0);
    assert(sensor.getDataValidity(20) == Sensor::DataValidity::Fresh);
    assert(sensor.hasValidSample());
    assert(sensor.isLightDetected());
    assert(std::fabs(sensor.getIlluminanceInLux() - 100.0f) < 0.01f);
    assert(recorder.count == 1);
    assert(recorder.eventIds[0] == DigitalLightIntensitySensor::LIGHT_DETECTED_EVENT_IDENTIFIER);

    sensor.setFaultThreshold(1);
    bus.queueRead({0}, I2cStatus::ShortRead);
    sensor.on(Sensor::MEASURE_DATA_REQUESTED_EVENT);
    assert(recorder.count == 2);
    assert(recorder.eventIds[1] == Sensor::SENSOR_FAULT_EVENT_IDENTIFIER);
    assert(sensor.isReadFaultActive());
    assert(sensor.getDataValidity(20) == Sensor::DataValidity::Fresh);
    assert(sensor.getLastI2cResult().status == I2cStatus::ShortRead);
    assert(sensor.hasValidSample());
    assert(sensor.isLightDetected());
    assert(std::fabs(sensor.getIlluminanceInLux() - 100.0f) < 0.01f);

    fakeMillis = 200;
    bus.queueRead({0, 120});
    sensor.on(Sensor::MEASURE_DATA_REQUESTED_EVENT);
    assert(recorder.count == 3);
    assert(recorder.eventIds[2] == Sensor::SENSOR_RECOVERED_EVENT_IDENTIFIER);
    assert(!sensor.isReadFaultActive());
    assert(sensor.getLastSuccessfulReadAgeInMilliseconds() == 0);
    assert(sensor.getDataValidity(20) == Sensor::DataValidity::Fresh);
    assert(sensor.hasValidSample());
    assert(sensor.isLightDetected());
    assert(std::fabs(sensor.getIlluminanceInLux() - 100.0f) < 0.01f);
    fakeMillis = 221;
    assert(sensor.getLastSuccessfulReadAgeInMilliseconds() == 21);
    assert(sensor.getDataValidity(20) == Sensor::DataValidity::Stale);
    fakeMillis = 0;
}

void testSensorEventToCheckedActuatorComposition() {
    fakeMillis = 0;
    fakeLedcAttachResult = true;
    fakeLedcWriteResult = true;

    Recorder recorder;
    FakeI2cBus bus;
    EventAggregator aggregator;
    RulesEngine rulesEngine;
    CommandRecorder parentCommands;
    ServoMotor servo(13, 0, &parentCommands);
    RulesOutputRouter outputRouter(recorder, rulesEngine);
    ActionResultCapture actionResult;

    int patternEventIds[] = {Sensor::SENSOR_FAULT_EVENT_IDENTIFIER,
                             Sensor::SENSOR_RECOVERED_EVENT_IDENTIFIER};
    assert(aggregator.addAllMatchPattern(patternEventIds, 2, 5, 900));
    assert(rulesEngine.addRule(900, &servo, ServoMotor::DRIVE_TO_ANGLE_COMMAND_IDENTIFIER, 90));
    rulesEngine.setActionResultObserver(captureActionResult, &actionResult);
    aggregator.setOutputHandler(&outputRouter);

    DigitalLightIntensitySensor sensor(bus, 21, 0x23, 10.0f, &aggregator);
    assert(bus.start({21, 22, 400000}) == I2cStatus::Ok);
    assert(sensor.start());
    sensor.setFaultThreshold(1);

    fakeMillis = 180;
    bus.queueRead({0, 120});
    sensor.on(Sensor::MEASURE_DATA_REQUESTED_EVENT);
    assert(recorder.count == 1);
    assert(recorder.eventIds[0] == DigitalLightIntensitySensor::LIGHT_DETECTED_EVENT_IDENTIFIER);

    bus.queueRead({0}, I2cStatus::ShortRead);
    sensor.on(Sensor::MEASURE_DATA_REQUESTED_EVENT);
    assert(recorder.count == 2);
    assert(recorder.eventIds[1] == Sensor::SENSOR_FAULT_EVENT_IDENTIFIER);

    fakeMillis = 200;
    bus.queueRead({0, 120});
    sensor.on(Sensor::MEASURE_DATA_REQUESTED_EVENT);
    assert(recorder.count == 4);
    assert(recorder.eventIds[2] == Sensor::SENSOR_RECOVERED_EVENT_IDENTIFIER);
    assert(recorder.eventIds[3] == 900);
    assert(recorder.sourceIds[2] == 21);
    assert(recorder.sourceIds[3] == EventAggregator::DEVICE_SOURCE_ID);
    assert(actionResult.count == 1);
    assert(actionResult.eventId == 900);
    assert(actionResult.status == CommandExecutionStatus::Succeeded);
    assert(parentCommands.count == 1);
    assert(servo.getCurrentActiveAngleInDegrees() == 90);

    aggregator.on(Event(901, 99));
    assert(recorder.count == 5);
    assert(recorder.eventIds[4] == 901);
    assert(actionResult.count == 1);
    assert(parentCommands.count == 1);
    fakeMillis = 0;
}

[[nodiscard]] int main() {
    testInjectedBusStartupSamplingAndFailure();
    testSensorEventToCheckedActuatorComposition();

    Recorder recorder;
    DigitalLightIntensitySensor sensor(21, 0x23, 10.0f, &recorder);
    assert(sensor.isInitialized());
    assert(!sensor.hasValidSample());
    sensor.on(Sensor::MEASURE_DATA_REQUESTED_EVENT);
    assert(Wire.requests == 0);
    fakeMillis = 179;
    sensor.on(Sensor::MEASURE_DATA_REQUESTED_EVENT);
    assert(Wire.requests == 0);
    fakeMillis = 180;
    sensor.on(Sensor::MEASURE_DATA_REQUESTED_EVENT);
    assert(sensor.hasValidSample());
    assert(recorder.count == 1);
    assert(recorder.eventIds[recorder.count - 1] == sensor.LIGHT_ABSENT_EVENT_IDENTIFIER);
    Wire.raw = 120;
    sensor.on(Sensor::MEASURE_DATA_REQUESTED_EVENT);
    assert(recorder.eventIds[recorder.count - 1] == sensor.LIGHT_DETECTED_EVENT_IDENTIFIER);
    assert(std::fabs(sensor.getIlluminanceInLux() - 100.0f) < 0.01f);
    for (int failedRead = 0; failedRead < 2; ++failedRead) {
        Wire.readFailureAt = failedRead;
        sensor.on(Sensor::MEASURE_DATA_REQUESTED_EVENT);
        assert(recorder.count == 2);
        assert(sensor.isLightDetected());
        assert(sensor.hasValidSample());
        assert(std::fabs(sensor.getIlluminanceInLux() - 100.0f) < 0.01f);
    }
    Wire.readFailureAt = -1;
    Wire.byteCount = 1;
    sensor.on(Sensor::MEASURE_DATA_REQUESTED_EVENT);
    assert(recorder.count == 2);
    assert(sensor.isLightDetected());
    assert(sensor.hasValidSample());
    assert(std::fabs(sensor.getIlluminanceInLux() - 100.0f) < 0.01f);

    for (int failedCommand = 1; failedCommand <= 3; ++failedCommand) {
        Wire = FakeWire();
        Wire.failureAt = failedCommand;
        DigitalLightIntensitySensor failed(21, 0x5c, 10.0f, &recorder);
        assert(!failed.isInitialized());
        assert(!failed.hasValidSample());
        assert(Wire.commandCount == failedCommand);
        failed.on(Sensor::MEASURE_DATA_REQUESTED_EVENT);
        assert(Wire.requests == 0);
        Wire.failureAt = -1;
        assert(failed.initialize());
        assert(failed.isInitialized());
        failed.on(Sensor::MEASURE_DATA_REQUESTED_EVENT);
        assert(Wire.requests == 0);
        fakeMillis += 180;
        Wire.raw = 12;
        failed.on(Sensor::MEASURE_DATA_REQUESTED_EVENT);
        assert(failed.hasValidSample());
        assert(failed.isLightDetected());
        assert(recorder.sourceIds[recorder.count - 1] == 21);
        assert(failed.initialize());
        assert(!failed.hasValidSample());
    }
}
