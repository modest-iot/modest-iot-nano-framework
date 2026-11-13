#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

/**
 * @file OledDisplay.h
 * @brief Declares an I2C OLED display actuator with frame buffer and drawing primitives.
 * @ingroup actuators_module
 * @date 2026-11-10
 * @version 1.0
 */

#include <cstdint>
#include "Actuator.h"
#include "Ssd1306Driver.h"

/**
 * @brief Actuator adapter for SSD1306-compatible OLED displays over I2C.
 * Composes Ssd1306Driver for native I2C communication via Wire, with a 1KB frame buffer
 * for 128x64 monochrome displays. Supports text rendering, drawing primitives, and display control.
 * @ingroup actuators_module
 */
class OledDisplay : public Actuator {
private:
    Ssd1306Driver driver; ///< SSD1306 protocol driver for I2C communication.

    // Display dimensions
    static constexpr uint8_t DISPLAY_WIDTH_IN_PIXELS = 128; ///< Display width in pixels.
    static constexpr uint8_t DISPLAY_HEIGHT_IN_PIXELS = 64; ///< Display height in pixels.
    static constexpr size_t FRAME_BUFFER_SIZE_IN_BYTES = 128; ///< Frame buffer size per page (128 bytes).
    static constexpr uint8_t NUMBER_OF_PAGES = 8; ///< Number of pages (64 pixels / 8 bits per byte).

    uint8_t frameBuffer[FRAME_BUFFER_SIZE_IN_BYTES * NUMBER_OF_PAGES]; ///< Frame buffer for the entire display.

    // 5x7 font bitmap (95 ASCII characters: 0x20-0x7E)
    static constexpr uint8_t FONT_WIDTH_IN_PIXELS = 5; ///< Font width in pixels.
    static constexpr uint8_t FONT_HEIGHT_IN_PIXELS = 7; ///< Font height in pixels.
    static constexpr uint8_t FONT_FIRST_CHARACTER = 0x20; ///< First ASCII character in the font (space).
    static constexpr uint8_t FONT_LAST_CHARACTER = 0x7E; ///< Last ASCII character in the font (~).
    static constexpr uint8_t FONT_CHARACTER_COUNT = 95; ///< Total number of characters in the font.

    // Private drawing methods
    void drawCharacter(char character, uint8_t x, uint8_t y); ///< Draws a single character at the given position.
    void setPixel(uint8_t x, uint8_t y, bool state); ///< Sets a single pixel in the frame buffer.
    void sendFrameBufferToDisplay() const; ///< Sends the entire frame buffer to the SSD1306.

protected:
    /**
     * @brief Intercepts display commands to update the screen, clear it, or modify backlight states.
     */
    void executeCommand(Command command) override;

public:
    /** @name Display Commands */
    //@{
    static constexpr int UPDATE_DISPLAY_COMMAND_IDENTIFIER = 350; ///< Command identifier to update the display.
    static constexpr int CLEAR_DISPLAY_COMMAND_IDENTIFIER = 351; ///< Command identifier to clear the display.
    static constexpr int SET_BACKLIGHT_COMMAND_IDENTIFIER = 352; ///< Command identifier to toggle the backlight.

    static const Command UPDATE_DISPLAY_COMMAND; ///< Prebuilt command to update the display.
    static const Command CLEAR_DISPLAY_COMMAND;  ///< Prebuilt command to clear the display.
    static const Command SET_BACKLIGHT_COMMAND;  ///< Prebuilt command to toggle the backlight.
    //@} // Display Commands

    /**
     * @param i2cAddress I2C address of the SSD1306 controller (0x3C or 0x3D).
     * @param parentHandler Base framework coordinator pointer connection.
     */
    explicit OledDisplay(uint8_t i2cAddress = 0x3C, CommandHandler* parentHandler = nullptr);

    /**
     * @brief Renders a string at the specified position.
     * @param text Null-terminated string to render.
     * @param x X coordinate in pixels (0-127).
     * @param y Y coordinate in pixels (0-63), aligned to 8-pixel rows.
     */
    void setText(const char* text, uint8_t x, uint8_t y);

#if defined(ARDUINO_ARCH_ESP32)
    /**
     * @brief Renders a flash-memory string at the specified position.
     * @param text Flash string (e.g., from the `F()` macro) to render.
     * @param x X coordinate in pixels (0-127).
     * @param y Y coordinate in pixels (0-63), aligned to 8-pixel rows.
     */
    void setText(const __FlashStringHelper* text, uint8_t x, uint8_t y);
#endif

    /**
     * @brief Clears the frame buffer, filling it with zeros.
     */
    void clearFrameBuffer() const;

    /**
     * @brief Draws a horizontal line.
     * @param x Start X coordinate.
     * @param y Y coordinate.
     * @param length Line length in pixels.
     */
    void drawHorizontalLine(uint8_t x, uint8_t y, uint8_t length);

    /**
     * @brief Draws a vertical line.
     * @param x X coordinate.
     * @param y Start Y coordinate.
     * @param length Line length in pixels.
     */
    void drawVerticalLine(uint8_t x, uint8_t y, uint8_t length);

    /**
     * @brief Draws a rectangle outline.
     * @param x Top-left X coordinate.
     * @param y Top-left Y coordinate.
     * @param width Rectangle width in pixels.
     * @param height Rectangle height in pixels.
     */
    void drawRectangle(uint8_t x, uint8_t y, uint8_t width, uint8_t height);

    /**
     * @brief Fills a rectangle.
     * @param x Top-left X coordinate.
     * @param y Top-left Y coordinate.
     * @param width Rectangle width in pixels.
     * @param height Rectangle height in pixels.
     */
    void fillRectangle(uint8_t x, uint8_t y, uint8_t width, uint8_t height);

    /**
     * @brief Reports whether the display backlight is currently enabled.
     * @return `true` when display is on, otherwise `false`.
     */
    [[nodiscard]] bool isBacklightOn() const;
};

#endif // OLED_DISPLAY_H
