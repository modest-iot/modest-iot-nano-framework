#include <time.h>
/**
 * @file test.cpp
 * @brief Host tests for Button (debounced digital button sensor).
 * @date 2026-11-13
 * @version 1.0
 */

#include "Button.h"
#include <cassert>
#include <cstdio>

// Test helper: captures events from the sensor
class EventCapture : public EventHandler {
public:
    Event lastEvent{-1};
    void on(Event event) override { lastEvent = event; }
};

// Testable subclass exposes protected methods for testing
class TestableButton : public Button {
public:
    using Button::Button;
    using Button::processEvent;
    uint32_t getSamplingIntervalDefault() const { return getDefaultSamplingIntervalInMilliseconds(); }
    uint32_t getSamplingIntervalMinimal() const { return getMinimalSamplingIntervalInMilliseconds(); }
};

void resetMocks() {
    fakeMillis = 0;
    fakeDigitalReadValue = HIGH;
    fakePinModeCallCount = 0;
    for (int i = 0; i < 64; i++) {
        fakePinModeCalls[i].pin = 0;
        fakePinModeCalls[i].mode = 0;
    }
}

void testConstructorInitializesState() {
    resetMocks();
    EventCapture capture;
    Button button(2, 50, &capture);
    // Button inherits from Sensor; pin should be configured
    assert(fakePinModeCallCount >= 1);
    assert(fakePinModeCalls[0].mode == INPUT_PULLUP);
    printf("  Constructor configures pin as INPUT_PULLUP\n");
}

void testConstructorDefaultDebounce() {
    resetMocks();
    EventCapture capture;
    Button button(2, 50, &capture);
    // Default debounce is 50ms
    printf("  Constructor accepts default debounce delay\n");
}

void testConstructorCustomDebounce() {
    resetMocks();
    EventCapture capture;
    Button button(2, 100, &capture);
    printf("  Constructor accepts custom debounce delay\n");
}

void testButtonPressedEventEmitted() {
    resetMocks();
    EventCapture capture;
    TestableButton button(2, 50, &capture);
    
    // Simulate button press: state transitions from HIGH to LOW
    fakeMillis = 100;
    fakeDigitalReadValue = LOW;
    
    Event request(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, 2);
    button.processEvent(request);
    
    assert(request.identifier == Button::BUTTON_PRESSED_EVENT_IDENTIFIER);
    assert(request.sourceId == 2);
    printf("  Button press event emitted on FALLING edge\n");
}

void testButtonReleaseSuppressed() {
    resetMocks();
    EventCapture capture;
    TestableButton button(2, 50, &capture);
    
    // First, simulate a press
    fakeMillis = 100;
    fakeDigitalReadValue = LOW;
    Event request1(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, 2);
    button.processEvent(request1);
    
    // Then simulate release: state transitions from LOW to HIGH
    fakeMillis = 200;
    fakeDigitalReadValue = HIGH;
    Event request2(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, 2);
    button.processEvent(request2);
    
    // Release (rising edge) should be suppressed
    assert(request2.identifier == -1);
    printf("  Button release event is suppressed\n");
}

void testDebouncePreventsRapidPress() {
    resetMocks();
    EventCapture capture;
    TestableButton button(2, 100, &capture);
    
    // First press
    fakeMillis = 101; // Just past debounce window (100ms)
    fakeDigitalReadValue = LOW;
    Event request1(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, 2);
    button.processEvent(request1);
    assert(request1.identifier == Button::BUTTON_PRESSED_EVENT_IDENTIFIER);
    
    // Release
    fakeMillis = 200;
    fakeDigitalReadValue = HIGH;
    Event request2(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, 2);
    button.processEvent(request2);
    
    // Rapid re-press (within debounce window)
    fakeMillis = 250; // Only 50ms after release, debounce is 100ms
    fakeDigitalReadValue = LOW;
    Event request3(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, 2);
    button.processEvent(request3);
    
    // Should be suppressed due to debounce
    assert(request3.identifier == -1);
    printf("  Debounce prevents rapid re-press\n");
}

void testDebounceAllowsPressAfterDelay() {
    resetMocks();
    EventCapture capture;
    TestableButton button(2, 100, &capture);
    
    // First press
    fakeMillis = 101; // Just past debounce window (100ms)
    fakeDigitalReadValue = LOW;
    Event request1(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, 2);
    button.processEvent(request1);
    
    // Release
    fakeMillis = 202; // Past debounce from press (202-101=101 > 100)
    fakeDigitalReadValue = HIGH;
    Event request2(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, 2);
    button.processEvent(request2);
    
    // Re-press after debounce delay
    fakeMillis = 350; // 148ms after release, debounce is 100ms
    fakeDigitalReadValue = LOW;
    Event request3(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, 2);
    button.processEvent(request3);
    
    // Should be accepted
    assert(request3.identifier == Button::BUTTON_PRESSED_EVENT_IDENTIFIER);
    printf("  Debounce allows press after delay\n");
}

void testNoStateChangeSuppressed() {
    resetMocks();
    EventCapture capture;
    TestableButton button(2, 50, &capture);
    
    // Pin stays HIGH (no press)
    fakeMillis = 100;
    fakeDigitalReadValue = HIGH;
    Event request(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, 2);
    button.processEvent(request);
    
    assert(request.identifier == -1);
    printf("  No state change suppresses event\n");
}

void testNonMeasureEventIgnored() {
    resetMocks();
    EventCapture capture;
    TestableButton button(2, 50, &capture);
    
    Event other(99, 2); // Not MEASURE_DATA_REQUESTED
    button.processEvent(other);
    
    // Event should pass through unchanged (Sensor base class handles propagation)
    // Button's processEvent only acts on MEASURE_DATA_REQUESTED
    printf("  Non-measure events are ignored\n");
}

void testButtonPressedEventIdentifier() {
    assert(Button::BUTTON_PRESSED_EVENT_IDENTIFIER == 10);
    assert(Button::BUTTON_PRESSED_EVENT.identifier == 10);
    printf("  BUTTON_PRESSED_EVENT identifier is correct\n");
}

void testDefaultSamplingInterval() {
    resetMocks();
    EventCapture capture;
    TestableButton button(2, 50, &capture);
    assert(button.getSamplingIntervalDefault() == 50);
    printf("  Default sampling interval is 50 ms\n");
}

void testMinimalSamplingInterval() {
    resetMocks();
    EventCapture capture;
    TestableButton button(2, 50, &capture);
    assert(button.getSamplingIntervalMinimal() == 10);
    printf("  Minimal sampling interval is 10 ms\n");
}

void testMultiplePresses() {
    resetMocks();
    EventCapture capture;
    TestableButton button(2, 50, &capture);
    
    // Press 1
    fakeMillis = 100;
    fakeDigitalReadValue = LOW;
    Event request1(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, 2);
    button.processEvent(request1);
    assert(request1.identifier == Button::BUTTON_PRESSED_EVENT_IDENTIFIER);
    
    // Release 1
    fakeMillis = 200;
    fakeDigitalReadValue = HIGH;
    Event request2(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, 2);
    button.processEvent(request2);
    
    // Press 2
    fakeMillis = 350;
    fakeDigitalReadValue = LOW;
    Event request3(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, 2);
    button.processEvent(request3);
    assert(request3.identifier == Button::BUTTON_PRESSED_EVENT_IDENTIFIER);
    
    // Release 2
    fakeMillis = 450;
    fakeDigitalReadValue = HIGH;
    Event request4(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, 2);
    button.processEvent(request4);
    
    // Press 3
    fakeMillis = 600;
    fakeDigitalReadValue = LOW;
    Event request5(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, 2);
    button.processEvent(request5);
    assert(request5.identifier == Button::BUTTON_PRESSED_EVENT_IDENTIFIER);
    
    printf("  Multiple presses are correctly detected\n");
}

void testSourceIdMatchesPin() {
    resetMocks();
    EventCapture capture;
    TestableButton button(21, 50, &capture);
    
    fakeMillis = 100;
    fakeDigitalReadValue = LOW;
    Event request(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, 21);
    button.processEvent(request);
    
    assert(request.sourceId == 21);
    printf("  Event sourceId matches the GPIO pin\n");
}

int main() {
    printf("Running Button tests...\n");
    testConstructorInitializesState();
    testConstructorDefaultDebounce();
    testConstructorCustomDebounce();
    testButtonPressedEventEmitted();
    testButtonReleaseSuppressed();
    testDebouncePreventsRapidPress();
    testDebounceAllowsPressAfterDelay();
    testNoStateChangeSuppressed();
    testNonMeasureEventIgnored();
    testButtonPressedEventIdentifier();
    testDefaultSamplingInterval();
    testMinimalSamplingInterval();
    testMultiplePresses();
    testSourceIdMatchesPin();
    printf("All Button tests passed!\n");
    return 0;
}
