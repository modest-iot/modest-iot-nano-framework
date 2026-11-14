/**
 * @file ArduinoFake.cpp
 * @brief Definitions for extern mock variables used by the fake Arduino.h.
 * @date 2026-11-13
 * @version 1.0
 */

#include "Arduino.h"

// Mock timing and clock simulation
unsigned long fakeMillis = 0;
unsigned long fakeMicros = 0;

// Mock ESP32 high-resolution timer (nanoseconds since boot)
int64_t fakeTimerNanoseconds = 0;

// Mock GPIO pins and digital I/O
int fakeDigitalWrite[64] = {};
int fakeDigitalReadValue = HIGH;
PinModeCallRecord fakePinModeCalls[64] = {};
int fakePinModeCallCount = 0;

// Mock analog input
int fakeAnalogRead[64] = {};

// Mock PWM analog output
int fakePwm[64] = {};

// Mock tone/noTone for buzzer testing
ToneCallRecord fakeToneCall = {};
bool fakeToneCalled = false;
int fakeNoTonePin = 0;
bool fakeNoToneCalled = false;

// Mock ESP32 LEDC PWM API for host testing
int fakeLedcFreq[32] = {};
int fakeLedcResolution[32] = {};
int fakeLedcPin[32] = {};
int fakeLedcDuty[32] = {};

// Mock ESP32 deep sleep API for host testing
esp_sleep_wakeup_cause_t fakeWakeupCause = ESP_SLEEP_WAKEUP_OTHER;

// Mock attachInterrupt state
void (*fakeInterruptHandlers[64])(void) = {};
void (*fakeInterruptHandlersArg[64])(void*) = {};
void* fakeInterruptArgs[64] = {};
int fakeInterruptModes[64] = {};
