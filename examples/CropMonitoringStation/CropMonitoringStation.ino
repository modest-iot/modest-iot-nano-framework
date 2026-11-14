/**
 * @file CropMonitoringStation.ino
 * @brief Battery-powered crop monitoring station with light sleep for energy efficiency.
 *
 * Monitors soil moisture, air temperature/humidity, and sunlight intensity in agricultural
 * fields. Uses ESP32 light sleep between sensor samples to reduce power consumption,
 * enabling months of battery operation. Three sensors with different sampling intervals
 * demonstrate the per-slot scheduler's tickless mode.
 *
 * @par Modest IoT Nano Framework aspects illustrated:
 * - `Device` base class with `PowerMode::Tickless` for ESP32 light sleep
 * - `SoilMoistureSensor` (analog), `DhtSensor` (digital), `DigitalLightIntensitySensor` (I2C)
 * - Per-slot scheduler with staggered sampling intervals (30s, 60s, 120s)
 * - `ticksUntilNextDue()` for optimal sleep duration calculation
 * - `TelemetryPackage` subclass with custom `serialize()` for JSON schema
 * - `MqttGatewayClient` for cloud telemetry publishing
 * - `WiFiConnectivityDriver` for WiFi link management
 * - `JSON_KEY_*` constants for telemetry serialization
 * @date 2026-11-09
 * @version 1.0
 */

#include <Arduino.h>
#include <memory>
#include <ArduinoJson.h>
#include <ModestIoT.h>

/** @name Default Configuration Constants */
//@{
// Static constants for constructor defaults, allowing use before class declaration.

/** @brief Default soil moisture sampling interval in milliseconds. */
static const unsigned long DEFAULT_SOIL_SAMPLING_INTERVAL_IN_MILLISECONDS = 30000UL;

/** @brief Default air temperature/humidity sampling interval in milliseconds. */
static const unsigned long DEFAULT_AIR_SAMPLING_INTERVAL_IN_MILLISECONDS = 60000UL;

/** @brief Default light intensity sampling interval in milliseconds. */
static const unsigned long DEFAULT_LIGHT_SAMPLING_INTERVAL_IN_MILLISECONDS = 120000UL;

/** @brief Default hardware timer channel index. */
static const uint8_t DEFAULT_TIMER_CHANNEL = 0;

/** @brief Sampler event queue length for battery operation. */
static const int SAMPLER_QUEUE_LENGTH = 5;
//@} // Default Configuration Constants

/** @name Telemetry JSON Keys */
//@{
/** @brief JSON key for soil moisture percentage. */
#define JSON_KEY_SOIL_MOISTURE "soilMoisture"
/** @brief JSON key for air temperature in Celsius. */
#define JSON_KEY_AIR_TEMPERATURE_IN_CELSIUS "airTemperatureInCelsius"
/** @brief JSON key for air humidity in percentage. */
#define JSON_KEY_AIR_HUMIDITY_IN_PERCENTAGE "airHumidityInPercentage"
/** @brief JSON key for light intensity in Lux. */
#define JSON_KEY_LIGHT_INTENSITY_IN_LUX "lightIntensityInLux"
//@} // Telemetry JSON Keys

/** @name Concrete Application Data Schema Specification */
//@{

/**
 * @brief Telemetry package for the Crop Monitoring Station.
 *
 * Contains readings from all three sensors: soil moisture, air temperature,
 * air humidity, and light intensity. Serializes to a single JSON record
 * for MQTT publishing.
 */
class CropMonitoringTelemetryPackage : public TelemetryPackage {
    private:
    int soilMoisture;
    float airTemperatureInCelsius;
    float airHumidityInPercentage;
    float lightIntensityInLux;

    public:
    CropMonitoringTelemetryPackage(
        int moisture,
        float temperature,
        float humidity,
        float light)
        : soilMoisture(moisture),
        airTemperatureInCelsius(temperature),
        airHumidityInPercentage(humidity),
        lightIntensityInLux(light) {}

    ~CropMonitoringTelemetryPackage() override = default;

    void serialize(JsonDocument& serializationDestination) const override {
        serializationDestination[JSON_KEY_SOIL_MOISTURE]              = this->soilMoisture;
        serializationDestination[JSON_KEY_AIR_TEMPERATURE_IN_CELSIUS]   = this->airTemperatureInCelsius;
        serializationDestination[JSON_KEY_AIR_HUMIDITY_IN_PERCENTAGE]   = this->airHumidityInPercentage;
        serializationDestination[JSON_KEY_LIGHT_INTENSITY_IN_LUX]       = this->lightIntensityInLux;
    }
};
//@} // Concrete Application Data Schema Specification

/** @name Application Mediator Implementation */
//@{

/**
 * @brief Implements a battery-powered crop monitoring station with light sleep support.
 *
 * This class extends the `Device` base class to manage three sensors (soil moisture,
 * air temperature/humidity, and sunlight intensity). It demonstrates tickless scheduling
 * where the ESP32 enters light sleep between sensor samples, reducing power consumption.
 * Telemetry readings are published via MQTT on each sensor event.
 *
 * The device wakes from light sleep when the hardware timer alarm fires,
 * reads the due sensor(s), publishes a unified telemetry record via MQTT,
 * and then enters light sleep again until the next sample is due.
 * The `ticksUntilNextDue()` method calculates the optimal sleep duration
 * across all registered sensors.
 */
class CropMonitoringStation : public Device {
    private:
    /**
     * @brief Soil moisture sensor for reading soil moisture levels.
     */
    SoilMoistureSensor soilSensor;

    /**
     * @brief DHT sensor for reading air temperature and humidity.
     */
    DhtSensor airSensor;

    /**
     * @brief Digital light sensor for measuring sunlight intensity.
     */
    DigitalLightIntensitySensor lightSensor;

    /**
     * @brief MQTT gateway client for publishing telemetry records.
     */
    MqttGatewayClient& gatewayClient;

    private:
    /**
     * @brief Packages the latest readings from all sensors into a telemetry record
     * and queues it for the network task to publish via MQTT.
     */
    void packageAndQueueTelemetrySnapshot() {
        auto packet = std::make_unique<CropMonitoringTelemetryPackage>(
            soilSensor.getMoistureLevel(),
            airSensor.getTemperatureInCelsius(),
            airSensor.getRelativeHumidityInPercentage(),
            lightSensor.getIlluminanceInLux()
        );
        enqueueTelemetryPayload(std::move(packet));
    }

    protected:
    /**
     * @brief Processes the queued telemetry payload by sending it via MQTT.
     *
     * This method is called from the network task context.
     *
     * @param rawQueueItemPayload The telemetry package to send.
     */
    void processQueuedTelemetryData(const TelemetryPackage* rawQueueItemPayload) const override {
        if (rawQueueItemPayload != nullptr) {
            gatewayClient.sendTelemetryRecord(*rawQueueItemPayload);
        }
    }

    /**
     * @brief Maintains the MQTT connection and processes pending network operations.
     *
     * This method is called from the network dispatch loop.
     */
    void maintainNetworkServices() const override {
        gatewayClient.service();
    }

    public:
    /**
     * @brief A localized event identifier for soil moisture data readings.
     */
    static const int SOIL_MOISTURE_EVENT_IDENTIFIER = 100;

    /**
     * @brief A localized event identifier for air temperature/humidity readings.
     */
    static const int AIR_TEMPERATURE_EVENT_IDENTIFIER = 101;

    /**
     * @brief A localized event identifier for sunlight intensity readings.
     */
    static const int LIGHT_INTENSITY_EVENT_IDENTIFIER = 102;

    /**
     * @brief Constructs a new CropMonitoringStation application instance.
     *
     * Initializes the device with multiple sensors at different sampling intervals,
     * configures tickless power mode, and sets up the asynchronous engine.
     *
     * @param soilSensorPin GPIO pin connected to the soil moisture sensor analog output.
     * @param dhtSensorPin GPIO pin connected to the DHT sensor data line.
     * @param client Reference to the MQTT gateway client for telemetry publishing.
     * @param soilSamplingIntervalInMilliseconds Interval between soil moisture readings in milliseconds.
     * @param airSamplingIntervalInMilliseconds Interval between air temperature readings in milliseconds.
     * @param lightSamplingIntervalInMilliseconds Interval between light intensity readings in milliseconds.
     * @param timerChannel Hardware timer group index (0-3). Defaults to 0.
     */
    CropMonitoringStation(
        int soilSensorPin,
        int dhtSensorPin,
        MqttGatewayClient& client,
        unsigned long soilSamplingIntervalInMilliseconds = DEFAULT_SOIL_SAMPLING_INTERVAL_IN_MILLISECONDS,
        unsigned long airSamplingIntervalInMilliseconds = DEFAULT_AIR_SAMPLING_INTERVAL_IN_MILLISECONDS,
        unsigned long lightSamplingIntervalInMilliseconds = DEFAULT_LIGHT_SAMPLING_INTERVAL_IN_MILLISECONDS,
        uint8_t timerChannel = DEFAULT_TIMER_CHANNEL)
    : Device(soilSamplingIntervalInMilliseconds, timerChannel),
    soilSensor(soilSensorPin, this),
    airSensor(dhtSensorPin, DhtSensor::SENSOR_MODEL_DHT22, this),
    lightSensor(0, DigitalLightIntensitySensor::DEFAULT_I2C_ADDRESS,
                DigitalLightIntensitySensor::DEFAULT_LIGHT_DETECTED_THRESHOLD_IN_LUX, this),
    gatewayClient(client)
    {
        // Configure tickless power mode for low-power operation
        // In this mode, the hardware timer uses one-shot alarms and the CPU
        // enters light sleep between sensor samples
        setPowerMode(PowerMode::Tickless);

        // Launch internal background task workers with a small queue for battery operation
        initializeAsynchronousEngine(SAMPLER_QUEUE_LENGTH);

        // Set sampling intervals before registration so the scheduler uses the correct periods
        soilSensor.setSamplingIntervalInMilliseconds(soilSamplingIntervalInMilliseconds);
        airSensor.setSamplingIntervalInMilliseconds(airSamplingIntervalInMilliseconds);
        lightSensor.setSamplingIntervalInMilliseconds(lightSamplingIntervalInMilliseconds);

        // Register the soil moisture sensor (most frequent: every 30s)
        appendSensorToScheduler(&soilSensor, SOIL_MOISTURE_EVENT_IDENTIFIER, soilSamplingIntervalInMilliseconds, 0);

        // Register the DHT air sensor (every 60s)
        appendSensorToScheduler(&airSensor, AIR_TEMPERATURE_EVENT_IDENTIFIER, airSamplingIntervalInMilliseconds, 0);

        // Register the light intensity sensor (least frequent: every 120s)
        appendSensorToScheduler(&lightSensor, LIGHT_INTENSITY_EVENT_IDENTIFIER, lightSamplingIntervalInMilliseconds, 0);
    }

    /**
     * @brief Handles sensor data-ready events to package and queue telemetry.
     * @param event The triggered event context payload.
     * @runs_on{sampler}
     */
    void on(Event event) override {
        // Package and queue a unified telemetry record for MQTT publishing.
        // The record includes the latest readings from all three sensors,
        // ensuring the MQTT payload always reflects the most current state.
        packageAndQueueTelemetrySnapshot();

        // The device will now enter light sleep until the next sample is due.
        // This happens automatically in the sampler task after all handlers complete.
    }

};
//@} // Application Mediator Implementation

/** @name Top-Level Declarative Canvas */
//@{

/**
 * @brief GPIO pin for the soil moisture sensor analog input.
 *
 * Connect the soil moisture sensor's A0 output to this pin.
 * The sensor should be powered by 3.3V (ESP32 ADC range).
 */
static const int SOIL_MOISTURE_SENSOR_PIN = 34;

/**
 * @brief GPIO pin for the DHT22 sensor data line.
 *
 * Connect the DHT22 DATA pin to this GPIO. Use a 10kΩ pull-up resistor.
 */
static const int DHT_SENSOR_PIN = 4;

/**
 * @brief Soil moisture sampling interval in milliseconds.
 *
 * Set to 30 seconds for battery operation.
 */
static const unsigned long SOIL_SAMPLING_INTERVAL_IN_MILLISECONDS = 30000;

/**
 * @brief Air temperature/humidity sampling interval in milliseconds.
 *
 * Set to 60 seconds — DHT22 minimum is 2 seconds, but slower sampling saves power.
 */
static const unsigned long AIR_SAMPLING_INTERVAL_IN_MILLISECONDS = 60000;

/**
 * @brief Light intensity sampling interval in milliseconds.
 *
 * Set to 120 seconds — sunlight changes slowly, so infrequent sampling is sufficient.
 */
static const unsigned long LIGHT_SAMPLING_INTERVAL_IN_MILLISECONDS = 120000;

/**
 * @brief Hardware timer channel.
 */
static const uint8_t TIMER_CHANNEL = 0;

/**
 * @brief Serial console baud rate.
 */
static const unsigned long SERIAL_BAUD_RATE = 115200;

/**
 * @brief Delay in the main loop when no work is pending.
 */
static const unsigned long MAIN_LOOP_DELAY_IN_MILLISECONDS = 60000;

/** @name Network Configuration */
//@{

/**
 * @brief Wi-Fi network SSID.
 */
static const char* WIRELESS_SSID = "Wokwi-GUEST";

/**
 * @brief Wi-Fi network password.
 */
static const char* WIRELESS_PASSWORD = "";

/**
 * @brief MQTT broker IP address.
 */
static const char* MQTT_BROKER_IP = "192.168.1.200";

/**
 * @brief MQTT broker port.
 */
static const uint16_t MQTT_BROKER_PORT = 1883;

/**
 * @brief MQTT topic for publishing crop monitoring telemetry records.
 */
static const char* MQTT_PUBLISH_TOPIC = "agriculture/crop-station/telemetry";

/**
 * @brief MQTT client identifier for this crop monitoring station.
 */
static const char* MQTT_CLIENT_ID = "CropStation_01";

/**
 * @brief Pointer to the Wi-Fi connectivity driver instance.
 */
static WiFiConnectivityDriver* transportDriver = nullptr;

/**
 * @brief Pointer to the MQTT gateway client instance.
 */
static MqttGatewayClient* gatewayClient = nullptr;

/**
 * @brief Pointer to the CropMonitoringStation application instance.
 */
static CropMonitoringStation* cropMonitoringApplication = nullptr;

/**
 * @brief Arduino setup function.
 *
 * Initializes serial communication, sets up Wi-Fi and MQTT connectivity,
 * and boots the crop monitoring station application. The device will begin
 * the tickless sleep cycle after initialization, publishing telemetry via
 * MQTT on each sensor reading.
 */
void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    Serial.println(F("[CropMonitoringStation] Starting..."));

    // Set up Wi-Fi connectivity
    transportDriver = new WiFiConnectivityDriver(WIRELESS_SSID, WIRELESS_PASSWORD);

    // Set up MQTT gateway client for telemetry publishing
    gatewayClient = new MqttGatewayClient(
        *transportDriver,
        transportDriver->getNetworkSocket(),
        MQTT_BROKER_IP,
        MQTT_BROKER_PORT,
        MQTT_PUBLISH_TOPIC,
        MQTT_CLIENT_ID
    );

    // Boot the framework application with tickless power mode and MQTT telemetry
    cropMonitoringApplication = new CropMonitoringStation(
        SOIL_MOISTURE_SENSOR_PIN,
        DHT_SENSOR_PIN,
        *gatewayClient,
        SOIL_SAMPLING_INTERVAL_IN_MILLISECONDS,
        AIR_SAMPLING_INTERVAL_IN_MILLISECONDS,
        LIGHT_SAMPLING_INTERVAL_IN_MILLISECONDS,
        TIMER_CHANNEL
    );
}

/**
 * @brief Arduino loop function.
 *
 * This loop is intentionally empty. The Modest-IoT Nano-Framework manages
 * all timing and sensor sampling via FreeRTOS tasks. In tickless mode,
 * the ESP32 enters light sleep between samples, waking only when needed.
 */
void loop() {
    // The master loop is completely clean and empty.
    // The framework handles all timing, sensor sampling, and light sleep cycles.
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}
