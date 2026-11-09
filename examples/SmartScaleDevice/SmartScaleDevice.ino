/**
 * @file SmartScaleDevice.ino
 * @brief Smart Scale example using 4 LoadCellAmplifiers and LCD display.
 * @date 2026-06-17
 * @version 1.0
 */

#include <Arduino.h>
#include <ModestIoT.h>

// Pin definitions
static const int            FRONT_LEFT_LOAD_CELL_DATA_PIN   = 4;
static const int            FRONT_LEFT_LOAD_CELL_CLOCK_PIN  = 5;
static const int            FRONT_RIGHT_LOAD_CELL_DATA_PIN  = 18;
static const int            FRONT_RIGHT_LOAD_CELL_CLOCK_PIN = 19;
static const int            REAR_LEFT_LOAD_CELL_DATA_PIN    = 26;
static const int            REAR_LEFT_LOAD_CELL_CLOCK_PIN   = 27;
static const int            REAR_RIGHT_LOAD_CELL_DATA_PIN   = 32;
static const int            REAR_RIGHT_LOAD_CELL_CLOCK_PIN  = 33;
static const int            LCD_I2C_ADDRESS                 = 0x27;

// Configuration constants
static const float          MINIMUM_WEIGHT_IN_GRAMS         = 0.0f;
static const float          MAXIMUM_WEIGHT_IN_GRAMS         = 50000.0f; // 50 Kg per load cell
static const long           MINIMUM_RAW_VALUE               = 0;
static const long           MAXIMUM_RAW_VALUE               = 1000000;
static const int            LOAD_CELL_FILTER_DEPTH          = 5;

// LCD dimensions
static const int            LCD_COLUMNS                     = 16;
static const int            LCD_ROWS                        = 2;

// Timing constants
static const unsigned long  SERIAL_BAUD_RATE                = 115200;

// Asynchronous engine configuration
static const int            ASYNC_ENGINE_QUEUE_DEPTH        = 16;
static const unsigned long  SAMPLING_INTERVAL_IN_MILLISECONDS            = 500;
static const unsigned long  MAIN_LOOP_DELAY_IN_MILLISECONDS              = 60000;
static const int            TIMER_GROUP_INDEX               = 0;

static const char*          STATUS_DISPLAY_TITLE            = "Smart Scale";
static const char*          STATUS_DISPLAY_CALIBRATING      = "Calibrating...";
static const char*          WEIGHT_DISPLAY_FORMAT           = "Weight: %.2f Kg";
static const char*          SYSTEM_BOOT_MESSAGE             = "[System Boot] Smart Scale active.";

/**
 * @brief Smart Scale Device.
 * Coordinates 4 LoadCellAmplifiers to calculate total weight and display it on an LCD.
 */
class SmartScaleDevice : public Device {
private:
    LoadCellAmplifier frontLeftLoadCell;
    LoadCellAmplifier frontRightLoadCell;
    LoadCellAmplifier rearLeftLoadCell;
    LoadCellAmplifier rearRightLoadCell;
    CharacterLcdDisplay statusDisplay;

    // Internal state
    float totalWeightInGrams;
    bool isCalibrated;

    /**
     * @brief Converts weight from grams to kilograms.
     * @param weightInGrams Weight in grams.
     * @return Weight in kilograms.
     */
    inline float convertGramsToKilograms(float weightInGrams) const {
        return weightInGrams / 1000.0f;
    }

    /**
     * @brief Internal state maintenance and LCD interface update.
     * Updates the status display with the total weight on the scale or calibration status.
     */
    void updateStatusDisplay() {
        char weightBuffer[17];

        if (!isCalibrated) {
            statusDisplay.setLineBuffer(0, STATUS_DISPLAY_TITLE);
            statusDisplay.setLineBuffer(1, STATUS_DISPLAY_CALIBRATING);
        } else {
            snprintf(weightBuffer, sizeof(weightBuffer), WEIGHT_DISPLAY_FORMAT, convertGramsToKilograms(totalWeightInGrams));
            //snprintf(weightBuffer, sizeof(weightBuffer), WEIGHT_DISPLAY_FORMAT, totalWeightInGrams);
            statusDisplay.setLineBuffer(0, STATUS_DISPLAY_TITLE);
            statusDisplay.setLineBuffer(1, weightBuffer);
        }
        statusDisplay.handle(CharacterLcdDisplay::UPDATE_TEXT_COMMAND);
    }

public:
    SmartScaleDevice(   int frontLeftDataPin,   int frontLeftClockPin,
                        int frontRightDataPin,  int frontRightClockPin,
                        int rearLeftDataPin,    int rearLeftClockPin,
                        int rearRightDataPin,   int rearRightClockPin,
                        uint8_t lcdI2cAddress)
        : Device(SAMPLING_INTERVAL_IN_MILLISECONDS, TIMER_GROUP_INDEX),
          frontLeftLoadCell(frontLeftDataPin,   frontLeftClockPin,  MINIMUM_WEIGHT_IN_GRAMS, MAXIMUM_WEIGHT_IN_GRAMS,
                            MINIMUM_RAW_VALUE,  MAXIMUM_RAW_VALUE, LOAD_CELL_FILTER_DEPTH, this),
          frontRightLoadCell(frontRightDataPin, frontRightClockPin, MINIMUM_WEIGHT_IN_GRAMS, MAXIMUM_WEIGHT_IN_GRAMS,
                            MINIMUM_RAW_VALUE, MAXIMUM_RAW_VALUE, LOAD_CELL_FILTER_DEPTH, this),
          rearLeftLoadCell(rearLeftDataPin,     rearLeftClockPin,   MINIMUM_WEIGHT_IN_GRAMS, MAXIMUM_WEIGHT_IN_GRAMS,
                            MINIMUM_RAW_VALUE, MAXIMUM_RAW_VALUE, LOAD_CELL_FILTER_DEPTH, this),
          rearRightLoadCell(rearRightDataPin,   rearRightClockPin,  MINIMUM_WEIGHT_IN_GRAMS, MAXIMUM_WEIGHT_IN_GRAMS,
                            MINIMUM_RAW_VALUE, MAXIMUM_RAW_VALUE, LOAD_CELL_FILTER_DEPTH, this),
          statusDisplay(lcdI2cAddress, LCD_COLUMNS, LCD_ROWS, this),
          totalWeightInGrams(0.0f),
          isCalibrated(false)
    {
        initializeAsynchronousEngine(ASYNC_ENGINE_QUEUE_DEPTH);

        // Hybrid scheduling: interrupt-driven data-ready detection eliminates the 100 ms busy-wait.
        // Periodic sampling performs the 24-bit read (~1.2 ms) and change detection.
        appendToBothSchedulers(&frontLeftLoadCell,     Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, FALLING);
        appendToBothSchedulers(&frontRightLoadCell,    Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, FALLING);
        appendToBothSchedulers(&rearLeftLoadCell,      Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, FALLING);
        appendToBothSchedulers(&rearRightLoadCell,     Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, FALLING);

        updateStatusDisplay();
        calibrate();
    }

    /**
     * @brief Performs the calibration phase.
     * Tares all load cells to set the initial zero weight baseline.
     */
    void calibrate() {
        frontLeftLoadCell.tare();
        frontRightLoadCell.tare();
        rearLeftLoadCell.tare();
        rearRightLoadCell.tare();
        isCalibrated = true;
    }

    /**
     * @brief Inversion of Control interface contract handler.
     * Intercepts framework events (e.g., sensor data ready) to calculate the total scale weight.
     * @param event The triggered event context payload.
     */
    void on(Event event) override {
        if (!isCalibrated) {
            return;
        }

        if (event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER) {
            totalWeightInGrams = frontLeftLoadCell.getWeightInGrams() + 
                                 frontRightLoadCell.getWeightInGrams() + 
                                 rearLeftLoadCell.getWeightInGrams() + 
                                 rearRightLoadCell.getWeightInGrams();
            updateStatusDisplay();
        }
    }
};

static SmartScaleDevice* smartScale = nullptr;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    smartScale = new SmartScaleDevice(
        FRONT_LEFT_LOAD_CELL_DATA_PIN, FRONT_LEFT_LOAD_CELL_CLOCK_PIN,
        FRONT_RIGHT_LOAD_CELL_DATA_PIN, FRONT_RIGHT_LOAD_CELL_CLOCK_PIN,
        REAR_LEFT_LOAD_CELL_DATA_PIN, REAR_LEFT_LOAD_CELL_CLOCK_PIN,
        REAR_RIGHT_LOAD_CELL_DATA_PIN, REAR_RIGHT_LOAD_CELL_CLOCK_PIN,
        LCD_I2C_ADDRESS
    );
    Serial.println(SYSTEM_BOOT_MESSAGE);
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}
