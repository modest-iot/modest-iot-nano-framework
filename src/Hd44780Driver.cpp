/**
 * @file Hd44780Driver.cpp
 * @brief Implements the HD44780 4-bit mode I2C protocol using PCF8574 backpack.
 * Native implementation eliminating the LiquidCrystal_I2C external dependency (W19).
 * @date 2026-11-09
 * @version 1.0
 */

#include "Hd44780Driver.h"
#include <Arduino.h>

Hd44780Driver::Hd44780Driver(const uint8_t i2cAddress, const bool backlightInitialState)
    : i2cAddress(i2cAddress),
      backlightEnabled(backlightInitialState) {
}

void Hd44780Driver::write4Bit(const uint8_t value) const {
    // Send a single 4-bit nibble to the HD44780 via PCF8574.
    // The PCF8574 maps P0-P3 to HD44780 data lines D4-D7.
    // P4 is the backlight, P5 is the enable pulse, P0 is RS (register select).
    Wire.beginTransmission(i2cAddress);
    Wire.write(value);
    (void)Wire.endTransmission(false);
    delayMicroseconds(HD44780_ENABLE_SETUP_TIME_IN_MICROSECONDS);

    // Generate enable pulse: toggle P5 (enable bit)
    Wire.beginTransmission(i2cAddress);
    Wire.write(value | PCF8574_ENABLE_BIT);
    (void)Wire.endTransmission(false);
    delayMicroseconds(HD44780_ENABLE_PULSE_WIDTH_IN_MICROSECONDS);

    Wire.beginTransmission(i2cAddress);
    Wire.write(value & ~PCF8574_ENABLE_BIT);
    (void)Wire.endTransmission(true);
    delayMicroseconds(HD44780_ENABLE_RECOVERY_TIME_IN_MICROSECONDS);
}

void Hd44780Driver::sendCommand(const uint8_t command) const {
    // RS=0 for command mode. Send high nibble, then low nibble.
    write4Bit((command & HD44780_HIGH_NIBBLE_MASK) | (backlightEnabled ? PCF8574_BACKLIGHT_BIT : HD44780_NO_BACKLIGHT));
    write4Bit((command & HD44780_LOW_NIBBLE_MASK) << 4 | (backlightEnabled ? PCF8574_BACKLIGHT_BIT : HD44780_NO_BACKLIGHT));
}

void Hd44780Driver::sendData(const uint8_t data) const {
    // RS=1 for data mode. Send high nibble, then low nibble.
    write4Bit((data & HD44780_HIGH_NIBBLE_MASK) | PCF8574_REGISTER_SELECT_BIT | (backlightEnabled ? PCF8574_BACKLIGHT_BIT : HD44780_NO_BACKLIGHT));
    write4Bit((data & HD44780_LOW_NIBBLE_MASK) << 4 | PCF8574_REGISTER_SELECT_BIT | (backlightEnabled ? PCF8574_BACKLIGHT_BIT : HD44780_NO_BACKLIGHT));
}

void Hd44780Driver::setBacklight(const bool backlightState) {
    backlightEnabled = backlightState;
    // Update backlight immediately by sending a no-op command with the new backlight state.
    Wire.beginTransmission(i2cAddress);
    Wire.write(backlightState ? PCF8574_BACKLIGHT_BIT : HD44780_NO_BACKLIGHT);
    (void)Wire.endTransmission(true);
}

bool Hd44780Driver::isBacklightOn() const {
    return backlightEnabled;
}

void Hd44780Driver::initialize() const {
    // HD44780 initialization sequence for 4-bit mode.
    // The LCD starts in 8-bit mode; we must transition to 4-bit mode.

    delayMicroseconds(HD44780_POWER_ON_STABILIZE_TIME_IN_MICROSECONDS);

    // Initial function set: HD44780_FUNCTION_SET_8BIT sent three times to enter 4-bit mode
    Wire.beginTransmission(i2cAddress);
    Wire.write(HD44780_FUNCTION_SET_8BIT | (backlightEnabled ? PCF8574_BACKLIGHT_BIT : HD44780_NO_BACKLIGHT));
    (void)Wire.endTransmission(true);
    delayMicroseconds(HD44780_FIRST_FUNCTION_SET_DELAY_IN_MICROSECONDS);

    Wire.beginTransmission(i2cAddress);
    Wire.write(HD44780_FUNCTION_SET_8BIT | (backlightEnabled ? PCF8574_BACKLIGHT_BIT : HD44780_NO_BACKLIGHT));
    (void)Wire.endTransmission(true);
    delayMicroseconds(HD44780_SECOND_FUNCTION_SET_DELAY_IN_MICROSECONDS);

    Wire.beginTransmission(i2cAddress);
    Wire.write(HD44780_FUNCTION_SET_8BIT | (backlightEnabled ? PCF8574_BACKLIGHT_BIT : HD44780_NO_BACKLIGHT));
    (void)Wire.endTransmission(true);
    delayMicroseconds(HD44780_THIRD_FUNCTION_SET_DELAY_IN_MICROSECONDS);

    // Set 4-bit mode: HD44780_FUNCTION_SET_4BIT
    Wire.beginTransmission(i2cAddress);
    Wire.write(HD44780_FUNCTION_SET_4BIT | (backlightEnabled ? PCF8574_BACKLIGHT_BIT : HD44780_NO_BACKLIGHT));
    (void)Wire.endTransmission(true);
    delayMicroseconds(HD44780_SET_4BIT_MODE_DELAY_IN_MICROSECONDS);

    // Now in 4-bit mode, use normal command sequence.
    sendCommand(HD44780_FUNCTION_SET_4BIT_2LINE_5X8);
    sendCommand(HD44780_DISPLAY_ON_CURSOR_OFF_BLINK_OFF);
    sendCommand(HD44780_CLEAR_DISPLAY);
    delayMicroseconds(HD44780_CLEAR_DISPLAY_DELAY_IN_MICROSECONDS);
    sendCommand(HD44780_ENTRY_MODE_INCREMENT_NO_SHIFT);
}
