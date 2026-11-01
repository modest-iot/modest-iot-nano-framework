#include "DigitalLightIntensitySensor.h"
#include <Wire.h>
#include <cassert>
#include <cmath>
#include <vector>

FakeWire Wire;
unsigned long now = 0;
unsigned long millis() { return now; }

class Recorder : public EventHandler {
public:
    std::vector<Event> events;
    void on(Event event) override { events.push_back(event); }
};

int main() {
    Recorder recorder;
    DigitalLightIntensitySensor sensor(21, 0x23, 10.0f, &recorder);
    assert(sensor.isInitialized());
    assert(!sensor.hasValidSample());
    sensor.on(Sensor::MEASURE_DATA_REQUESTED_EVENT);
    assert(Wire.requests == 0);
    now = 179;
    sensor.on(Sensor::MEASURE_DATA_REQUESTED_EVENT);
    assert(Wire.requests == 0);
    now = 180;
    sensor.on(Sensor::MEASURE_DATA_REQUESTED_EVENT);
    assert(sensor.hasValidSample());
    assert(recorder.events.size() == 1);
    assert(recorder.events.back().identifier == sensor.LIGHT_ABSENT_EVENT_IDENTIFIER);
    Wire.raw = 120;
    sensor.on(Sensor::MEASURE_DATA_REQUESTED_EVENT);
    assert(recorder.events.back().identifier == sensor.LIGHT_DETECTED_EVENT_IDENTIFIER);
    assert(std::fabs(sensor.getIlluminanceInLux() - 100.0f) < 0.01f);
    for (int failedRead = 0; failedRead < 2; ++failedRead) {
        Wire.readFailureAt = failedRead;
        sensor.on(Sensor::MEASURE_DATA_REQUESTED_EVENT);
        assert(recorder.events.size() == 2);
        assert(sensor.isLightDetected());
        assert(sensor.hasValidSample());
        assert(std::fabs(sensor.getIlluminanceInLux() - 100.0f) < 0.01f);
    }
    Wire.readFailureAt = -1;
    Wire.byteCount = 1;
    sensor.on(Sensor::MEASURE_DATA_REQUESTED_EVENT);
    assert(recorder.events.size() == 2);
    assert(sensor.isLightDetected());
    assert(sensor.hasValidSample());
    assert(std::fabs(sensor.getIlluminanceInLux() - 100.0f) < 0.01f);

    for (int failedCommand = 1; failedCommand <= 3; ++failedCommand) {
        Wire = FakeWire();
        Wire.failureAt = failedCommand;
        DigitalLightIntensitySensor failed(21, 0x5c, 10.0f, &recorder);
        assert(!failed.isInitialized());
        assert(!failed.hasValidSample());
        assert(static_cast<int>(Wire.commands.size()) == failedCommand);
        failed.on(Sensor::MEASURE_DATA_REQUESTED_EVENT);
        assert(Wire.requests == 0);
        Wire.failureAt = -1;
        assert(failed.initialize());
        assert(failed.isInitialized());
        failed.on(Sensor::MEASURE_DATA_REQUESTED_EVENT);
        assert(Wire.requests == 0);
        now += 180;
        Wire.raw = 12;
        failed.on(Sensor::MEASURE_DATA_REQUESTED_EVENT);
        assert(failed.hasValidSample());
        assert(failed.isLightDetected());
        assert(recorder.events.back().sourceId == 21);
        assert(failed.initialize());
        assert(!failed.hasValidSample());
    }
}