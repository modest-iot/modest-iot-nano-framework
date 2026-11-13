#ifndef FAKE_ARDUINO_H
#define FAKE_ARDUINO_H

#include <time.h>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <string>

typedef std::uint8_t byte;
typedef bool boolean;

// Mock FreeRTOS types for host testing
typedef std::int32_t BaseType_t;
typedef std::int32_t UBaseType_t;
typedef void* TaskHandle_t;
typedef void* QueueHandle_t;
typedef std::uint32_t TickType_t;
#define pdTRUE ((BaseType_t)1)
#define pdFALSE ((BaseType_t)0)
#define pdPASS ((BaseType_t)1)
#define pdFAIL ((BaseType_t)0)
#define portMAX_DELAY ((TickType_t)0xFFFFFFFFUL)
#define portYIELD_FROM_ISR(...) ((void)0)
#define eSetValueWithOverwrite 1
#define eSetBits 2
#define IRAM_ATTR
struct hw_timer_s;
typedef hw_timer_s* hw_timer_t;

// Mock FreeRTOS task/queue APIs (no-ops for host testing)
inline TaskHandle_t xTaskCreateStatic(void*, const char*, std::uint32_t, void*, std::uint8_t, TaskHandle_t*) { return nullptr; }
inline void xTaskCreatePinnedToCore(void (*)(void*), const char*, std::uint32_t, void*, std::uint8_t, TaskHandle_t*, BaseType_t) {}
inline void vTaskDelete(TaskHandle_t) {}
inline void vTaskDelay(TickType_t) {}
inline uint32_t ulTaskNotifyTake(BaseType_t, TickType_t) { return 0; }
inline BaseType_t xTaskNotifyGive(TaskHandle_t) { return pdPASS; }
inline void vTaskNotifyGive(TaskHandle_t) {}
inline BaseType_t xTaskNotify(TaskHandle_t, uint32_t, int) { return pdPASS; }
inline BaseType_t xTaskNotifyFromISR(TaskHandle_t, uint32_t, int, BaseType_t*) { return pdPASS; }
inline BaseType_t xTaskNotifyWait(BaseType_t, BaseType_t, uint32_t*, TickType_t) { return pdFALSE; }
inline QueueHandle_t xQueueCreate(std::uint32_t, std::uint32_t) { return nullptr; }
inline BaseType_t xQueueSend(QueueHandle_t, const void*, TickType_t) { return pdFAIL; }
inline BaseType_t xQueueReceive(QueueHandle_t, void*, TickType_t) { return pdFAIL; }
inline void vQueueDelete(QueueHandle_t) {}
inline UBaseType_t uxQueueMessagesWaiting(QueueHandle_t) { return 0; }
inline UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t) { return 0; }
inline TickType_t pdMS_TO_TICKS(std::uint32_t ms) { return ms; }

// Mock ESP32 heap diagnostics (matches real EspClass from Esp.h)
class EspClass {
public:
    std::uint32_t getFreeHeap() { return 100000; }
    std::uint32_t getMinFreeHeap() { return 50000; }
};
inline EspClass ESP;

// Mock timer APIs
inline hw_timer_t* timerBegin(std::uint32_t) { return nullptr; }
inline void timerEnd(hw_timer_t*) {}
inline void timerAttachInterruptArg(hw_timer_t*, void (*)(void*), void*) {}
inline void timerDetachInterrupt(hw_timer_t*) {}
inline void timerAlarm(hw_timer_t*, std::uint64_t, BaseType_t, std::uint32_t) {}

class String : public std::string {
public:
    using std::string::string;
    String(const std::string& s) : std::string(s) {}
    String(std::string&& s) noexcept : std::string(std::move(s)) {}
    String& operator=(const std::string& s) {
        std::string::operator=(s);
        return *this;
    }
    String& operator=(std::string&& s) noexcept {
        std::string::operator=(std::move(s));
        return *this;
    }
};

#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#define INPUT_PULLDOWN 3
#define CHANGE 1
#define RISING 2
#define FALLING 3

// Mock GPIO interrupt APIs
using InterruptHandler = void(*)();
using InterruptHandlerArg = void(*)(void*);
inline InterruptHandler fakeInterruptHandlers[64] = {};
inline InterruptHandlerArg fakeInterruptHandlersArg[64] = {};
inline void* fakeInterruptArgs[64] = {};
inline int fakeInterruptModes[64] = {};
inline int digitalPinToInterrupt(const std::uint8_t pin) { return pin; }
inline void attachInterrupt(int pin, InterruptHandler fn, int mode) {
    if (pin >= 0 && pin < 64) {
        fakeInterruptHandlers[pin] = fn;
        fakeInterruptModes[pin] = mode;
    }
}
inline void attachInterruptArg(int pin, InterruptHandlerArg fn, void* arg, int mode) {
    if (pin >= 0 && pin < 64) {
        fakeInterruptHandlersArg[pin] = fn;
        fakeInterruptArgs[pin] = arg;
        fakeInterruptModes[pin] = mode;
    }
}
inline void detachInterrupt(int pin) {
    if (pin >= 0 && pin < 64) {
        fakeInterruptHandlers[pin] = nullptr;
        fakeInterruptHandlersArg[pin] = nullptr;
        fakeInterruptArgs[pin] = nullptr;
        fakeInterruptModes[pin] = 0;
    }
}

// Mock timing and clock simulation
inline unsigned long fakeMillis = 0;
inline unsigned long fakeMicros = 0;
inline unsigned long millis() { return fakeMillis; }
inline unsigned long micros() { return fakeMicros; }
inline void delay(const unsigned long ms) { fakeMillis += ms; }
inline void delayMicroseconds(const unsigned int us) { fakeMicros += us; }

// Mock ESP32 high-resolution timer (nanoseconds since boot)
inline int64_t fakeTimerNanoseconds = 0;
inline int64_t esp_timer_get_time() { return fakeTimerNanoseconds / 1000; } // Returns microseconds

// Mock pulseIn (for backward compatibility)
inline unsigned long pulseIn(int pin, int state, unsigned long timeout = 0) {
    (void)pin; (void)state; (void)timeout;
    return 0;
}

// Mock GPIO pins and digital I/O
inline int fakeDigitalWrite[64] = {};
inline int fakeDigitalReadValue = HIGH;
inline struct { std::uint8_t pin; std::uint8_t mode; } fakePinModeCalls[64] = {};
inline int fakePinModeCallCount = 0;
inline void pinMode(const std::uint8_t pin, const std::uint8_t mode) {
    if (fakePinModeCallCount < 64) {
        fakePinModeCalls[fakePinModeCallCount].pin = pin;
        fakePinModeCalls[fakePinModeCallCount].mode = mode;
        fakePinModeCallCount++;
    }
}
inline void digitalWrite(const std::uint8_t pin, const std::uint8_t val) {
    if (pin < std::size(fakeDigitalWrite)) {
        fakeDigitalWrite[pin] = val;
    }
}
inline int digitalRead(const std::uint8_t pin) { (void)pin; return fakeDigitalReadValue; }
inline void yield() {}

// Mock analog input
inline int fakeAnalogRead[64] = {};
inline int analogRead(const std::uint8_t pin) {
    if (pin < std::size(fakeAnalogRead)) {
        return fakeAnalogRead[pin];
    }
    return 0;
}

// Mock PWM analog output
inline int fakePwm[64] = {};
inline void analogWrite(const std::uint8_t pin, const int val) {
    if (pin < std::size(fakePwm)) {
        fakePwm[pin] = val;
    }
}

// Mock tone/noTone for buzzer testing
struct ToneCallRecord { int pin; int frequency; unsigned long duration; };
extern ToneCallRecord fakeToneCall;
extern bool fakeToneCalled;
extern int fakeNoTonePin;
extern bool fakeNoToneCalled;
inline void tone(int pin, int frequency, unsigned long duration = 0) {
    fakeToneCall.pin = pin;
    fakeToneCall.frequency = frequency;
    fakeToneCall.duration = duration;
    fakeToneCalled = true;
}
inline void noTone(int pin) {
    fakeNoTonePin = pin;
    fakeNoToneCalled = true;
}

// Mock ESP32 LEDC PWM API for host testing (W19, core 3.x)
inline int fakeLedcFreq[32] = {};
inline int fakeLedcResolution[32] = {};
inline int fakeLedcPin[32] = {};
inline int fakeLedcDuty[32] = {};
inline bool ledcAttach(uint8_t pin, uint32_t freq, uint8_t resolution) {
    fakeLedcFreq[pin] = freq;
    fakeLedcResolution[pin] = resolution;
    fakeLedcPin[pin] = pin;
    return true;
}
inline void ledcWrite(uint8_t pin, int duty) {
    fakeLedcDuty[pin] = duty;
}
inline bool ledcDetach(uint8_t pin) {
    (void)pin;
    return true;
}

// Mock ESP32 deep sleep API for host testing (W17)
#define RTC_DATA_ATTR
enum esp_sleep_wakeup_cause_t {
    ESP_SLEEP_WAKEUP_EXT0,
    ESP_SLEEP_WAKEUP_EXT1,
    ESP_SLEEP_WAKEUP_TIMER,
    ESP_SLEEP_WAKEUP_TOUCHPAD,
    ESP_SLEEP_WAKEUP_ULP,
    ESP_SLEEP_WAKEUP_OTHER
};
inline esp_sleep_wakeup_cause_t fakeWakeupCause = ESP_SLEEP_WAKEUP_OTHER;
inline void esp_deep_sleep_start() {}
inline void esp_sleep_enable_timer_wakeup(uint64_t time_in_us) { (void)time_in_us; }
inline esp_sleep_wakeup_cause_t esp_sleep_get_wakeup_cause() { return fakeWakeupCause; }

#endif // FAKE_ARDUINO_H
