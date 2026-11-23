/**
 * @file TogglableLedDevice.ino
 * @brief Interactive pushbutton that toggles an LED on each press.
 *
 * A practical device for demonstrating interrupt-driven sensor operation: a physical
 * button press triggers a GPIO interrupt, the framework debounces the signal, and
 * dispatches a toggle command to an LED actuator — all without polling in `loop()`.
 *
 * @par Modest IoT Nano Framework aspects illustrated:
 * - `Device` base class with `initializeAsynchronousEngine()` for loop-less execution
 * - `Button` sensor with interrupt-driven operation via `appendSensorToTriggeringScheduler()`
 * - `Led` actuator with prebuilt `TOGGLE_LED_COMMAND`
 * - Event routing via `on(Event)` handler with `BUTTON_PRESSED_EVENT_IDENTIFIER`
 * - Direct actuator command dispatch via `handle(Command)`
 * - Debounce timing configured per-button instance
 * @date 2026-06-12
 * @version 1.0
 */

#include <Arduino.h>
#include <ModestIoT.h>

/** @brief Serial communication baud rate. */
static const unsigned long SERIAL_BAUD_RATE = 115200;

/** @brief Delay in the main loop when no work is pending. */
static const unsigned long MAIN_LOOP_DELAY_IN_MILLISECONDS = 60000;

/** @name Serial Output Configuration */
//@{
/** @brief Log prefix for all TogglableLedDevice serial output. */
static const char* TOGGLABLE_LED_DEVICE_LOG_PREFIX = "[TogglableLedDevice]";
//@} // Serial Output Configuration

/** @name Application Mediator Implementation */
//@{

class TogglableLedDevice : public Device {
    private:
    Button toggleLedButton; // Purpose-driven semantic member name
    Led statusLed;          // Purpose-driven semantic member name

    public:
    /**
     * @param buttonPin GPIO target connected to the physical pushbutton.
     * @param ledPin GPIO target connected to the physical status LED.
     * @param timerChannel Dedicated hardware 64-bit timer group index (0-3).
     */
    TogglableLedDevice(int buttonPin, int ledPin, uint8_t timerChannel = 0)
    : Device(50, timerChannel), // Setup polling intervals at a safe 50ms task tick
    toggleLedButton(buttonPin, 200, this), // Pass pin, 200ms debounce window, and parent context
    statusLed(ledPin, false, this)              // Pass pin, false initial active state, and parent context
    {
        // 1. Register the button for interrupt-driven operation (FALLING edge).
        // The GPIO interrupt fires on button press, and the sampler task handles
        // debouncing via processEvent(). This eliminates polling latency.
        // IMPORTANT: Registration must occur before initializeAsynchronousEngine(), because
        // the engine start guard rejects any registration after the engine is running.
        appendSensorToTriggeringScheduler(&toggleLedButton,
                                          Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER,
                                          FALLING);

        // 2. Launch internal background task workers with a safe queue boundary depth of 5
        initializeAsynchronousEngine(5);
    }

    /**
     * @brief Inversion of Control interface contract handler.
     * Intercepts framework events dispatched from the background worker tasks.
     * @param event The triggered event context payload.
     * @runs_on{sampler}
     */
    void on(Event event) override {
            // Intercept and evaluate the specific pre-defined button event token
        if (event.identifier == Button::BUTTON_PRESSED_EVENT_IDENTIFIER) {

            // Route the pre-instantiated static toggle command directly to the actuator.
            // The mediator maintains no local state tracking variables or toggles!
            statusLed.handle(Led::TOGGLE_LED_COMMAND);

            Serial.println((String(TOGGLABLE_LED_DEVICE_LOG_PREFIX) + " Toggle LED Button press event caught -> LED toggle dispatched.").c_str());
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

static TogglableLedDevice* togglableLedApplication = nullptr;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    Device::setFirmwareVersion("TLD-110 FW 1.0.0");

    // Boot the framework application.
    // Background execution queues, FreeRTOS tasks, and hardware alarm vectors launch automatically!
    togglableLedApplication = new TogglableLedDevice(TOGGLE_BUTTON_PIN, STATUS_LED_PIN, CORE_TIMER_CHANNEL);

    Serial.println((String(TOGGLABLE_LED_DEVICE_LOG_PREFIX) + " Loop-less interactive toggle node active.").c_str());
}

void loop() {
    // The master loop is completely clean and empty.
    // The framework handles all timing, MQTT communication, and LED control asynchronously.
    // vTaskDelay clarifies that this loop yields to FreeRTOS — the device is loopless.
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}
//@} // Top-Level Declarative Canvas