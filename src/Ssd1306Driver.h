#ifndef SSD1306_DRIVER_H
#define SSD1306_DRIVER_H

/**
 * @file Ssd1306Driver.h
 * @brief Declares a native SSD1306 OLED I2C driver for 128x64 monochrome displays.
 * @ingroup actuators_module
 * @date 2026-11-10
 * @version 1.0
 */
#include <cstdint>
#include <Wire.h>

/**
 * @brief Low-level SSD1306 OLED controller driver over I2C.
 * Implements the SSD1306 command protocol directly, eliminating external display libraries.
 * Supports 128x64 and 128x32 monochrome OLED displays with page addressing mode.
 * @ingroup actuators_module
 */
class Ssd1306Driver {
private:
    uint8_t i2cAddress; ///< I2C address of the SSD1306 controller (0x3C or 0x3D).
    uint8_t displayWidth; ///< Display width in pixels (typically 128).
    uint8_t displayHeight; ///< Display height in pixels (32 or 64).
    bool displayOn; ///< Current display on/off state.

    // SSD1306 command constants
    static constexpr uint8_t SSD1306_COMMAND_PREFIX = 0x00; ///< I2C command mode prefix byte.
    static constexpr uint8_t SSD1306_DATA_PREFIX = 0x40; ///< I2C data mode prefix byte.

    // SSD1306 initialization commands
    static constexpr uint8_t SSD1306_SET_DISPLAY_OFF = 0xAE; ///< Display off (sleep mode).
    static constexpr uint8_t SSD1306_SET_DISPLAY_ON = 0xAF; ///< Display on.
    static constexpr uint8_t SSD1306_SET_CONTRAST_CONTROL = 0x81; ///< Set contrast control register.
    static constexpr uint8_t SSD1306_SET_CHARGE_PUMP = 0x8D; ///< Set charge pump regulator.
    static constexpr uint8_t SSD1306_SET_MEMORY_ADDRESSING_MODE = 0x20; ///< Set memory addressing mode.
    static constexpr uint8_t SSD1306_SET_SEGMENT_REMAP = 0xA1; ///< Segment remap (column 0 at COM segment D).
    static constexpr uint8_t SSD1306_SET_DISPLAY_START_LINE = 0x40; ///< Set display start line.
    static constexpr uint8_t SSD1306_SET_DISPLAY_CLOCK_DIV = 0xD5; ///< Set display clock divide ratio.
    static constexpr uint8_t SSD1306_SET_MULTIPLEX_RATIO = 0xA8; ///< Set multiplex ratio.
    static constexpr uint8_t SSD1306_SET_DISPLAY_OFFSET = 0xD3; ///< Set display offset.
    static constexpr uint8_t SSD1306_SET_NORMAL_DISPLAY = 0xA6; ///< Normal display (not inverted).
    static constexpr uint8_t SSD1306_SET_INVERT_DISPLAY = 0xA7; ///< Inverted display.
    static constexpr uint8_t SSD1306_SET_DEACTIVATE_SCROLL = 0x2E; ///< Deactivate scroll.
    static constexpr uint8_t SSD1306_SET_PAGE_ADDRESS = 0xB0; ///< Set page address (0-7).
    static constexpr uint8_t SSD1306_SET_LOWER_COLUMN_ADDRESS = 0x00; ///< Set lower column address.
    static constexpr uint8_t SSD1306_SET_HIGHER_COLUMN_ADDRESS = 0x10; ///< Set higher column address.

    // SSD1306 display dimensions
    static constexpr uint8_t SSD1306_MAX_DISPLAY_WIDTH = 128; ///< Maximum display width in pixels.

    // SSD1306 initialization parameter defaults
    static constexpr uint8_t SSD1306_CLOCK_DIVIDE_RATIO_DEFAULT = 0x80; ///< Default clock divide ratio.
    static constexpr uint8_t SSD1306_DISPLAY_OFFSET_NONE = 0x00; ///< No display offset.
    static constexpr uint8_t SSD1306_CONTRAST_DEFAULT = 0x7F; ///< Default contrast value for normal display.
    static constexpr uint8_t SSD1306_CHARGE_PUMP_ENABLE = 0x14; ///< Enable charge pump regulator.
    static constexpr uint8_t SSD1306_ADDRESSING_MODE_HORIZONTAL = 0x00; ///< Horizontal memory addressing mode.
    static constexpr uint8_t SSD1306_CLEAR_BUFFER_FILL_VALUE = 0x00; ///< Value to fill clear buffer with.

    // Timing constants (microseconds) per SSD1306 datasheet
    static constexpr int SSD1306_I2C_COMMAND_DELAY_IN_MICROSECONDS = 10; ///< Delay after I2C command.

    /**
     * @brief Sends a single command byte to the SSD1306.
     * @param command Command byte to send.
     */
    void sendCommandByte(uint8_t command) const;

public:
    explicit Ssd1306Driver(uint8_t i2cAddress = 0x3C, uint8_t width = 128, uint8_t height = 64);

    /**
     * @brief Initializes SSD1306 hardware registers via I2C.
     * Configures display clock, multiplex ratio, contrast, charge pump, and memory addressing mode.
     */
    void initialize();

    /**
     * @brief Sends a block of data (frame buffer) to the SSD1306.
     * @param data Pointer to the data buffer.
     * @param length Number of bytes to send.
     */
    void sendData(const uint8_t* data, size_t length) const;

    /**
     * @brief Sends the entire frame buffer to the display using page addressing mode.
     * @param frameBuffer Pointer to the frame buffer (width * height / 8 bytes).
     * @param width Display width in pixels.
     */
    void sendFrameBuffer(const uint8_t* frameBuffer, uint8_t width) const;

    /**
     * @brief Sets the display contrast.
     * @param contrast Contrast value (0-255, default 0x7F).
     */
    void setContrast(uint8_t contrast) const;

    /**
     * @brief Toggles display inversion.
     * @param invert True for inverted display, false for normal.
     */
    void setInvert(bool invert) const;

    /**
     * @brief Turns the display on or off.
     * @param enable True to turn on, false to turn off.
     */
    void setDisplayOn(bool enable);

    /**
     * @brief Reports whether the display is currently on.
     * @return `true` when display is on, otherwise `false`.
     */
    [[nodiscard]] bool isDisplayOn() const;

    /** @brief Returns the display width in pixels. */
    [[nodiscard]] uint8_t getWidth() const;

    /** @brief Returns the display height in pixels. */
    [[nodiscard]] uint8_t getHeight() const;
};

#endif // SSD1306_DRIVER_H
