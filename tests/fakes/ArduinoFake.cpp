/**
 * @file ArduinoFake.cpp
 * @brief Definitions for extern mock variables used by the fake Arduino.h.
 * @date 2026-11-13
 * @version 1.0
 */

#include "Arduino.h"

FakeFreeRtosState fakeFreeRtosState = {};
FakeQueueHandle fakeQueueHandles[64] = {};
FakeTaskHandle fakeTaskHandles[64] = {};
thread_local FakeTaskHandle* currentFakeTaskHandle = nullptr;
std::mutex fakeTaskStateMutex;
hw_timer_s fakeTimerObjects[64] = {};
hw_timer_t fakeTimerHandles[64] = {};

void joinFakeTasks() {
    for (auto& handle : fakeTaskHandles) {
        if (handle.worker.joinable()) {
            handle.worker.join();
        }
    }
}

void resetFakeFreeRtosState() {
    joinFakeTasks();
    fakeFreeRtosState = {};
    fakeFreeRtosState.failQueueCreateAtCall = -1;
    fakeFreeRtosState.failTaskCreateAtCall = -1;
    for (auto& handle : fakeQueueHandles) {
        handle.active = false;
    }
    for (auto& handle : fakeTaskHandles) {
        handle.active.store(false);
        std::lock_guard<std::mutex> lock(handle.notificationMutex);
        handle.notificationValue = 0;
    }
    for (std::size_t index = 0; index < std::size(fakeTimerObjects); ++index) {
        fakeTimerObjects[index].active = false;
        fakeTimerHandles[index] = nullptr;
    }
}

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
bool fakeLedcAttachResult = true;
bool fakeLedcWriteResult = true;
int fakeLedcDetachCalls = 0;

// Mock ESP32 deep sleep API for host testing
esp_sleep_wakeup_cause_t fakeWakeupCause = ESP_SLEEP_WAKEUP_OTHER;

// Mock attachInterrupt state
void (*fakeInterruptHandlers[64])(void) = {};
void (*fakeInterruptHandlersArg[64])(void*) = {};
void* fakeInterruptArgs[64] = {};
int fakeInterruptModes[64] = {};
