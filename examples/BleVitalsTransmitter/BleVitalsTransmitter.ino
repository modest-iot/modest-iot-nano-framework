/**
 * @file BleVitalsTransmitter.ino
 * @brief Wearable fitness band that sends heart rate and temperature via BLE to a phone app.
 *
 * This example demonstrates a grounded real-world device: a wrist-worn fitness tracker
 * that measures heart rate using a pulse sensor and skin temperature using a DHT22 sensor,
 * then transmits the readings via BLE NUS (Nordic UART Service) to a paired smartphone.
 * The phone receives JSON telemetry lines and displays vitals in real-time.
 * No WiFi is required — the device works anywhere a BLE connection is available.
 *
 * @par Modest IoT Nano Framework aspects illustrated:
 * - `BleGatewayClient` for BLE NUS telemetry transmission
 * - `BleConnectivityDriver` for BLE peripheral mode advertising and connection management
 * - `TelemetryPackage` subclass for custom data schema (heart rate, temperature)
 * - `PulseSensor` for heart rate measurement
 * - `DhtSensor` for skin temperature measurement
 * - `getStatistics()` for send success/failure tracking via `GatewayStatistics`
 * - Loop-less, event-driven asynchronous execution via `initializeAsynchronousEngine()`
 *
 * @date 2026-11-12
 * @version 1.0
 */

#define JSON_KEY_HEART_BEATS_PER_MINUTE "heartBeatsPerMinute"
#define JSON_KEY_SKIN_TEMPERATURE_IN_CELSIUS "skinTemperatureInCelsius"

/** @brief Sampler event queue length for BLE vitals transmission. */
static const int SAMPLER_QUEUE_LENGTH = 10;

#include <Arduino.h>
#include <memory>
#include "ModestIoT.h"

/** @name Concrete Application Data Schema Specification */
//@{

class VitalsTelemetryPackage : public TelemetryPackage {
private:
    int32_t heartBeatsPerMinute;
    float skinTemperatureInCelsius;

public:
    VitalsTelemetryPackage(int bpm, float temp)
        : heartBeatsPerMinute(bpm),
          skinTemperatureInCelsius(temp) {}

    ~VitalsTelemetryPackage() override = default;

    void write(TelemetryWriter& writer) const override {
        writer.field(JSON_KEY_HEART_BEATS_PER_MINUTE, heartBeatsPerMinute);
        writer.field(JSON_KEY_SKIN_TEMPERATURE_IN_CELSIUS, skinTemperatureInCelsius);
    }
};
//@} // Concrete Application Data Schema Specification

/** @brief Serial communication baud rate. */
static const unsigned long SERIAL_BAUD_RATE = 115200;

/** @brief Delay in the main loop when no work is pending. */
static const unsigned long MAIN_LOOP_DELAY_IN_MILLISECONDS = 60000;

/** @name Serial Output Configuration */
//@{
/** @brief Log prefix for all BleVitalsTransmitter serial output. */
static const char* BLE_VITALS_TRANSMITTER_LOG_PREFIX = "[BleVitalsTransmitter]";
//@} // Serial Output Configuration

/** @name Application Mediator Implementation */
//@{

/**
 * @brief Wearable fitness band device that transmits vitals via BLE NUS.
 *
 * This class extends the `Device` base class to manage a pulse sensor for heart rate,
 * a DHT22 sensor for skin temperature, and a BLE gateway client for wireless transmission.
 * All sensor sampling and telemetry processing is handled asynchronously by the framework.
 */
class BleVitalsTransmitter : public Device {
private:
    PulseSensor heartRateSensor;
    DhtSensor skinTempSensor;
    BleGatewayClient& gatewayClient;

    /**
     * @brief Packages the latest sensor readings into a telemetry packet and queues it.
     */
    void packageAndQueueTelemetrySnapshot() {
        auto packet = std::make_unique<VitalsTelemetryPackage>(
            heartRateSensor.getBeatsPerMinute(),
            skinTempSensor.getTemperatureInCelsius()
        );
        enqueueTelemetryPayload(std::move(packet));
    }

    /**
     * @brief Reports BLE transmission statistics to Serial.
     */
    void reportStatistics() {
        GatewayStatistics stats = gatewayClient.getStatistics();
        Serial.printf("%s BLE stats -> succeeded: %lu, failed: %lu\n",
                      BLE_VITALS_TRANSMITTER_LOG_PREFIX,
                      stats.sendsSucceeded, stats.sendsFailed);
    }

protected:
    void processQueuedTelemetryData(const TelemetryPackage* rawQueueItemPayload) const override {
        if (rawQueueItemPayload != nullptr) {
            gatewayClient.sendTelemetryRecord(*rawQueueItemPayload);
        }
    }

    void maintainNetworkServices() const override {
        gatewayClient.service();
    }

public:
    /**
     * @brief Constructs a new BleVitalsTransmitter application instance.
     *
     * @param pulsePin GPIO pin connected to the pulse sensor output.
     * @param tempPin GPIO pin connected to the DHT22 data line.
     * @param client Reference to the BLE gateway client.
     * @param samplingIntervalInMilliseconds How often to sample sensors (ms).
     * @param timerGroupIndex Hardware timer group index (0-3).
     */
    BleVitalsTransmitter(int pulsePin, int tempPin, BleGatewayClient& client,
                         unsigned long samplingIntervalInMilliseconds,
                         uint8_t timerGroupIndex = 0)
        : Device(samplingIntervalInMilliseconds, timerGroupIndex),
          heartRateSensor(pulsePin, this),
          skinTempSensor(tempPin, DhtSensor::SENSOR_MODEL_DHT22, this),
          gatewayClient(client)
    {
        // IMPORTANT: Registration must occur before initializeAsynchronousEngine(), because
        // the engine start guard rejects any registration after the engine is running.
        appendSensorToScheduler(&heartRateSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
        appendSensorToScheduler(&skinTempSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);

        initializeAsynchronousEngine(SAMPLER_QUEUE_LENGTH);
    }

    ~BleVitalsTransmitter() override {
        stopAsynchronousEngine();
    }

    void on(Event event) override {
        if (event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER) {
            packageAndQueueTelemetrySnapshot();
        }
    }

    void handle(Command command) override {}

    /**
     * @brief Starts BLE advertising and begins telemetry operations.
     * @return True if BLE initialization succeeded.
     */
    bool start() {
        Device::setFirmwareVersion("BVT-100 FW 1.0.0");
        Serial.println((String(BLE_VITALS_TRANSMITTER_LOG_PREFIX) + " Starting BLE advertising...").c_str());
        gatewayClient.service();
        Serial.println((String(BLE_VITALS_TRANSMITTER_LOG_PREFIX) + " BLE vitals transmitter online. Pair with your phone.").c_str());
        return true;
    }
};
//@} // Application Mediator Implementation

/** @name Top-Level Declarative Canvas */
//@{

static const int PULSE_SENSOR_PIN = 15;
static const int SKIN_TEMPERATURE_PIN = 4;

static const char* BLE_DEVICE_NAME = "VitalsBand";

static const unsigned long SAMPLING_INTERVAL_IN_MILLISECONDS = 2000;
static const uint8_t HARDWARE_TIMER_CHANNEL_INDEX = 0;

static BleConnectivityDriver* transportDriver = nullptr;
static BleGatewayClient* gatewayClient = nullptr;
static BleVitalsTransmitter* vitalsApplication = nullptr;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);

    transportDriver = new BleConnectivityDriver(BLE_DEVICE_NAME);
    gatewayClient = new BleGatewayClient(*transportDriver, BLE_DEVICE_NAME);

    vitalsApplication = new BleVitalsTransmitter(
        PULSE_SENSOR_PIN,
        SKIN_TEMPERATURE_PIN,
        *gatewayClient,
        SAMPLING_INTERVAL_IN_MILLISECONDS,
        HARDWARE_TIMER_CHANNEL_INDEX
    );

    if (!vitalsApplication->start()) {
        Serial.println((String(BLE_VITALS_TRANSMITTER_LOG_PREFIX) + " Device startup failed.").c_str());
        return;
    }
}

void loop() {
    // The master loop is completely clean and empty.
    // The framework handles all timing, sensor sampling, and BLE telemetry transmission.
    // vTaskDelay clarifies that this loop yields to FreeRTOS — the device is loopless.
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}
//@} // Top-Level Declarative Canvas
