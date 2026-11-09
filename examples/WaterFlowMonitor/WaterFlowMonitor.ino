/**
 * @file WaterFlowMonitor.ino
 * @brief Water flow monitor example using YF-S201 sensor with hybrid scheduling.
 * @date 2026-11-09
 * @version 1.0
 */

#include <Arduino.h>
#include <ModestIoT.h>

// Pin definitions
static const int            WATER_FLOW_SIGNAL_PIN           = 4;

// Timing constants
static const unsigned long  SERIAL_BAUD_RATE                = 115200;

// Asynchronous engine configuration
static const int            ASYNC_ENGINE_QUEUE_DEPTH        = 16;
static const unsigned long  SAMPLING_INTERVAL_IN_MILLISECONDS            = 1000;
static const unsigned long  MAIN_LOOP_DELAY_IN_MILLISECONDS              = 60000;
static const int            TIMER_GROUP_INDEX               = 0;

static const char*          SYSTEM_BOOT_MESSAGE             = "[System Boot] Water Flow Monitor active.";
static const char*          FLOW_RATE_FORMAT                = "Flow rate: %.2f L/min";

/**
 * @brief Water Flow Monitor Device.
 * Monitors water flow rate using a YF-S201 Hall effect sensor with hybrid scheduling:
 * interrupt-driven pulse counting and periodic flow rate computation.
 */
class WaterFlowMonitor : public Device {
private:
    WaterFlowSensor flowSensor;

    /**
     * @brief Logs the current flow rate to Serial.
     */
    void logFlowRate() {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), FLOW_RATE_FORMAT, flowSensor.getFlowRateInLitersPerMinute());
        Serial.println(buffer);
    }

public:
    WaterFlowMonitor(int signalPin)
        : Device(SAMPLING_INTERVAL_IN_MILLISECONDS, TIMER_GROUP_INDEX),
          flowSensor(signalPin, this)
    {
        initializeAsynchronousEngine(ASYNC_ENGINE_QUEUE_DEPTH);

        // Hybrid scheduling: interrupt-driven pulse counting via CHANGE edge captures
        // every Hall effect pulse, while periodic sampling computes the flow rate.
        appendToBothSchedulers(&flowSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, CHANGE);
    }

    /**
     * @brief Inversion of Control interface contract handler.
     * Intercepts sensor data-ready events to log the flow rate.
     * @param event The triggered event context payload.
     */
    void on(Event event) override {
        if (event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER) {
            logFlowRate();
        }
    }
};

static WaterFlowMonitor* monitor = nullptr;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    monitor = new WaterFlowMonitor(WATER_FLOW_SIGNAL_PIN);
    Serial.println(SYSTEM_BOOT_MESSAGE);
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}
