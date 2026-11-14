#include <time.h>
#include <cassert>
#include <cstdio>
#include <Arduino.h>
#include "Buzzer.h"

// Tone/noTone mock variables are now defined in tests/fakes/ArduinoFake.cpp

namespace {
struct Recorder : CommandHandler {
    int count = 0;
    void handle(const Command) override { count++; }
};

void resetToneMocks() {
    fakeToneCalled = false;
    fakeNoToneCalled = false;
    fakeNoTonePin = -1;
    fakeToneCall.pin = 0;
    fakeToneCall.frequency = 0;
    fakeToneCall.duration = 0;
}
} // namespace

[[nodiscard]] int main() {
    // Test 1: Constructor initializes pin as OUTPUT and ensures buzzer is off
    {
        resetToneMocks();
        Buzzer buzzer(2);
        assert(!buzzer.isActive());
        assert(buzzer.getBufferedFrequency() == 0);
        assert(buzzer.getBufferedDuration() == 0);
    }

    // Test 2: TURN_ON_COMMAND plays default tone
    {
        resetToneMocks();
        Buzzer buzzer(3);
        buzzer.handle(Buzzer::TURN_ON_COMMAND);
        assert(buzzer.isActive());
        assert(fakeToneCalled);
        assert(fakeToneCall.pin == 3);
        assert(fakeToneCall.frequency == 1000); // DEFAULT_TONE_FREQUENCY_IN_HERTZ
    }

    // Test 3: TURN_OFF_COMMAND stops the buzzer
    {
        resetToneMocks();
        Buzzer buzzer(4);
        buzzer.handle(Buzzer::TURN_ON_COMMAND);
        buzzer.handle(Buzzer::TURN_OFF_COMMAND);
        assert(!buzzer.isActive());
        assert(fakeNoToneCalled);
        assert(fakeNoTonePin == 4);
    }

    // Test 4: PLAY_TONE_COMMAND plays buffered frequency and duration
    {
        resetToneMocks();
        Buzzer buzzer(5);
        buzzer.setTone(440, 500);
        assert(buzzer.getBufferedFrequency() == 440);
        assert(buzzer.getBufferedDuration() == 500);
        buzzer.handle(Buzzer::PLAY_TONE_COMMAND);
        assert(buzzer.isActive());
        assert(fakeToneCalled);
        assert(fakeToneCall.pin == 5);
        assert(fakeToneCall.frequency == 440);
        assert(fakeToneCall.duration == 500);
    }

    // Test 5: PLAY_TONE_COMMAND with invalid parameters turns off buzzer
    {
        resetToneMocks();
        Buzzer buzzer(6);
        buzzer.setTone(0, 0); // Invalid: zero frequency and duration
        buzzer.handle(Buzzer::PLAY_TONE_COMMAND);
        assert(!buzzer.isActive());
        assert(fakeNoToneCalled);
    }

    // Test 6: PLAY_FREQUENCY_COMMAND sets buffered frequency from payload, then plays
    {
        resetToneMocks();
        Buzzer buzzer(7);
        buzzer.setTone(0, 1000); // Set duration only; frequency will come from payload
        assert(buzzer.getBufferedDuration() == 1000);
        buzzer.handle(Command{Buzzer::PLAY_FREQUENCY_COMMAND_IDENTIFIER, 880});
        assert(buzzer.isActive());
        assert(fakeToneCalled);
        assert(fakeToneCall.pin == 7);
        assert(fakeToneCall.frequency == 880);
        assert(fakeToneCall.duration == 1000);
        assert(buzzer.getBufferedFrequency() == 880); // Frequency buffered for reference
    }

    // Test 7: PLAY_FREQUENCY_COMMAND with no buffered duration turns off buzzer
    {
        resetToneMocks();
        Buzzer buzzer(8);
        buzzer.setTone(0, 0); // No duration buffered
        buzzer.handle(Command{Buzzer::PLAY_FREQUENCY_COMMAND_IDENTIFIER, 440});
        assert(!buzzer.isActive());
        assert(fakeNoToneCalled);
    }

    // Test 8: PLAY_FREQUENCY_COMMAND with zero payload turns off buzzer
    {
        resetToneMocks();
        Buzzer buzzer(9);
        buzzer.setTone(0, 500);
        buzzer.handle(Command{Buzzer::PLAY_FREQUENCY_COMMAND_IDENTIFIER, 0});
        assert(!buzzer.isActive());
        assert(fakeNoToneCalled);
    }

    // Test 9: Parent handler receives propagated commands
    {
        resetToneMocks();
        Recorder parent;
        Buzzer buzzer(10, &parent);
        buzzer.handle(Buzzer::TURN_ON_COMMAND);
        buzzer.handle(Buzzer::TURN_OFF_COMMAND);
        buzzer.handle(Buzzer::PLAY_TONE_COMMAND);
        buzzer.handle(Command{Buzzer::PLAY_FREQUENCY_COMMAND_IDENTIFIER, 440});
        assert(parent.count == 4);
    }

    // Test 10: setTone updates both frequency and duration
    {
        Buzzer buzzer(11);
        buzzer.setTone(2000, 2000);
        assert(buzzer.getBufferedFrequency() == 2000);
        assert(buzzer.getBufferedDuration() == 2000);
    }

    return 0;
}
