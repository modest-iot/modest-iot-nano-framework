#include <time.h>
#include <cassert>
#include <Arduino.h>
#include "RelayActuator.h"

namespace {
struct Recorder : CommandHandler {
    int count = 0;
    void handle(const Command) override { count++; }
};
} // namespace

[[nodiscard]] int main() {
    // Test 1: Constructor initializes pin as OUTPUT with initial state
    Recorder parent;
    RelayActuator relay(2, false, &parent);
    assert(!relay.isActive() && fakeDigitalWrite[2] == LOW);

    // Test 2: RELAY_ON_COMMAND turns relay on
    relay.handle(RelayActuator::RELAY_ON_COMMAND);
    assert(relay.isActive() && fakeDigitalWrite[2] == HIGH);

    // Test 3: RELAY_OFF_COMMAND turns relay off
    relay.handle(RelayActuator::RELAY_OFF_COMMAND);
    assert(!relay.isActive() && fakeDigitalWrite[2] == LOW);

    // Test 4: TOGGLE_RELAY_COMMAND toggles from off to on
    relay.handle(RelayActuator::TOGGLE_RELAY_COMMAND);
    assert(relay.isActive() && fakeDigitalWrite[2] == HIGH);

    // Test 5: TOGGLE_RELAY_COMMAND toggles from on to off
    relay.handle(RelayActuator::TOGGLE_RELAY_COMMAND);
    assert(!relay.isActive() && fakeDigitalWrite[2] == LOW);

    // Test 6: Unknown command does not change state but still propagates
    relay.handle(Command{999});
    assert(!relay.isActive() && fakeDigitalWrite[2] == LOW);

    // Test 7: Parent handler receives all propagated commands (including unknown)
    assert(parent.count == 5);

    // Test 8: Constructor with initial state true
    RelayActuator relayOn(3, true);
    assert(relayOn.isActive() && fakeDigitalWrite[3] == HIGH);

    return 0;
}
