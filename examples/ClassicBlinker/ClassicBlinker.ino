/**
 * @file ClassicBlinker.ino
 * @brief Official entry example for the Modest-IoT Nano-Framework.
 * Demonstrates a loop-less, event-driven asynchronous LED toggle using
 * framework Event handlers and class-encapsulated Actuator Commands.
 * @date 2026-06-06
 * @version 1.0
 */

#include <Arduino.h>
#include <ModestIoT.h>

// --- 1. EVENT-DRIVEN APPLICATION MEDIATOR ---

/**
 * @brief Implements the Classic Blinker application as an event-driven mediator.
 *
 * This class extends the `Device` base class from the Modest-IoT Nano-Framework
 * to manage an LED, toggling its state asynchronously based on timer events.
 * It demonstrates a loop-less, event-driven approach using framework Event handlers
 * and class-encapsulated Actuator Commands, avoiding traditional `delay()` calls
 * and direct state management within the main loop.
 */
class ClassicBlinker : public Device {
    private:
    /**
     * @brief The LED actuator controlled by this application.
     */
    Led statusLed;

    public:
    /**
     * @brief A localized, explicit event identifier for the hardware timer tick.
     *
     * This constant is used to uniquely identify timer events dispatched
     * to this `ClassicBlinker` instance, allowing for clear event routing.
     */
    static const int TIMER_EVENT_IDENTIFIER = 300;

    /**
     * @brief Constructs a new ClassicBlinker application instance.
     *
     * Initializes the device with a specified LED pin, blink interval,
     * and an optional hardware timer channel. It sets up the asynchronous
     * engine and registers itself to receive automated clock alarm notifications.
     *
     * @param ledPin Target GPIO pin where the physical LED is connected.
     * @param blinkIntervalMs Ticking frequency rate in milliseconds.
     * @param timerChannel Dedicated hardware 64-bit timer group index (0-3).
     *                     Defaults to 0 if not specified.
     */
    ClassicBlinker(int ledPin, unsigned long blinkIntervalMs, uint8_t timerChannel = 0)
    : Device(blinkIntervalMs, timerChannel),
    statusLed(ledPin, false, this) // Pass pin, initial active state (false), and set parent handler
    {
        // 1. Launch internal background task workers with a safe queue boundary depth of 5
        initializeAsynchronousEngine(5);

        // 2. Register this device context to receive automated clock alarm notifications
        // mapped directly to our clean event identity token
        // Capture and evaluate scheduler capacity bounds gracefully
        bool registered = appendSensorToScheduler(this, TIMER_EVENT_IDENTIFIER);
        if (!registered) {
            Serial.println(F("[ERROR] Blinker registration failed: Scheduler capacity full!"));
        }
    }

    /**
     * @brief Inversion of Control interface contract handler for incoming events.
     *
     * This method is called by the framework when an event is dispatched
     * from the background worker tasks. It checks for the specific timer event
     * and routes a toggle command directly to the `statusLed` actuator.
     * No local state tracking variables or toggles are written here,
     * demonstrating a truly event-driven command routing.
     *
     * @param event The triggered event context payload containing identifier and data.
     */
    void on(Event event) override {
        // Handle the explicit timing event assigned to this device
        if (event.identifier == TIMER_EVENT_IDENTIFIER) {

            // Route the pre-instantiated static toggle command directly to the actuator.
            // No local state tracking variables or toggles are written here!
            statusLed.handle(Led::TOGGLE_LED_COMMAND);
            Serial.println("[ClassicBlinker] Timer event caught -> LED toggle dispatched.");
        }
    }
};

// --- 2. DECLARATIVE RUNTIME CANVAS ---

/**
 * @brief Defines the GPIO pin for the system diagnostic LED.
 *
 * This typically targets the ESP32 onboard diagnostic blue LED pin (GPIO 2).
 */
static const int SYSTEM_DIAGNOSTIC_LED_PIN = 2;

/**
 * @brief Defines the blink rate for the LED in milliseconds.
 */
static const unsigned long BLINK_RATE_MS = 500;

/**
 * @brief Defines the hardware core timer channel to be used.
 *
 * This specifies which 64-bit timer group (0-3) will be utilized for timing events.
 */
static const uint8_t CORE_TIMER_CHANNEL = 0;

/**
 * @brief Pointer to the ClassicBlinker application instance.
 *
 * This global pointer holds the single instance of our event-driven blinker application.
 */
static ClassicBlinker* blinkerApplication = nullptr;

/**
 * @brief Arduino setup function.
 *
 * Initializes serial communication and boots the Modest-IoT Nano-Framework application.
 * Background execution queues, FreeRTOS tasks, and hardware alarm vectors
 * are launched automatically upon instantiation of `ClassicBlinker`.
 */
void setup() {
    Serial.begin(115200);

    // Boot the framework application.
    // Background execution queues, FreeRTOS tasks, and hardware alarm vectors launch automatically!
    blinkerApplication = new ClassicBlinker(SYSTEM_DIAGNOSTIC_LED_PIN, BLINK_RATE_MS, CORE_TIMER_CHANNEL);

    Serial.println("[Blinker Node] Loop-less, event-driven command routing active.");
}

/**
 * @brief Arduino loop function.
 *
 * This master loop is intentionally kept completely clean and empty.
 * It yields 100% of its execution cycles natively back to the FreeRTOS kernel scheduler,
 * allowing the Modest-IoT Nano-Framework's background tasks to manage the application logic.
 * A `vTaskDelay` is included to prevent watchdog timer resets in some FreeRTOS configurations
 * when the loop is otherwise completely idle.
 */
void loop() {
    // The master loop is completely clean and empty.
    // It yields 100% of its execution cycles natively back to the FreeRTOS kernel scheduler.
    vTaskDelay(pdMS_TO_TICKS(60000));
}