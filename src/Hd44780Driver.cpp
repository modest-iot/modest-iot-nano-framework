/**
 * @file Hd44780Driver.cpp
 * @brief Implements the HD44780 4-bit mode I2C protocol using PCF8574 backpack.
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-11-09
 * @version 1.0
 */

#include "Hd44780Driver.h"
#include <Arduino.h>

Hd44780Driver::Hd44780Driver(const uint8_t i2cAddress, const bool backlightInitialState)
    : bus(nullptr),
      i2cAddress(i2cAddress),
      backlightEnabled(backlightInitialState),
      initialized(false),
      initializing(false),
      lastI2cResult{I2cStatus::NotStarted, 0, 0} {
}

Hd44780Driver::Hd44780Driver(I2cBus& bus, const uint8_t i2cAddress, const bool backlightInitialState)
    : bus(&bus),
      i2cAddress(i2cAddress),
      backlightEnabled(backlightInitialState),
      initialized(false),
      initializing(false),
      lastI2cResult{I2cStatus::NotStarted, 0, 0} {
}

I2cResult Hd44780Driver::writePortValue(const uint8_t value, const bool stop) const {
    if (bus != nullptr) {
        const I2cResult result = bus->write(i2cAddress, &value, 1);
        lastI2cResult = result;
        return result;
    }

    Wire.beginTransmission(i2cAddress);
    const std::size_t written = Wire.write(value);
    const uint8_t wireStatus = Wire.endTransmission(stop);
    const I2cStatus status = written == 1 ? mapWireStatus(wireStatus) : I2cStatus::InvalidArgument;
    lastI2cResult = {status, status == I2cStatus::Ok ? 1U : 0U, 1};
    return lastI2cResult;
}

bool Hd44780Driver::write4Bit(const uint8_t value) const {
    if (bus != nullptr && !initialized && !initializing) {
        lastI2cResult = {I2cStatus::NotStarted, 0, 1};
        return false;
    }

    // Send a single 4-bit nibble to the HD44780 via PCF8574.
    // The PCF8574 maps P0-P3 to HD44780 data lines D4-D7.
    // P4 is the backlight, P5 is the enable pulse, P0 is RS (register select).
    if (writePortValue(value, false).isInvalidOrIncomplete()) {
        return false;
    }
    delayMicroseconds(HD44780_ENABLE_SETUP_TIME_IN_MICROSECONDS);

    // Generate enable pulse: toggle P5 (enable bit)
    if (writePortValue(value | PCF8574_ENABLE_BIT, false).isInvalidOrIncomplete()) {
        return false;
    }
    delayMicroseconds(HD44780_ENABLE_PULSE_WIDTH_IN_MICROSECONDS);

    if (writePortValue(value & ~PCF8574_ENABLE_BIT).isInvalidOrIncomplete()) {
        return false;
    }
    delayMicroseconds(HD44780_ENABLE_RECOVERY_TIME_IN_MICROSECONDS);
    return true;
}

void Hd44780Driver::sendCommand(const uint8_t command) const {
    // RS=0 for command mode. Send high nibble, then low nibble.
    if (!write4Bit((command & HD44780_HIGH_NIBBLE_MASK) |
                   (backlightEnabled ? PCF8574_BACKLIGHT_BIT : HD44780_NO_BACKLIGHT))) {
        return;
    }
    (void)write4Bit((command & HD44780_LOW_NIBBLE_MASK) << 4 |
                    (backlightEnabled ? PCF8574_BACKLIGHT_BIT : HD44780_NO_BACKLIGHT));
}

void Hd44780Driver::sendData(const uint8_t data) const {
    // RS=1 for data mode. Send high nibble, then low nibble.
    if (!write4Bit((data & HD44780_HIGH_NIBBLE_MASK) | PCF8574_REGISTER_SELECT_BIT |
                   (backlightEnabled ? PCF8574_BACKLIGHT_BIT : HD44780_NO_BACKLIGHT))) {
        return;
    }
    (void)write4Bit((data & HD44780_LOW_NIBBLE_MASK) << 4 | PCF8574_REGISTER_SELECT_BIT |
                    (backlightEnabled ? PCF8574_BACKLIGHT_BIT : HD44780_NO_BACKLIGHT));
}

void Hd44780Driver::setBacklight(const bool backlightState) {
    if (bus != nullptr && !initialized) {
        lastI2cResult = {I2cStatus::NotStarted, 0, 1};
        return;
    }

    // Update backlight immediately by sending a no-op command with the new backlight state.
    const I2cResult result = writePortValue(backlightState ? PCF8574_BACKLIGHT_BIT : HD44780_NO_BACKLIGHT);
    if (!result.isInvalidOrIncomplete()) {
        backlightEnabled = backlightState;
    }
}

bool Hd44780Driver::isBacklightOn() const {
    return backlightEnabled;
}

bool Hd44780Driver::initializeSequence() const {
    initialized = false;
    if (bus != nullptr && !bus->isReady()) {
        lastI2cResult = {I2cStatus::NotStarted, 0, 1};
        return false;
    }
    initializing = true;

    // HD44780 initialization sequence for 4-bit mode.
    // The LCD starts in 8-bit mode; we must transition to 4-bit mode.

    delayMicroseconds(HD44780_POWER_ON_STABILIZE_TIME_IN_MICROSECONDS);

    // Initial function set: HD44780_FUNCTION_SET_8BIT sent three times to enter 4-bit mode
    if (writePortValue(HD44780_FUNCTION_SET_8BIT |
                       (backlightEnabled ? PCF8574_BACKLIGHT_BIT : HD44780_NO_BACKLIGHT)).isInvalidOrIncomplete()) {
        initializing = false;
        return false;
    }
    delayMicroseconds(HD44780_FIRST_FUNCTION_SET_DELAY_IN_MICROSECONDS);

    if (writePortValue(HD44780_FUNCTION_SET_8BIT |
                       (backlightEnabled ? PCF8574_BACKLIGHT_BIT : HD44780_NO_BACKLIGHT)).isInvalidOrIncomplete()) {
        initializing = false;
        return false;
    }
    delayMicroseconds(HD44780_SECOND_FUNCTION_SET_DELAY_IN_MICROSECONDS);

    if (writePortValue(HD44780_FUNCTION_SET_8BIT |
                       (backlightEnabled ? PCF8574_BACKLIGHT_BIT : HD44780_NO_BACKLIGHT)).isInvalidOrIncomplete()) {
        initializing = false;
        return false;
    }
    delayMicroseconds(HD44780_THIRD_FUNCTION_SET_DELAY_IN_MICROSECONDS);

    // Set 4-bit mode: HD44780_FUNCTION_SET_4BIT
    if (writePortValue(HD44780_FUNCTION_SET_4BIT |
                       (backlightEnabled ? PCF8574_BACKLIGHT_BIT : HD44780_NO_BACKLIGHT)).isInvalidOrIncomplete()) {
        initializing = false;
        return false;
    }
    delayMicroseconds(HD44780_SET_4BIT_MODE_DELAY_IN_MICROSECONDS);

    // Now in 4-bit mode, use normal command sequence.
    sendCommand(HD44780_FUNCTION_SET_4BIT_2LINE_5X8);
    if (lastI2cResult.isInvalidOrIncomplete()) {
        initializing = false;
        return false;
    }
    sendCommand(HD44780_DISPLAY_ON_CURSOR_OFF_BLINK_OFF);
    if (lastI2cResult.isInvalidOrIncomplete()) {
        initializing = false;
        return false;
    }
    sendCommand(HD44780_CLEAR_DISPLAY);
    if (lastI2cResult.isInvalidOrIncomplete()) {
        initializing = false;
        return false;
    }
    delayMicroseconds(HD44780_CLEAR_DISPLAY_DELAY_IN_MICROSECONDS);
    sendCommand(HD44780_ENTRY_MODE_INCREMENT_NO_SHIFT);
    initializing = false;
    initialized = !lastI2cResult.isInvalidOrIncomplete();
    return initialized;
}

bool Hd44780Driver::start() {
    return initializeSequence();
}

void Hd44780Driver::initialize() const {
    (void)initializeSequence();
}

I2cResult Hd44780Driver::getLastI2cResult() const {
    return lastI2cResult;
}

bool Hd44780Driver::isInitialized() const {
    return initialized;
}

I2cStatus Hd44780Driver::mapWireStatus(const uint8_t status) {
    switch (status) {
        case 0: return I2cStatus::Ok;
        case 2: return I2cStatus::AddressNack;
        case 3: return I2cStatus::DataNack;
        case 5: return I2cStatus::Timeout;
        case 1: return I2cStatus::InvalidArgument;
        default: return I2cStatus::BusError;
    }
}
