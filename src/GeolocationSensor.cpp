#include "GeolocationSensor.h"

GeolocationSensor::GeolocationSensor(HardwareSerial& serial, int receiveGpioPin, int transmitGpioPin, long baudRate, EventHandler* parentHandler)
    : Sensor(receiveGpioPin, parentHandler), 
      transmitGpioPin(transmitGpioPin),
      gpsSerial(serial),
      nmeaBuffer{'\0'},
      bufferIndex(0),
      cachedLocation{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0.0f, 0, 0, 0, 0, 0, 0, 0, false},
      lastNotifiedLocation{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0.0f, 0, 0, 0, 0, 0, 0, 0, false} {
    gpsSerial.begin(baudRate, SERIAL_8N1, static_cast<int8_t>(receiveGpioPin), static_cast<int8_t>(transmitGpioPin));
}

void GeolocationSensor::processEvent(Event& event) {
    if (event.identifier == MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER) {
        if (updateInternalBuffer()) {
            if (cachedLocation == lastNotifiedLocation) {
                event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
            } else {
                lastNotifiedLocation = cachedLocation;
                event = Event(DATA_READ_EVENT_IDENTIFIER, pin);
            }
        }
    }
}

bool GeolocationSensor::updateInternalBuffer() {
    bool dataUpdated = false;
    while (gpsSerial.available() > 0) {
        char c = 0;
        c = static_cast<char>(gpsSerial.read());
        if (c == '\n' || c == '\r') {
            if (bufferIndex > 0) {
                nmeaBuffer[bufferIndex] = '\0';
                if (parseNmeaSentence(nmeaBuffer)) {
                    dataUpdated = true;
                }
                bufferIndex = 0;
            }
        } else if (bufferIndex < MAX_NMEA_SIZE - 1) {
            nmeaBuffer[bufferIndex++] = c;
        }
    }
    return dataUpdated;
}

bool GeolocationSensor::parseNmeaSentence(char* sentence) {
    if (strncmp(sentence, GPGGA_SENTENCE_HEADER, NMEA_HEADER_LENGTH) == 0) {
        parseGgaSentence(sentence);
        return true;
    } else if (strncmp(sentence, GPRMC_SENTENCE_HEADER, NMEA_HEADER_LENGTH) == 0) {
        parseRmcSentence(sentence);
        return true;
    }
    return false;
}

void GeolocationSensor::parseGgaSentence(char* sentence) {
    // $GPGGA,Time,Lat,N/S,Lon,E/W,FixQuality,Satellites,HDOP,Alt,M,GeoidSep,M,Age,RefStation
    strtok(sentence, NMEA_DELIMITER); // Skip ID
    getNextToken(); // Skip Time
    
    float rawLatitude = getNextTokenAsFloat();
    char* latitudeDirectionToken = getNextToken();
    char latitudeDirection = (latitudeDirectionToken != nullptr) ? latitudeDirectionToken[0] : NMEA_NORTH;
    
    float rawLongitude = getNextTokenAsFloat();
    char* longitudeDirectionToken = getNextToken();
    char longitudeDirection = (longitudeDirectionToken != nullptr) ? longitudeDirectionToken[0] : NMEA_EAST;
    
    cachedLocation.fixQuality = getNextTokenAsInt();
    cachedLocation.satellitesTracked = getNextTokenAsInt();
    cachedLocation.horizontalDilutionOfPrecision = getNextTokenAsFloat();
    cachedLocation.altitudeInMeters = getNextTokenAsFloat();

    cachedLocation.latitudeInDegrees = convertToDecimalDegrees(rawLatitude, latitudeDirection);
    cachedLocation.longitudeInDegrees = convertToDecimalDegrees(rawLongitude, longitudeDirection);
}

void GeolocationSensor::parseRmcSentence(char* sentence) {
    // $GPRMC,Time,Status,Lat,N/S,Lon,E/W,Speed,Course,Date,...
    strtok(sentence, NMEA_DELIMITER); // Skip ID
    char* timeToken = getNextToken();
    char* statusToken = getNextToken();
    
    cachedLocation.isDataValid = (statusToken != nullptr && statusToken[0] == NMEA_VALID_STATUS);
    
    // Time (HHMMSS.sss)
    if (timeToken != nullptr) {
        char buffer[TIME_DATE_PART_LENGTH + 1];
        buffer[TIME_DATE_PART_LENGTH] = '\0';
        
        strncpy(buffer, timeToken, TIME_DATE_PART_LENGTH); 
        cachedLocation.hour = static_cast<int>(strtol(buffer, nullptr, 10));
        
        strncpy(buffer, timeToken + TIME_DATE_PART_LENGTH, TIME_DATE_PART_LENGTH); 
        cachedLocation.minute = static_cast<int>(strtol(buffer, nullptr, 10));
        
        strncpy(buffer, timeToken + (TIME_DATE_PART_LENGTH * 2), TIME_DATE_PART_LENGTH); 
        cachedLocation.second = static_cast<int>(strtol(buffer, nullptr, 10));
        
        if (char* dot = strchr(timeToken, '.')) cachedLocation.milliseconds = static_cast<int>(strtol(dot + 1, nullptr, 10));
    }
    
    getNextToken(); // Lat
    getNextToken(); // N/S
    getNextToken(); // Lon
    getNextToken(); // E/W
    
    cachedLocation.speedInKnots = getNextTokenAsFloat();
    cachedLocation.courseInDegrees = getNextTokenAsFloat();
    
    char* dateToken = getNextToken();
    // Date (DDMMYY)
    if (dateToken != nullptr) {
        char buffer[TIME_DATE_PART_LENGTH + 1];
        buffer[TIME_DATE_PART_LENGTH] = '\0';
        
        strncpy(buffer, dateToken, TIME_DATE_PART_LENGTH); 
        cachedLocation.day = static_cast<int>(strtol(buffer, nullptr, 10));
        
        strncpy(buffer, dateToken + TIME_DATE_PART_LENGTH, TIME_DATE_PART_LENGTH); 
        cachedLocation.month = static_cast<int>(strtol(buffer, nullptr, 10));
        
        strncpy(buffer, dateToken + (TIME_DATE_PART_LENGTH * 2), TIME_DATE_PART_LENGTH); 
        cachedLocation.year = static_cast<int>(strtol(buffer, nullptr, 10));
    }
}

float GeolocationSensor::convertToDecimalDegrees(float rawCoordinate, char direction) {
    int degrees = static_cast<int>(rawCoordinate / COORDINATE_CONVERSION_DIVISOR);
    float minutes = rawCoordinate - (static_cast<float>(degrees) * COORDINATE_CONVERSION_DIVISOR);
    float decimal = static_cast<float>(degrees) + (minutes / MINUTES_PER_DEGREE);
    if (direction == NMEA_SOUTH || direction == NMEA_WEST) decimal = -decimal;
    return decimal;
}

char* GeolocationSensor::getNextToken() {
    return strtok(nullptr, NMEA_DELIMITER);
}

float GeolocationSensor::getNextTokenAsFloat() {
    char* token = getNextToken();
    return (token != nullptr) ? static_cast<float>(strtod(token, nullptr)) : 0.0f;
}

int GeolocationSensor::getNextTokenAsInt() {
    char* token = getNextToken();
    return (token != nullptr) ? static_cast<int>(strtol(token, nullptr, 10)) : 0;
}

