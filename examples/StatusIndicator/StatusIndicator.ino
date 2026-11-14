/**
 * @file StatusIndicator.ino
 * @brief Minimal status indicator with periodic LED toggle.
 *
 * This example demonstrates a grounded real-world device: a status indicator LED
 * that toggles at a fixed interval to signal device health or connectivity state.
 * Every IoT device includes such an indicator; this example shows the simplest
 * way to implement one using the Modest-IoT Nano-Framework's event-driven model.
 *
 * @par Modest IoT Nano Framework aspects illustrated:
 * - `Device` base class as the application mediator
 * - `Led` actuator with `TOGGLE_LED_COMMAND`
 * - `appendSensorToScheduler()` for periodic timer events
 * - Loop-less, event-driven asynchronous execution via `initializeAsynchronousEngine()`
 * - Custom event identifiers for timer-based scheduling
 *
 * @date 2026-06-06
 * @version 1.0
 */

#include <Arduino.h>
#include <ModestIoT.h>

/** @brief Serial communication baud rate. */
static const unsigned long SERIAL_BAUD_RATE = 115200;

/** @brief Delay in the main loop when no work is pending. */
static const unsigned long MAIN_LOOP_DELAY_IN_MILLISECONDS = 60000;

/** @brief Sampler event queue length for status indicator. */
static const int ASYNC_ENGINE_QUEUE_DEPTH = 5;

/** @name Application Mediator Implementation */
//@{

/**
 * @brief Status indicator device that toggles an LED at a fixed interval.
 *
 * This class extends the `Device` base class from the Modest-IoT Nano-Framework
 * to manage an LED, toggling its state asynchronously based on timer events.
 * It demonstrates a loop-less, event-driven approach using framework Event handlers
 * and class-encapsulated Actuator Commands, avoiding traditional `delay()` calls
 * and direct state management within the main loop.
 */
class StatusIndicator : public Device {
    private:
    /**
     * @brief The LED actuator controlled by this application.
     */
    Led statusLed;

    public:
    /** @brief Event identifier for the hardware timer tick. */
    static constexpr int TIMER_EVENT_IDENTIFIER = 300;

    /**
     * @brief Constructs a new StatusIndicator application instance.
     *
     * Initializes the device with a specified LED pin, blink interval,
     * and an optional hardware timer channel. It sets up the asynchronous
     * engine and registers itself to receive automated clock alarm notifications.
     *
     * @param ledPin Target GPIO pin where the physical LED is connected.
     * @param blinkIntervalInMilliseconds Ticking frequency rate in milliseconds.
     * @param timerChannel Dedicated hardware 64-bit timer group index (0-3).
     *                     Defaults to 0 if not specified.
     */
    StatusIndicator(int ledPin, unsigned long blinkIntervalInMilliseconds, uint8_t timerChannel = 0)
    : Device(blinkIntervalInMilliseconds, timerChannel),
    statusLed(ledPin, false, this) // Pass pin, initial active state (false), and set parent handler
    {
        // 1. Launch internal background task workers
        initializeAsynchronousEngine(ASYNC_ENGINE_QUEUE_DEPTH);

        // 2. Register this device context to receive automated clock alarm notifications
        // mapped directly to our clean event identity token
        // Capture and evaluate scheduler capacity bounds gracefully
        // Note: `this` is a Device, not a Sensor, so the EventHandler* overload is used with
        // an explicit period matching the timer tick interval.
        bool registered = appendSensorToScheduler(this, StatusIndicator::TIMER_EVENT_IDENTIFIER, blinkIntervalInMilliseconds, 0);
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
    if (event.identifier == StatusIndicator::TIMER_EVENT_IDENTIFIER) {

        // Route the pre-instantiated static toggle command directly to the actuator.
        // No local state tracking variables or toggles are written here!
        statusLed.handle(Led::TOGGLE_LED_COMMAND);
        Serial.println("[StatusIndicator] Timer event caught -> LED toggle dispatched.");
    }
}
};
//@} // Application Mediator Implementation

/** @name Top-Level Declarative Canvas */
//@{

/**
 * @brief Defines the GPIO pin for the system diagnostic LED.
 *
 * This typically targets the ESP32 onboard diagnostic blue LED pin (GPIO 2).
 */
static const int SYSTEM_DIAGNOSTIC_LED_PIN = 2;

/**
 * @brief Defines the blink rate for the LED in milliseconds.
 */
static const unsigned long BLINK_RATE_IN_MILLISECONDS = 500;

/**
 * @brief Defines the hardware core timer channel to be used.
 *
 * This specifies which 64-bit timer group (0-3) will be utilized for timing events.
 */
static const uint8_t CORE_TIMER_CHANNEL = 0;

/**
 * @brief Pointer to the StatusIndicator application instance.
 *
 * This global pointer holds the single instance of our status indicator application.
 */
static StatusIndicator* statusIndicatorApplication = nullptr;

/**
 * @brief Arduino setup function.
 *
 * Initializes serial communication and boots the Modest-IoT Nano-Framework application.
 * Background execution queues, FreeRTOS tasks, and hardware alarm vectors
 * are launched automatically upon instantiation of `StatusIndicator`.
 */
void setup() {
    Serial.begin(SERIAL_BAUD_RATE);

    // Boot the framework application.
    // Background execution queues, FreeRTOS tasks, and hardware alarm vectors launch automatically!
    statusIndicatorApplication = new StatusIndicator(
    SYSTEM_DIAGNOSTIC_LED_PIN,
    BLINK_RATE_IN_MILLISECONDS,
    CORE_TIMER_CHANNEL);

    Serial.println("[StatusIndicator] Loop-less, event-driven command routing active.");
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
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}
//@} // Top-Level Declarative Canvas