#ifndef GEOLOCATION_SENSOR_H
#define GEOLOCATION_SENSOR_H

#include "Sensor.h"
#include "LocationData.h"
#include <SoftwareSerial.h>

/**
 * @brief GPS/Geolocation Sensor abstraction.
 * @ingroup sensors_module
 */
class GeolocationSensor : public Sensor {
private:
    // Constants to avoid magic strings
    static constexpr auto           GPGGA_SENTENCE_HEADER               = "$GPGGA";
    static constexpr auto           GPRMC_SENTENCE_HEADER               = "$GPRMC";
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

    int transmitGpioPin;       ///< GPIO pin used for UART transmission.
    SoftwareSerial gpsSerial;  ///< Software serial interface for GPS.
    
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
     */
    void parseNmeaSentence(char* sentence);
    
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
    char* getNextToken();

    /**
     * @brief Helper to extract the next token and convert to float.
     */
    float getNextTokenAsFloat();

    /**
     * @brief Helper to extract the next token and convert to int.
     */
    int getNextTokenAsInt();

    /**
     * @brief Converts NMEA coordinate format (DDMM.MMMM) to decimal degrees.
     */
    float convertToDecimalDegrees(float rawCoordinate, char direction);

protected:
    void processEvent(Event& event) override;

public:
    GeolocationSensor(int receiveGpioPin, int transmitGpioPin, long baudRate = 9600, EventHandler* parentHandler = nullptr);

    /**
     * @brief Returns the full navigation state.
     * @return A constant reference to the current LocationData.
     */
    const LocationData& getLocationData() const { return cachedLocation; }
};

#endif // GEOLOCATION_SENSOR_H
