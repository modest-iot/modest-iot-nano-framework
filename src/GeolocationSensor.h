#ifndef GEOLOCATION_SENSOR_H
#define GEOLOCATION_SENSOR_H

/**
 * @file GeolocationSensor.h
 * @brief Declares a GPS/NMEA geolocation sensor that parses serial NMEA sentences.
 * @ingroup sensors_module
 * @date 2026-06-06
 * @version 1.0
 */

#include <HardwareSerial.h>
#include "LocationData.h"
#include "Sensor.h"

/**
 * @brief GPS/NMEA geolocation sensor that parses serial NMEA sentences.
 * @ingroup sensors_module
 */
class GeolocationSensor : public Sensor {
private:
    // Constants to avoid magic strings
    static constexpr char           GPGGA_SENTENCE_HEADER[]             = "$GPGGA";
    static constexpr char           GPRMC_SENTENCE_HEADER[]             = "$GPRMC";
    static constexpr auto           NMEA_DELIMITER                      = ",";
    static constexpr int            MAX_NMEA_SIZE                       = 82;
    static constexpr int            NMEA_HEADER_LENGTH                  = 6;
    static constexpr float          COORDINATE_CONVERSION_DIVISOR       = 100.0f;
    static constexpr float          MINUTES_PER_DEGREE                  = 60.0f;
    static constexpr int            TIME_DATE_PART_LENGTH               = 2;
    static constexpr char           NMEA_NORTH                          = 'N';
    static constexpr char           NMEA_EAST                           = 'E';
    static constexpr char           NMEA_SOUTH                          = 'S';
    static constexpr char           NMEA_WEST                           = 'W';
    static constexpr char           NMEA_VALID_STATUS                   = 'A';
    static constexpr int            PROPAGATION_SUPPRESSION_IDENTIFIER  = -1;

    /// Default sampling interval in milliseconds.
    /// @note GPS modules typically update at 1 Hz (1000 ms).
    static constexpr uint32_t DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS = 1000;
    /// Minimum sampling interval in milliseconds.
    /// @note Most GPS modules cannot update faster than 100 ms.
    static constexpr uint32_t MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS = 100;

    int transmitGpioPin;       ///< GPIO pin used for UART transmission.
    HardwareSerial& gpsSerial; ///< Reference to the hardware serial interface for GPS.
    
    char nmeaBuffer[MAX_NMEA_SIZE];
    int bufferIndex;

    LocationData cachedLocation;
    LocationData lastNotifiedLocation;

    /**
     * @brief Processes available serial data and updates the internal buffer.
     * @return True if a new NMEA sentence was parsed, false otherwise.
     */
    bool updateInternalBuffer();

    /**
     * @brief Dispatches the sentence to specific parser handlers.
     * @return True if a valid sentence was parsed, false otherwise.
     */
    bool parseNmeaSentence(char* sentence);
    
    /**
     * @brief Parses $GPGGA (Fix Data) sentence.
     */
    void parseGgaSentence(char* sentence);
    
    /**
     * @brief Parses $GPRMC (Recommended Minimum Data) sentence.
     */
    void parseRmcSentence(char* sentence);
    
    /**
     * @brief Helper to extract the next token from the NMEA sentence.
     */
    static char* getNextToken(char** saveptr);

    /**
     * @brief Helper to extract the next token and convert to float.
     */
    static float getNextTokenAsFloat(char** saveptr);

    /**
     * @brief Helper to extract the next token and convert to int.
     */
    static int getNextTokenAsInt(char** saveptr);

    /**
     * @brief Converts NMEA coordinate format (DDMM.MMMM) to decimal degrees.
     */
    static float convertToDecimalDegrees(float rawCoordinate, char direction);

protected:
    void processEvent(Event& event) override;

    [[nodiscard]] uint32_t getDefaultSamplingIntervalInMilliseconds() const override;
    [[nodiscard]] uint32_t getMinimalSamplingIntervalInMilliseconds() const override;

public:
    /**
     * @brief Constructs a GPS sensor that reads NMEA sentences over a hardware serial port.
     * @param serial Hardware serial instance connected to the GPS module.
     * @param receiveGpioPin GPIO pin for UART receive (RX).
     * @param transmitGpioPin GPIO pin for UART transmit (TX).
     * @param baudRate Serial baud rate, typically 9600 for GPS modules.
     * @param parentHandler Optional upstream handler for propagated events.
     */
    GeolocationSensor(HardwareSerial& serial, int receiveGpioPin, int transmitGpioPin, long baudRate = 9600, EventHandler* parentHandler = nullptr);

    /**
     * @brief Returns the full navigation state.
     * @return A constant reference to the current LocationData.
     */
    [[nodiscard]] const LocationData& getLocationData() const { return cachedLocation; }
};

#endif // GEOLOCATION_SENSOR_H
