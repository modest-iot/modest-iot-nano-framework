#ifndef FAKE_ARDUINO_H
#define FAKE_ARDUINO_H

#include <time.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <mutex>
#include <string>
#include <thread>

typedef std::uint8_t byte;
typedef bool boolean;

// Mock FreeRTOS types for host testing
typedef std::int32_t BaseType_t;
typedef std::int32_t UBaseType_t;
typedef void* TaskHandle_t;
typedef void* QueueHandle_t;
typedef std::uint32_t TickType_t;
struct FakeQueueHandle { bool active = false; };
struct FakeTaskHandle {
    std::atomic<bool> active{false};
    std::thread worker;
    std::mutex notificationMutex;
    std::condition_variable notificationCondition;
    std::uint32_t notificationValue = 0;
};
struct FakeFreeRtosState {
    int queueCreateCalls = 0;
    int queueDeleteCalls = 0;
    int taskCreateCalls = 0;
    int taskDeleteCalls = 0;
    int timerBeginCalls = 0;
    int timerEndCalls = 0;
    int timerAttachCalls = 0;
    int timerAlarmCalls = 0;
    bool lastTimerAlarmAutoReload = false;
    int isrNotificationCalls = 0;
    TaskHandle_t isrNotificationTargets[64] = {};
    int failQueueCreateAtCall = -1;
    int failTaskCreateAtCall = -1;
    bool failTimerBegin = false;
};
extern FakeFreeRtosState fakeFreeRtosState;
extern FakeQueueHandle fakeQueueHandles[64];
extern FakeTaskHandle fakeTaskHandles[64];
extern thread_local FakeTaskHandle* currentFakeTaskHandle;
extern std::mutex fakeTaskStateMutex;
void resetFakeFreeRtosState();
void joinFakeTasks();

#define pdTRUE ((BaseType_t)1)
#define pdFALSE ((BaseType_t)0)
#define pdPASS ((BaseType_t)1)
#define pdFAIL ((BaseType_t)0)
#define portMAX_DELAY ((TickType_t)0xFFFFFFFFUL)
#define portYIELD_FROM_ISR(...) ((void)0)
#define eSetValueWithOverwrite 1
#define eSetBits 2
#define IRAM_ATTR
struct hw_timer_s { bool active = false; };
typedef hw_timer_s* hw_timer_t;
extern hw_timer_s fakeTimerObjects[64];
extern hw_timer_t fakeTimerHandles[64];

// Mock FreeRTOS task/queue APIs
inline TaskHandle_t xTaskCreateStatic(void*, const char*, std::uint32_t, void*, std::uint8_t, TaskHandle_t*) { return nullptr; }
inline BaseType_t xTaskCreatePinnedToCore(void (*taskFunction)(void*), const char* name,
                                          std::uint32_t stackSize, void* taskParameters,
                                          std::uint8_t priority, TaskHandle_t* taskHandle,
                                          BaseType_t core) {
    (void)name;
    (void)stackSize;
    (void)taskParameters;
    (void)priority;
    (void)core;
    const int call = ++fakeFreeRtosState.taskCreateCalls;
    if (call == fakeFreeRtosState.failTaskCreateAtCall) {
        if (taskHandle != nullptr) {
            *taskHandle = nullptr;
        }
        return pdFAIL;
    }
    for (auto& handle : fakeTaskHandles) {
        if (!handle.active.load() && !handle.worker.joinable()) {
            auto* taskHandlePointer = &handle;
            {
                std::lock_guard<std::mutex> lock(handle.notificationMutex);
                handle.notificationValue = 0;
            }
            handle.active.store(true);
            if (taskHandle != nullptr) {
                *taskHandle = &handle;
            }
            try {
                handle.worker = std::thread([taskHandlePointer, taskFunction, taskParameters]() {
                    currentFakeTaskHandle = taskHandlePointer;
                    taskFunction(taskParameters);
                    taskHandlePointer->active.store(false);
                    currentFakeTaskHandle = nullptr;
                });
            } catch (...) {
                handle.active.store(false);
                if (taskHandle != nullptr) {
                    *taskHandle = nullptr;
                }
                return pdFAIL;
            }
            return pdPASS;
        }
    }
    return pdFAIL;
}
inline void vTaskDelete(TaskHandle_t task) {
    auto* handle = task == nullptr ? currentFakeTaskHandle : static_cast<FakeTaskHandle*>(task);
    if (handle == nullptr) {
        return;
    }
    if (handle->active.exchange(false)) {
        std::lock_guard<std::mutex> lock(fakeTaskStateMutex);
        ++fakeFreeRtosState.taskDeleteCalls;
    }
}
inline void vTaskDelay(TickType_t ticks) {
    if (ticks == 0) {
        std::this_thread::yield();
    } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(ticks));
    }
}
inline uint32_t ulTaskNotifyTake(BaseType_t, TickType_t) { return 0; }
inline BaseType_t xTaskNotifyGive(TaskHandle_t) { return pdPASS; }
inline void vTaskNotifyGive(TaskHandle_t) {}
inline BaseType_t fakeNotifyTask(TaskHandle_t task, uint32_t value) {
    if (task == nullptr) {
        return pdFAIL;
    }
    auto* handle = static_cast<FakeTaskHandle*>(task);
    {
        std::lock_guard<std::mutex> lock(handle->notificationMutex);
        handle->notificationValue |= value;
    }
    handle->notificationCondition.notify_one();
    return pdPASS;
}
inline BaseType_t xTaskNotify(TaskHandle_t task, uint32_t value, int) {
    return fakeNotifyTask(task, value);
}
inline BaseType_t xTaskNotifyFromISR(TaskHandle_t task, uint32_t value, int, BaseType_t*) {
    const int call = fakeFreeRtosState.isrNotificationCalls++;
    if (call < static_cast<int>(std::size(fakeFreeRtosState.isrNotificationTargets))) {
        fakeFreeRtosState.isrNotificationTargets[call] = task;
    }
    return fakeNotifyTask(task, value);
}
inline BaseType_t xTaskNotifyWait(BaseType_t clearOnEntry, BaseType_t clearOnExit,
                                  uint32_t* notificationValue, TickType_t ticksToWait) {
    auto* handle = currentFakeTaskHandle;
    if (handle == nullptr) {
        return pdFALSE;
    }
    std::unique_lock<std::mutex> lock(handle->notificationMutex);
    handle->notificationValue &= ~static_cast<uint32_t>(clearOnEntry);
    if (handle->notificationValue == 0 && ticksToWait != 0) {
        const auto notificationAvailable = [&handle]() { return handle->notificationValue != 0; };
        if (ticksToWait == portMAX_DELAY) {
            handle->notificationCondition.wait(lock, notificationAvailable);
        } else {
            handle->notificationCondition.wait_for(lock, std::chrono::milliseconds(ticksToWait),
                                                   notificationAvailable);
        }
    }
    if (handle->notificationValue == 0) {
        return pdFALSE;
    }
    if (notificationValue != nullptr) {
        *notificationValue = handle->notificationValue;
    }
    handle->notificationValue &= ~static_cast<uint32_t>(clearOnExit);
    return pdTRUE;
}
inline QueueHandle_t xQueueCreate(std::uint32_t queueLength, std::uint32_t itemSize) {
    (void)itemSize;
    const int call = ++fakeFreeRtosState.queueCreateCalls;
    if (queueLength == 0 || call == fakeFreeRtosState.failQueueCreateAtCall) {
        return nullptr;
    }
    for (auto& handle : fakeQueueHandles) {
        if (!handle.active) {
            handle.active = true;
            return &handle;
        }
    }
    return nullptr;
}
inline BaseType_t xQueueSend(QueueHandle_t, const void*, TickType_t) { return pdFAIL; }
inline BaseType_t xQueueReceive(QueueHandle_t, void*, TickType_t) { return pdFAIL; }
inline void vQueueDelete(QueueHandle_t queue) {
    if (queue == nullptr) {
        return;
    }
    auto* handle = static_cast<FakeQueueHandle*>(queue);
    if (handle->active) {
        handle->active = false;
        ++fakeFreeRtosState.queueDeleteCalls;
    }
}
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
inline hw_timer_t* timerBegin(std::uint32_t frequency) {
    (void)frequency;
    ++fakeFreeRtosState.timerBeginCalls;
    if (fakeFreeRtosState.failTimerBegin) {
        return nullptr;
    }
    for (std::size_t index = 0; index < std::size(fakeTimerObjects); ++index) {
        if (!fakeTimerObjects[index].active) {
            fakeTimerObjects[index].active = true;
            fakeTimerHandles[index] = &fakeTimerObjects[index];
            return &fakeTimerHandles[index];
        }
    }
    return nullptr;
}
inline void timerEnd(hw_timer_t* timer) {
    if (timer == nullptr || *timer == nullptr) {
        return;
    }
    (*timer)->active = false;
    *timer = nullptr;
    ++fakeFreeRtosState.timerEndCalls;
}
inline void timerAttachInterruptArg(hw_timer_t* timer, void (*handler)(void*), void* argument) {
    (void)handler;
    (void)argument;
    if (timer != nullptr) {
        ++fakeFreeRtosState.timerAttachCalls;
    }
}
inline void timerDetachInterrupt(hw_timer_t*) {}
inline void timerAlarm(hw_timer_t* timer, std::uint64_t alarm, BaseType_t autoReload,
                       std::uint32_t reloadCount) {
    (void)alarm;
    (void)autoReload;
    (void)reloadCount;
    if (timer != nullptr) {
        ++fakeFreeRtosState.timerAlarmCalls;
        fakeFreeRtosState.lastTimerAlarmAutoReload = autoReload != pdFALSE;
    }
}

class String : public std::string {
public:
    using std::string::string;
    String(const std::string& s) : std::string(s) {}
    String(std::string&& s) noexcept : std::string(std::move(s)) {}
    bool isEmpty() const { return empty(); }
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
#define SERIAL_8N1 0x00

// Mock GPIO interrupt APIs
using InterruptHandler = void(*)();
using InterruptHandlerArg = void(*)(void*);
extern InterruptHandler fakeInterruptHandlers[64];
extern InterruptHandlerArg fakeInterruptHandlersArg[64];
extern void* fakeInterruptArgs[64];
extern int fakeInterruptModes[64];
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
extern unsigned long fakeMillis;
extern unsigned long fakeMicros;
inline unsigned long millis() { return fakeMillis; }
inline unsigned long micros() { return fakeMicros; }
inline void delay(const unsigned long ms) { fakeMillis += ms; }
inline void delayMicroseconds(const unsigned int us) { fakeMicros += us; }

// Mock ESP32 high-resolution timer (nanoseconds since boot)
extern int64_t fakeTimerNanoseconds;
inline int64_t esp_timer_get_time() { return fakeTimerNanoseconds / 1000; } // Returns microseconds

// Mock pulseIn (for backward compatibility)
inline unsigned long pulseIn(int pin, int state, unsigned long timeout = 0) {
    (void)pin; (void)state; (void)timeout;
    return 0;
}

// Mock GPIO pins and digital I/O
struct PinModeCallRecord { std::uint8_t pin; std::uint8_t mode; };
extern int fakeDigitalWrite[64];
extern int fakeDigitalReadValue;
extern PinModeCallRecord fakePinModeCalls[64];
extern int fakePinModeCallCount;
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
extern int fakeAnalogRead[64];
inline int analogRead(const std::uint8_t pin) {
    if (pin < std::size(fakeAnalogRead)) {
        return fakeAnalogRead[pin];
    }
    return 0;
}

// Mock PWM analog output
extern int fakePwm[64];
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
extern int fakeLedcFreq[32];
extern int fakeLedcResolution[32];
extern int fakeLedcPin[32];
extern int fakeLedcDuty[32];
extern bool fakeLedcAttachResult;
extern bool fakeLedcWriteResult;
extern int fakeLedcDetachCalls;
inline bool ledcAttach(uint8_t pin, uint32_t freq, uint8_t resolution) {
    fakeLedcFreq[pin] = freq;
    fakeLedcResolution[pin] = resolution;
    fakeLedcPin[pin] = pin;
    return fakeLedcAttachResult;
}
inline bool ledcWrite(uint8_t pin, int duty) {
    if (fakeLedcWriteResult) {
        fakeLedcDuty[pin] = duty;
    }
    return fakeLedcWriteResult;
}
inline bool ledcDetach(uint8_t pin) {
    (void)pin;
    ++fakeLedcDetachCalls;
    return true;
}

// Fake HardwareSerial for GPS and other serial sensor testing
class HardwareSerial {
public:
    char buffer[256]{};
    int bufferSize = 0;
    int readIndex = 0;

    void begin(long baud, int config = SERIAL_8N1, int8_t rxPin = -1, int8_t txPin = -1, bool invert = false) {
        (void)baud; (void)config; (void)rxPin; (void)txPin; (void)invert;
    }
    void end() { bufferSize = 0; readIndex = 0; }
    int available() { return bufferSize - readIndex; }
    int read() {
        if (readIndex < bufferSize) return (unsigned char)buffer[readIndex++];
        return -1;
    }
    int peek() {
        if (readIndex < bufferSize) return (unsigned char)buffer[readIndex];
        return -1;
    }
    void flush() { readIndex = bufferSize; }
    size_t write(uint8_t) { return 1; }
    size_t write(const uint8_t* data, size_t len) { (void)data; return len; }
    void feed(const char* data) {
        bufferSize = strlen(data);
        strncpy(buffer, data, sizeof(buffer) - 1);
        buffer[bufferSize] = '\0';
        readIndex = 0;
    }
    void clear() { bufferSize = 0; readIndex = 0; }
};

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
extern esp_sleep_wakeup_cause_t fakeWakeupCause;
inline void esp_deep_sleep_start() {}
inline void esp_sleep_enable_timer_wakeup(uint64_t time_in_us) { (void)time_in_us; }
inline esp_sleep_wakeup_cause_t esp_sleep_get_wakeup_cause() { return fakeWakeupCause; }

#endif // FAKE_ARDUINO_H
