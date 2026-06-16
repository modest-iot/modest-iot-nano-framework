#ifndef LOCATION_DATA_H
#define LOCATION_DATA_H

#include <cmath>

/**
 * @brief Comprehensive container for GPS navigation data.
 * @ingroup sensors_module
 */
struct LocationData {
    // --- Position ---
    float latitudeInDegrees;
    float longitudeInDegrees;
    float altitudeInMeters;

    // --- Movement ---
    float speedInKnots;
    float courseInDegrees;

    // --- Fix Quality & Precision ---
    int fixQuality;           ///< 0: Invalid, 1: GPS Fix, 2: DGPS Fix
    int satellitesTracked;
    float horizontalDilutionOfPrecision; ///< HDOP

    // --- Time & Date ---
    int year;                 ///< YY format
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int milliseconds;

    // --- Status ---
    bool isDataValid;         ///< True if status is 'A' (Active/Valid)

    /**
     * @brief Checks if this location data is equal to another.
     * @param other The location data to compare against.
     * @return True if all fields are identical, false otherwise.
     */
    bool operator==(const LocationData& other) const {
        return (floatEquals(latitudeInDegrees, other.latitudeInDegrees) &&
                floatEquals(longitudeInDegrees, other.longitudeInDegrees) &&
                floatEquals(altitudeInMeters, other.altitudeInMeters) &&
                floatEquals(speedInKnots, other.speedInKnots) &&
                floatEquals(courseInDegrees, other.courseInDegrees) &&
                fixQuality == other.fixQuality &&
                satellitesTracked == other.satellitesTracked &&
                floatEquals(horizontalDilutionOfPrecision, other.horizontalDilutionOfPrecision) &&
                year == other.year &&
                month == other.month &&
                day == other.day &&
                hour == other.hour &&
                minute == other.minute &&
                second == other.second &&
                milliseconds == other.milliseconds &&
                isDataValid == other.isDataValid);
    }

private:
    static bool floatEquals(float a, float b) {
        return std::fabs(a - b) < 0.0001f;
    }
};

#endif // LOCATION_DATA_H
