#ifndef HD44780_DRIVER_H
#define HD44780_DRIVER_H

/**
 * @file Hd44780Driver.h
 * @brief Declares a native HD44780 4-bit mode I2C driver using PCF8574 backpack.
 * @ingroup actuators_module
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-11-09
 * @version 1.0
 */


#include <Wire.h>

/**
 * @brief Low-level HD44780 LCD controller driver over I2C with PCF8574 I/O expander.
 * Implements the HD44780 4-bit mode protocol directly, eliminating the LiquidCrystal_I2C
 * external dependency. The PCF8574 backpack maps P0-P3 to HD44780 data lines D4-D7,
 * P4 to backlight, P5 to enable, and P0 to register select (RS).
 * @ingroup actuators_module
 */
class Hd44780Driver {
private:
    uint8_t i2cAddress; ///< I2C address of the PCF8574 backpack.
    bool backlightEnabled; ///< Current backlight state.

    // PCF8574 pin mapping constants for HD44780 4-bit mode
    static constexpr uint8_t PCF8574_BACKLIGHT_BIT = 0x08; ///< Bit 3: Backlight enable.
    static constexpr uint8_t PCF8574_ENABLE_BIT = 0x04; ///< Bit 2: Enable pulse.
    static constexpr uint8_t PCF8574_REGISTER_SELECT_BIT = 0x01; ///< Bit 0: Register select (RS).

    // HD44780 instruction set constants
    static constexpr uint8_t HD44780_FUNCTION_SET_8BIT = 0x30; ///< Initial function set for 8-bit mode entry.
    static constexpr uint8_t HD44780_FUNCTION_SET_4BIT = 0x20; ///< Transition to 4-bit mode.
    static constexpr uint8_t HD44780_FUNCTION_SET_4BIT_2LINE_5X8 = 0x28; ///< 4-bit, 2-line, 5x8 dot font.
    static constexpr uint8_t HD44780_DISPLAY_ON_CURSOR_OFF_BLINK_OFF = 0x0C; ///< Display on, cursor off, blink off.
    static constexpr uint8_t HD44780_CLEAR_DISPLAY = 0x01; ///< Clear display and return home.
    static constexpr uint8_t HD44780_ENTRY_MODE_INCREMENT_NO_SHIFT = 0x06; ///< Increment cursor, no shift.

    // Bit mask constants for nibble extraction
    static constexpr uint8_t HD44780_HIGH_NIBBLE_MASK = 0xF0; ///< Mask for high 4 bits.
    static constexpr uint8_t HD44780_LOW_NIBBLE_MASK = 0x0F; ///< Mask for low 4 bits.
    static constexpr uint8_t HD44780_NO_BACKLIGHT = 0x00; ///< No backlight flag value.

    // Timing constants (microseconds) per HD44780 datasheet
    static constexpr int HD44780_POWER_ON_STABILIZE_TIME_IN_MICROSECONDS = 15000; ///< V0 stabilization after power-on.
    static constexpr int HD44780_FIRST_FUNCTION_SET_DELAY_IN_MICROSECONDS = 4100; ///< Delay after first 0x30.
    static constexpr int HD44780_SECOND_FUNCTION_SET_DELAY_IN_MICROSECONDS = 100; ///< Delay after second 0x30.
    static constexpr int HD44780_THIRD_FUNCTION_SET_DELAY_IN_MICROSECONDS = 100; ///< Delay after third 0x30.
    static constexpr int HD44780_SET_4BIT_MODE_DELAY_IN_MICROSECONDS = 100; ///< Delay after setting 4-bit mode.
    static constexpr int HD44780_CLEAR_DISPLAY_DELAY_IN_MICROSECONDS = 1600; ///< Clear command execution time.
    static constexpr int HD44780_ENABLE_SETUP_TIME_IN_MICROSECONDS = 1; ///< Minimum enable setup time.
    static constexpr int HD44780_ENABLE_PULSE_WIDTH_IN_MICROSECONDS = 1; ///< Minimum enable pulse high time.
    static constexpr int HD44780_ENABLE_RECOVERY_TIME_IN_MICROSECONDS = 37; ///< Minimum enable pulse recovery time.

    /**
     * @brief Sends a 4-bit nibble to the LCD with enable pulse.
     * @param value The 4-bit value to send.
     */
    void write4Bit(uint8_t value) const; ///< Sends a 4-bit nibble to the LCD with enable pulse.

public:
    explicit Hd44780Driver(uint8_t i2cAddress, bool backlightInitialState = true);

    /**
     * @brief Initializes HD44780 hardware registers via I2C.
     * Transitions from 8-bit to 4-bit mode, configures 2-line display, and clears the screen.
     */
    void initialize() const;

    /**
     * @brief Sends an instruction command to the HD44780.
     * @param command Instruction byte (e.g., 0x80 for DDRAM address).
     */
    void sendCommand(uint8_t command) const;

    /**
     * @brief Sends a data byte to the HD44780 DDRAM.
     * @param data Character byte to display.
     */
    void sendData(uint8_t data) const;

    /**
     * @brief Sets the backlight state.
     * @param backlightState True to turn on, false to turn off.
     */
    void setBacklight(bool backlightState);

    /**
     * @brief Reports whether the backlight is currently enabled.
     * @return `true` when backlight is on, otherwise `false`.
     */
    [[nodiscard]] bool isBacklightOn() const;
};

#endif // HD44780_DRIVER_H
