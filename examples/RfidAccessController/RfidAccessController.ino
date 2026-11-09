/**
 * @file RfidAccessController.ino
 * @brief RFID-based access control device using MFRC522 and Modest IoT.
 *
 * Demonstrates RfidSensor integration within an event-driven Device subclass.
 * Reads ISO/IEC 14443A card UIDs and verifies them against an authorized tag,
 * driving feedback actuators (Buzzer and LED/Servo) accordingly.
 *
 * Default SPI Wiring (ESP32):
 * - SCK:  GPIO 18
 * - MISO: GPIO 19
 * - MOSI: GPIO 23
 * - CS:   GPIO 5 (SDA/SS pin on MFRC522)
 * - RST:  GPIO 22 (RST pin on MFRC522)
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
static const unsigned long SAMPLING_INTERVAL_IN_MILLISECONDS = 100;
static const int ASYNC_ENGINE_QUEUE_DEPTH       = 8;
static const unsigned long MAIN_LOOP_DELAY_IN_MILLISECONDS   = 60000;

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

static RfidAccessController* controller = nullptr;

void setup() {
    Serial.begin(115200);
    controller = new RfidAccessController(RFID_CS_PIN, RFID_RST_PIN, ACCESS_LED_PIN, BUZZER_PIN);
    Serial.println("[System Boot] RFID Access Controller active.");
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}
