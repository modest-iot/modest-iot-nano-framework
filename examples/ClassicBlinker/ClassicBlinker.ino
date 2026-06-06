/**
 * @file ClassicBlinker.ino
 * @brief Official entry example for the Modest-IoT Nano-Framework.
 * Demonstrates a loop-less, event-driven asynchronous LED toggle using
 * framework Event handlers and class-encapsulated Actuator Commands.
 */

#include <Arduino.h>
#include <ModestIoT.h>

// --- 1. EVENT-DRIVEN APPLICATION MEDIATOR ---

class ClassicBlinker : public Device {
    private:
    Led statusLed;

    public:
    // Define a localized, explicit event identifier for the hardware timer tick
    static const int TIMER_EVENT_IDENTIFIER = 300;

    /**
     * @param ledPin Target GPIO pin where the physical LED is connected.
     * @param blinkIntervalMs Ticking frequency rate.
     * @param timerChannel Dedicated hardware 64-bit timer group index (0-3).
     */
    ClassicBlinker(int ledPin, unsigned long blinkIntervalMs, uint8_t timerChannel = 0)
    : Device(blinkIntervalMs, timerChannel),
    statusLed(ledPin, false, this) // Pass pin, initial active state (false), and set parent handler
{
    // 1. Launch internal background task workers with a safe queue boundary depth of 5
    initializeAsynchronousEngine(5);

    // 2. Register this device context to receive automated clock alarm notifications
    // mapped directly to our clean event identity token
    appendSensorToScheduler(this, TIMER_EVENT_IDENTIFIER);
}

    /**
     * @brief Inversion of Control interface contract handler.
     * Intercepts framework events dispatched from the background worker tasks.
     * @param event The triggered event context payload.
     */
    void on(Event event) override {
    // Handle the explicit timing event assigned to this device
    if (event.identifier == TIMER_EVENT_IDENTIFIER) {

    // Route the pre-instantiated static toggle command directly to the actuator.
    // No local state tracking variables or toggles are written here!
    statusLed.handle(Led::TOGGLE_LED_COMMAND);
}
}
};

// --- 2. DECLARATIVE RUNTIME CANVAS ---

// Target the ESP32 onboard diagnostic blue LED pin (GPIO 2)
static const int SYSTEM_DIAGNOSTIC_LED_PIN = 2;
static const unsigned long BLINK_RATE_MS = 500;
static const uint8_t CORE_TIMER_CHANNEL = 0;

static ClassicBlinker* blinkerApplication = nullptr;

void setup() {
    Serial.begin(115200);

    // Boot the framework application.
    // Background execution queues, FreeRTOS tasks, and hardware alarm vectors launch automatically!
    blinkerApplication = new ClassicBlinker(SYSTEM_DIAGNOSTIC_LED_PIN, BLINK_RATE_MS, CORE_TIMER_CHANNEL);

    Serial.println("[Blinker Node] Loop-less, event-driven command routing active.");
}

void loop() {
    // The master loop is completely clean and empty.
    // It yields 100% of its execution cycles natively back to the FreeRTOS kernel scheduler.
    vTaskDelay(pdMS_TO_TICKS(60000));
}