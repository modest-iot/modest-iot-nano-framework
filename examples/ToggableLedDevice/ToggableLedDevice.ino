/**
 * @file ToggableLedDevice.ino
 * @brief Modern replacement example inspired by the legacy Wokwi prototype.
 * Demonstrates a loop-less, event-driven hardware button press triggering an LED toggle
 * using framework Event handlers and class-encapsulated Actuator Commands.
 * @date 2026-06-12
 * @version 1.0
 */

#include <Arduino.h>
#include <ModestIoT.h>

/** @brief Serial communication baud rate. */
static const unsigned long SERIAL_BAUD_RATE = 115200;

/** @brief Delay in the main loop when no work is pending. */
static const unsigned long MAIN_LOOP_DELAY_IN_MILLISECONDS = 60000;

/** @name Application Mediator Implementation */
//@{

class ToggableLedDevice : public Device {
    private:
    Button toggleLedButton; // Purpose-driven semantic member name
    Led statusLed;          // Purpose-driven semantic member name

    public:
    /**
     * @param buttonPin GPIO target connected to the physical pushbutton.
     * @param ledPin GPIO target connected to the physical status LED.
     * @param timerChannel Dedicated hardware 64-bit timer group index (0-3).
     */
    ToggableLedDevice(int buttonPin, int ledPin, uint8_t timerChannel = 0)
    : Device(50, timerChannel), // Setup polling intervals at a safe 50ms task tick
    toggleLedButton(buttonPin, 200, this), // Pass pin, 200ms debounce window, and parent context
    statusLed(ledPin, false, this)              // Pass pin, false initial active state, and parent context
    {
        // 1. Launch internal background task workers with a safe queue boundary depth of 5
        initializeAsynchronousEngine(5);

        // 2. Register the button for interrupt-driven operation (FALLING edge).
        // The GPIO interrupt fires on button press, and the sampler task handles
        // debouncing via processEvent(). This eliminates polling latency.
        appendSensorToTriggeringScheduler(&toggleLedButton,
                                          Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER,
                                          FALLING);
    }

    /**
     * @brief Inversion of Control interface contract handler.
     * Intercepts framework events dispatched from the background worker tasks.
     * @param event The triggered event context payload.
     */
    void on(Event event) override {
            // Intercept and evaluate the specific pre-defined button event token
        if (event.identifier == Button::BUTTON_PRESSED_EVENT_IDENTIFIER) {

            // Route the pre-instantiated static toggle command directly to the actuator.
            // The mediator maintains no local state tracking variables or toggles!
            statusLed.handle(Led::TOGGLE_LED_COMMAND);

            Serial.println("[ToggableLedDevice] Toggle LED Button press event caught -> LED toggle dispatched.");
        }
    }
};
//@} // Application Mediator Implementation

/** @name Top-Level Declarative Canvas */
//@{

// Clean configuration canvas constants aligned perfectly with member variable naming
static const int     TOGGLE_BUTTON_PIN  = 26; // Mapped pushbutton GPIO
static const int     STATUS_LED_PIN     = 27;  // Onboard ESP32 diagnostic blue LED
static const uint8_t CORE_TIMER_CHANNEL = 0;

static ToggableLedDevice* applicationNode = nullptr;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);

    // Boot the framework application.
    // Background execution queues, FreeRTOS tasks, and hardware alarm vectors launch automatically!
    applicationNode = new ToggableLedDevice(TOGGLE_BUTTON_PIN, STATUS_LED_PIN, CORE_TIMER_CHANNEL);

    Serial.println("[System Boot] Loop-less interactive toggle node active.");
}

void loop() {
    // 100% of execution resources are permanently yielded to background FreeRTOS workers
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}
//@} // Top-Level Declarative Canvas