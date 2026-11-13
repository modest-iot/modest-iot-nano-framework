#include <time.h>
#include "DigitalLightIntensitySensor.h"
#include <Arduino.h>
#include <Wire.h>
#include <cassert>
#include <cmath>

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
} // namespace

[[nodiscard]] int main() {
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