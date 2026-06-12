#include "GeolocationSensor.h"
#include <string.h>
#include <stdlib.h>

GeolocationSensor::GeolocationSensor(int receiveGpioPin, int transmitGpioPin, long baudRate, EventHandler* parentHandler)
    : Sensor(receiveGpioPin, parentHandler), 
      transmitGpioPin(transmitGpioPin), 
      gpsSerial(receiveGpioPin, transmitGpioPin),
      bufferIndex(0) {
    // Initialize cachedLocation with defaults
    cachedLocation = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0.0f, 0, 0, 0, 0, 0, 0, 0, false};
    lastNotifiedLocation = cachedLocation;
    gpsSerial.begin(baudRate);
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
        char c = gpsSerial.read();
        if (c == '\n' || c == '\r') {
            if (bufferIndex > 0) {
                nmeaBuffer[bufferIndex] = '\0';
                parseNmeaSentence(nmeaBuffer);
                bufferIndex = 0;
                dataUpdated = true;
            }
        } else if (bufferIndex < MAX_NMEA_SIZE - 1) {
            nmeaBuffer[bufferIndex++] = c;
        }
    }
    return dataUpdated;
}

void GeolocationSensor::parseNmeaSentence(char* sentence) {
    if (strncmp(sentence, GPGGA_SENTENCE_HEADER, NMEA_HEADER_LENGTH) == 0) {
        parseGgaSentence(sentence);
    } else if (strncmp(sentence, GPRMC_SENTENCE_HEADER, NMEA_HEADER_LENGTH) == 0) {
        parseRmcSentence(sentence);
    }
}

void GeolocationSensor::parseGgaSentence(char* sentence) {
    // $GPGGA,Time,Lat,N/S,Lon,E/W,FixQuality,Satellites,HDOP,Alt,M,GeoidSep,M,Age,RefStation
    strtok(sentence, NMEA_DELIMITER); // Skip ID
    getNextToken(); // Skip Time
    
    float rawLatitude = getNextTokenAsFloat();
    char* latitudeDirectionToken = getNextToken();
    char latitudeDirection = (latitudeDirectionToken != NULL) ? latitudeDirectionToken[0] : NMEA_NORTH;
    
    float rawLongitude = getNextTokenAsFloat();
    char* longitudeDirectionToken = getNextToken();
    char longitudeDirection = (longitudeDirectionToken != NULL) ? longitudeDirectionToken[0] : NMEA_EAST;
    
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
    
    cachedLocation.isDataValid = (statusToken != NULL && statusToken[0] == NMEA_VALID_STATUS);
    
    // Time (HHMMSS.sss)
    if (timeToken != NULL) {
        char buffer[TIME_DATE_PART_LENGTH + 1];
        buffer[TIME_DATE_PART_LENGTH] = '\0';
        
        strncpy(buffer, timeToken, TIME_DATE_PART_LENGTH); 
        cachedLocation.hour = atoi(buffer);
        
        strncpy(buffer, timeToken + TIME_DATE_PART_LENGTH, TIME_DATE_PART_LENGTH); 
        cachedLocation.minute = atoi(buffer);
        
        strncpy(buffer, timeToken + (TIME_DATE_PART_LENGTH * 2), TIME_DATE_PART_LENGTH); 
        cachedLocation.second = atoi(buffer);
        
        char* dot = strchr(timeToken, '.');
        if (dot) cachedLocation.milliseconds = atoi(dot + 1);
    }
    
    getNextToken(); // Lat
    getNextToken(); // N/S
    getNextToken(); // Lon
    getNextToken(); // E/W
    
    cachedLocation.speedInKnots = getNextTokenAsFloat();
    cachedLocation.courseInDegrees = getNextTokenAsFloat();
    
    char* dateToken = getNextToken();
    // Date (DDMMYY)
    if (dateToken != NULL) {
        char buffer[TIME_DATE_PART_LENGTH + 1];
        buffer[TIME_DATE_PART_LENGTH] = '\0';
        
        strncpy(buffer, dateToken, TIME_DATE_PART_LENGTH); 
        cachedLocation.day = atoi(buffer);
        
        strncpy(buffer, dateToken + TIME_DATE_PART_LENGTH, TIME_DATE_PART_LENGTH); 
        cachedLocation.month = atoi(buffer);
        
        strncpy(buffer, dateToken + (TIME_DATE_PART_LENGTH * 2), TIME_DATE_PART_LENGTH); 
        cachedLocation.year = atoi(buffer);
    }
}

float GeolocationSensor::convertToDecimalDegrees(float rawCoordinate, char direction) {
    int degrees = (int)(rawCoordinate / COORDINATE_CONVERSION_DIVISOR);
    float minutes = rawCoordinate - (degrees * COORDINATE_CONVERSION_DIVISOR);
    float decimal = degrees + (minutes / MINUTES_PER_DEGREE);
    if (direction == NMEA_SOUTH || direction == NMEA_WEST) decimal = -decimal;
    return decimal;
}

char* GeolocationSensor::getNextToken() {
    return strtok(NULL, NMEA_DELIMITER);
}

float GeolocationSensor::getNextTokenAsFloat() {
    char* token = getNextToken();
    return (token != NULL) ? atof(token) : 0.0f;
}

int GeolocationSensor::getNextTokenAsInt() {
    char* token = getNextToken();
    return (token != NULL) ? atoi(token) : 0;
}

