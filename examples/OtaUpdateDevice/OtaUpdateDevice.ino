/**
 * @file OtaUpdateDevice.ino
 * @brief Demonstrates Over-The-Air firmware updates with progress tracking.
 *
 * This example shows how to enable OTA firmware updates on an ESP32 device
 * using the OtaUpdateHandler component. The device connects to Wi-Fi, starts
 * an OTA listener with hostname and password protection, and reports update
 * progress via Serial. A DHT22 sensor provides basic telemetry to demonstrate
 * a realistic device that can be updated remotely.
 *
 * @par Modest IoT Nano Framework aspects illustrated:
 * - `OtaUpdateHandler` for OTA firmware updates via Wi-Fi
 * - `OtaUpdateHandler::begin()` with hostname configuration
 * - `OtaUpdateHandler::setPassword()` for password protection
 * - `OtaUpdateHandler::onProgress()` for progress tracking
 * - `OtaUpdateHandler::onError()` for error reporting
 * - `OtaUpdateHandler::onStart()` and `onEnd()` for lifecycle callbacks
 * - `OtaUpdateHandler::service()` called from `maintainNetworkServices()`
 * - `Device::setFirmwareVersion()` for fleet management in telemetry
 * - `DhtSensor` for temperature and humidity measurement
 * - `WiFiConnectivityDriver` with `waitForConnection()` timeout
 * - Loop-less, event-driven asynchronous execution via `initializeAsynchronousEngine()`
 *
 * @par Flashing via OTA
 * 1. First, flash the device normally via USB.
 * 2. Note the IP address printed to Serial.
 * 3. In Arduino IDE, select the device from Tools > Port (it appears as
 *    "modest-iot-ota.local" or by IP address).
 * 4. Upload new firmware as usual — the IDE uses OTA automatically.
 *
 * @date 2026-11-15
 * @version 1.1
 */

#include <Arduino.h>
#include <ModestIoT.h>

/** @name Network Configuration */
//@{
static const char* WIRELESS_SSID = "ModestIoT_Network";
static const char* WIRELESS_PASSWORD = "SecureMesh2026";
//@} // Network Configuration

/** @name OTA Configuration */
//@{
/** @brief Device hostname for OTA discovery on the network. */
static const char* OTA_HOSTNAME = "modest-iot-ota";

/** @brief Password required for OTA update authentication. */
static const char* OTA_PASSWORD = "ota-secret-2026";
//@} // OTA Configuration

/** @name Sensor Configuration */
//@{
static const int DHT_SENSOR_PIN = 4;
static const unsigned long TELEMETRY_POLLING_INTERVAL_IN_MILLISECONDS = 60000;
static const uint8_t HARDWARE_TIMER_CHANNEL_INDEX = 0;
//@} // Sensor Configuration

/** @name Timing Configuration */
//@{
static const unsigned long MAIN_LOOP_DELAY_IN_MILLISECONDS = 60000;
static const unsigned long CONNECTION_TIMEOUT_IN_MILLISECONDS = 15000;
//@} // Timing Configuration

/** @name Framework Configuration */
//@{
static const int SAMPLER_QUEUE_LENGTH = 5;
static const unsigned long SERIAL_BAUD_RATE = 115200;
//@} // Framework Configuration

/** @name Serial Output Configuration */
//@{
/** @brief Log prefix for all OtaUpdateDevice serial output. */
static const char* OTA_UPDATE_DEVICE_LOG_PREFIX = "[OtaUpdateDevice]";
/** @brief Buffer size for formatted sensor readings. */
static const size_t SERIAL_OUTPUT_BUFFER_SIZE = 128;
//@} // Serial Output Configuration

/**
 * @brief OTA Update Device.
 * Demonstrates OTA firmware updates with a simple DHT22 sensor.
 * The device reports temperature and humidity, and can receive
 * firmware updates over Wi-Fi.
 */
class OtaUpdateDevice : public Device {
private:
    DhtSensor dhtSensor;

protected:
    void processQueuedTelemetryData(const TelemetryPackage* rawQueueItemPayload) const override {
        // No gateway client in this example — telemetry is logged to Serial only.
        // In a real device, you would send the payload via MQTT or HTTP.
        if (rawQueueItemPayload != nullptr) {
            Serial.println((String(OTA_UPDATE_DEVICE_LOG_PREFIX) + " Telemetry ready (logged locally only).").c_str());
        }
    }

    /**
     * @brief Services network components including OTA update handler.
     * This is where OtaUpdateHandler::service() is called to process
     * incoming OTA update requests.
     */
    void maintainNetworkServices() const override {
        OtaUpdateHandler::service();
    }

public:
    OtaUpdateDevice(int dhtPin,
                    unsigned long samplingIntervalInMilliseconds,
                    uint8_t timerGroupIndex = 0)
        : Device(samplingIntervalInMilliseconds, timerGroupIndex),
          dhtSensor(dhtPin, DhtSensor::SENSOR_MODEL_DHT22, this)
    {
        // IMPORTANT: Registration must occur before initializeAsynchronousEngine(), because
        // the engine start guard rejects any registration after the engine is running.
        appendSensorToScheduler(&dhtSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);

        if (!initializeAsynchronousEngine(SAMPLER_QUEUE_LENGTH)) {
            return;
        }
    }

    ~OtaUpdateDevice() override {
        stopAsynchronousEngine();
    }

    /**
     * @brief Handles sensor data-ready events to log readings.
     * @param event The triggered event context payload.
     * @runs_on{sampler}
     */
    void on(Event event) override {
        if (event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER) {
            char buffer[SERIAL_OUTPUT_BUFFER_SIZE];
            snprintf(buffer, sizeof(buffer),
                     "%s Temp: %.1f°C, Humidity: %.1f%%",
                     OTA_UPDATE_DEVICE_LOG_PREFIX,
                     dhtSensor.getTemperatureInCelsius(),
                     dhtSensor.getRelativeHumidityInPercentage());
            Serial.println(buffer);
        }
    }

    void handle(Command command) override {}

    /**
     * @brief Initializes OTA update handler with hostname, password,
     * and all callback types (progress, error, start, end).
     * @param hostname The device hostname for OTA discovery.
     * @param password The password required for OTA authentication.
     */
    void configureOtaUpdate(const char* hostname, const char* password) {
        OtaUpdateHandler::begin(hostname);
        OtaUpdateHandler::setPassword(password);

        OtaUpdateHandler::onProgress([](uint32_t progressInNumberOfBytesWritten,
                                         uint32_t totalNumberOfBytes) {
            float percent = static_cast<float>(progressInNumberOfBytesWritten)
                           / static_cast<float>(totalNumberOfBytes) * 100.0f;
            Serial.printf("%s OTA Progress: %.1f%%\r",
                          OTA_UPDATE_DEVICE_LOG_PREFIX, percent);
        });

        OtaUpdateHandler::onError([](int errorCode) {
            Serial.printf("%s OTA Error: %d\n",
                          OTA_UPDATE_DEVICE_LOG_PREFIX, errorCode);
        });

        OtaUpdateHandler::onStart([]() {
            Serial.println((String("\n") + OTA_UPDATE_DEVICE_LOG_PREFIX + " OTA Update started...").c_str());
        });

        OtaUpdateHandler::onEnd([]() {
            Serial.println((String("\n") + OTA_UPDATE_DEVICE_LOG_PREFIX + " OTA Update ended.").c_str());
        });

        Serial.printf("%s OTA updates enabled on hostname: %s\n",
                      OTA_UPDATE_DEVICE_LOG_PREFIX, hostname);
        Serial.println((String(OTA_UPDATE_DEVICE_LOG_PREFIX) + " To update firmware, select this device from Arduino IDE > Tools > Port").c_str());
    }

    /**
     * @brief Connects to Wi-Fi and initializes OTA updates.
     * This method encapsulates all setup logic so the sketch's setup()
     * is only responsible for instance creation.
     * @param transport The Wi-Fi connectivity driver for network access.
     * @param hostname The device hostname for OTA discovery.
     * @param password The password required for OTA authentication.
     * @param connectionTimeoutInMilliseconds Maximum time to wait for Wi-Fi connection.
     * @return True if all initialization steps succeeded.
     */
    bool start(WiFiConnectivityDriver& transport,
                const char* hostname,
                const char* password,
                unsigned long connectionTimeoutInMilliseconds)
    {
        if (!isAsynchronousEngineStarted()) {
            return false;
        }
        // Set firmware version for fleet management in telemetry
        Device::setFirmwareVersion("MOD-OTA-01 FW 1.0.0");

        // Connect to Wi-Fi with timeout
        Serial.println((String(OTA_UPDATE_DEVICE_LOG_PREFIX) + " Connecting to Wi-Fi...").c_str());
        if (!transport.waitForConnection(connectionTimeoutInMilliseconds)) {
            Serial.println((String(OTA_UPDATE_DEVICE_LOG_PREFIX) + " Wi-Fi connection timed out").c_str());
            return false;
        }

        Serial.printf("%s Connected! IP: %s\n",
                      OTA_UPDATE_DEVICE_LOG_PREFIX,
                      transport.getLocalIpAddress().toString().c_str());

        // Configure OTA updates
        configureOtaUpdate(hostname, password);

        return true;
    }
};

/** @name Top-Level Declarative Canvas */
//@{

static WiFiConnectivityDriver* transportDriver = nullptr;
static OtaUpdateDevice* otaDeviceApplication = nullptr;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);

    transportDriver = new WiFiConnectivityDriver(WIRELESS_SSID, WIRELESS_PASSWORD);

    otaDeviceApplication = new OtaUpdateDevice(
        DHT_SENSOR_PIN,
        TELEMETRY_POLLING_INTERVAL_IN_MILLISECONDS,
        HARDWARE_TIMER_CHANNEL_INDEX
    );

    if (!otaDeviceApplication->start(*transportDriver, OTA_HOSTNAME, OTA_PASSWORD,
                                     CONNECTION_TIMEOUT_IN_MILLISECONDS)) {
        Serial.println((String(OTA_UPDATE_DEVICE_LOG_PREFIX) + " Device startup failed.").c_str());
        return;
    }
}

void loop() {
    // The master loop is completely clean and empty.
    // The framework handles all timing, sensor sampling, and OTA update servicing.
    // vTaskDelay clarifies that this loop yields to FreeRTOS — the device is loopless.
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}
