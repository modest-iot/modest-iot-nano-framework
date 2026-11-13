#ifndef LOCATION_DATA_H
#define LOCATION_DATA_H

#include <cmath>

/**
 * @file LocationData.h
 * @brief Declares the LocationData struct for GPS navigation state.
 * @ingroup sensors_module
 * @copyright (c) 2025-2026 Angel Velasquez
 * @license Apache-2.0
 * @date 2026-06-06
 * @version 1.0
 */

/**
 * @brief Comprehensive container for GPS navigation data parsed from NMEA sentences.
 * @ingroup sensors_module
 */
struct LocationData {
    /** @name Position */
    //@{
    float latitudeInDegrees;  ///< Latitude in decimal degrees.
    float longitudeInDegrees; ///< Longitude in decimal degrees.
    float altitudeInMeters;   ///< Altitude in meters above sea level.
    //@} // Position

    /** @name Movement */
    //@{
    float speedInKnots;   ///< Speed over ground in knots.
    float courseInDegrees; ///< Course over ground in degrees (0–360).
    //@} // Movement

    /** @name Fix Quality & Precision */
    //@{
    int fixQuality;                  ///< 0: Invalid, 1: GPS Fix, 2: DGPS Fix.
    int satellitesTracked;           ///< Number of satellites in view.
    float horizontalDilutionOfPrecision; ///< HDOP value.
    //@} // Fix Quality & Precision

    /** @name Time & Date */
    //@{
    int year;         ///< Year in YY format.
    int month;        ///< Month (1–12).
    int day;          ///< Day of month (1–31).
    int hour;         ///< Hour (0–23, UTC).
    int minute;       ///< Minute (0–59).
    int second;       ///< Second (0–59).
    int milliseconds; ///< Milliseconds (0–999).
    //@} // Time & Date

    /** @name Status */
    //@{
    bool isDataValid; ///< True if status is 'A' (Active/Valid).
    //@} // Status

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
