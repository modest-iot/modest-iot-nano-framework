/**
 * @file RfidAccessController.ino
 * @brief RFID door access controller with visual and audible feedback.
 *
 * Uses an MFRC522 RFID reader to scan ISO/IEC 14443A cards, compares the scanned
 * UID against an authorized tag list, and provides feedback via a green LED (access
 * granted) or red LED (access denied) and a buzzer with distinct tones. Demonstrates
 * SPI-based sensor integration with the framework's event-driven architecture.
 *
 * @par Modest IoT Nano Framework aspects illustrated:
 * - `Device` base class with async engine for loop-less execution
 * - `RfidSensor` with SPI-based MFRC522 for card UID reading
 * - `Led` actuator with `TURN_ON_COMMAND` and `TURN_OFF_COMMAND` for visual feedback
 * - `Buzzer` actuator with `PLAY_TONE_COMMAND` for audible feedback
 * - Event routing via `on(Event)` with `TAG_DETECTED_EVENT_IDENTIFIER`
 * - Tag removal handling via `TAG_REMOVED_EVENT_IDENTIFIER`
 * - Direct actuator command dispatch via `handle(Command)`
 * @date 2026-06-12
 * @version 1.0
 */

#include <Arduino.h>
#include <ModestIoT.h>

// Pin definitions
static const int RFID_CS_PIN      = 5;
static const int RFID_RST_PIN     = 22;
static const int ACCESS_LED_PIN   = 2;
static const int BUZZER_PIN       = 4;

// Authorized RFID Card UID (example UID)
static const char* const AUTHORIZED_UID = "DE:AD:BE:EF";

// Timing constants
/** @brief Serial communication baud rate. */
static const unsigned long SERIAL_BAUD_RATE = 115200;

static const unsigned long SAMPLING_INTERVAL_IN_MILLISECONDS = 100;
static const int ASYNC_ENGINE_QUEUE_DEPTH = 8;
static const unsigned long MAIN_LOOP_DELAY_IN_MILLISECONDS = 60000;

/**
 * @brief Access Control Device managing an RFID reader, feedback LED, and buzzer.
 */
class RfidAccessController : public Device {
private:
    RfidSensor rfidReader;
    Led accessLed;
    Buzzer feedbackBuzzer;

public:
    RfidAccessController(int csPin, int rstPin, int ledPin, int buzzerPin)
        : Device(SAMPLING_INTERVAL_IN_MILLISECONDS),
          rfidReader(csPin, rstPin, this),
          accessLed(ledPin, false, this),
          feedbackBuzzer(buzzerPin, this)
    {
        initializeAsynchronousEngine(ASYNC_ENGINE_QUEUE_DEPTH);
        appendSensorToScheduler(&rfidReader, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    }

    /**
     * @brief Handles RFID tag detected and removed events.
     * @param event The triggered event context payload.
     * @runs_on{sampler}
     */
    void on(Event event) override {
        if (event.identifier == RfidSensor::TAG_DETECTED_EVENT_IDENTIFIER) {
            const char* scannedUid = rfidReader.getUid();
            Serial.printf("[RFID] Tag detected: %s\n", scannedUid);

            if (strcmp(scannedUid, AUTHORIZED_UID) == 0) {
                Serial.println("[Access] ACCESS GRANTED");
                accessLed.handle(Led::TURN_ON_COMMAND);
                feedbackBuzzer.setTone(2000, 200);
                feedbackBuzzer.handle(Buzzer::PLAY_TONE_COMMAND);
            } else {
                Serial.println("[Access] ACCESS DENIED");
                accessLed.handle(Led::TURN_OFF_COMMAND);
                feedbackBuzzer.setTone(500, 500);
                feedbackBuzzer.handle(Buzzer::PLAY_TONE_COMMAND);
            }
        } else if (event.identifier == RfidSensor::TAG_REMOVED_EVENT_IDENTIFIER) {
            Serial.println("[RFID] Tag removed from reader field.");
            accessLed.handle(Led::TURN_OFF_COMMAND);
        }
    }
};

static RfidAccessController* rfidAccessApplication = nullptr;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    rfidAccessApplication = new RfidAccessController(RFID_CS_PIN, RFID_RST_PIN, ACCESS_LED_PIN, BUZZER_PIN);
    Serial.println("[System Boot] RFID Access Controller active.");
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}
