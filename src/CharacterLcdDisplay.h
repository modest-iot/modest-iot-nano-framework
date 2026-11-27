#ifndef CHARACTER_LCD_DISPLAY_H
#define CHARACTER_LCD_DISPLAY_H

/**
 * @file CharacterLcdDisplay.h
 * @brief Declares an I2C character LCD actuator with buffered text updates.
 * @ingroup actuators_module
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-06-06
 * @version 1.0
 */

#include "Actuator.h"
#include "Hd44780Driver.h"

/**
 * @brief Actuator adapter for HD44780-compatible character Liquid Crystal displays over I2C.
 * Composes Hd44780Driver for native I2C communication with a PCF8574 I/O expander backpack,
 * implementing the HD44780 4-bit mode protocol (no external library). The legacy constructor
 * uses global @c Wire; the injected constructor uses the supplied shared @ref I2cBus.
 * @ingroup actuators_module
 */
class CharacterLcdDisplay : public Actuator {
private:
    Hd44780Driver driver; ///< HD44780 protocol driver for I2C communication.

    // Instance-specific structural dimensions assigned at construction
    const uint8_t maximumRowCount; ///< Maximum number of rows supported.
    const uint8_t maximumColumnCount; ///< Maximum number of columns supported.

    // Contiguous memory allocation buffer tracking absolute dimensions safely
    char* characterScreenMatrixBuffer; ///< Buffer for storing characters to be displayed.
    bool initialized;

    // HD44780 DDRAM address constants for cursor positioning
    static constexpr uint8_t HD44780_DDRAM_ADDRESS_ROW_0 = 0x80; ///< DDRAM address for row 0, column 0.
    static constexpr uint8_t HD44780_DDRAM_ADDRESS_ROW_1 = 0xC0; ///< DDRAM address for row 1, column 0.
    static constexpr uint8_t HD44780_DDRAM_ADDRESS_ROW_2 = 0x94; ///< DDRAM address for row 2, column 0.
    static constexpr uint8_t HD44780_DDRAM_ADDRESS_ROW_3 = 0xD4; ///< DDRAM address for row 3, column 0.

    // Private encapsulated layout routines to elevate code clarity
    void commitBuffersToPhysicalScreen() const; ///< Writes buffered data to the physical screen.
    void initializeScreenBuffer();

protected:
    /**
     * @brief Intercepts target display commands to commit textual changes or modify backlight states.
     */
    void executeCommand(Command command) override;

public:
    static constexpr int UPDATE_TEXT_COMMAND_IDENTIFIER = 200;
    static constexpr int TURN_BACKLIGHT_ON_COMMAND_IDENTIFIER = 201;
    static constexpr int TURN_BACKLIGHT_OFF_COMMAND_IDENTIFIER = 202;

    static const Command UPDATE_TEXT_COMMAND;
    static const Command TURN_BACKLIGHT_ON_COMMAND;
    static const Command TURN_BACKLIGHT_OFF_COMMAND;

    /**
     * @brief Constructs the legacy global-@c Wire character display.
     * @param i2cAddress I2C address of the PCF8574 backpack (e.g., 0x27, 0x3E).
     * @param columnCount Total number of columns (e.g., 16, 20, 40).
     * @param rowCount Total number of rows (e.g., 2, 4).
     * @param backlightInitialState Backlight state configuration upon physical initialization.
     * @param parentHandler Base framework coordinator pointer connection.
     * @note This compatibility constructor initializes the legacy display path.
     */
    CharacterLcdDisplay(uint8_t i2cAddress, uint8_t columnCount, uint8_t rowCount,
                        bool backlightInitialState = true, CommandHandler* parentHandler = nullptr);

    /**
     * @brief Constructs a configuration-only character display using a shared bus.
     * @param bus Non-owning I2C bus reference; it must outlive this display.
     * @param i2cAddress I2C address of the PCF8574 backpack (e.g., 0x27, 0x3E).
     * @param columnCount Total number of columns (e.g., 16, 20, 40).
     * @param rowCount Total number of rows (e.g., 2, 4).
     * @param backlightInitialState Backlight state configuration for initialization.
     * @param parentHandler Base framework coordinator pointer connection.
     * @note Performs no hardware I/O. Start @p bus before calling @ref start.
     */
    CharacterLcdDisplay(I2cBus& bus, uint8_t i2cAddress, uint8_t columnCount, uint8_t rowCount,
                        bool backlightInitialState = true, CommandHandler* parentHandler = nullptr);

    /**
     * @brief Initializes the character display through its configured transport.
     * @return @c true if the complete controller initialization succeeded.
     * @note The injected bus must already be started. Use @ref getLastI2cResult
     *       to inspect the failed transfer if initialization returns @c false.
     */
    [[nodiscard]] bool start();

    /**
     * @brief Returns the result of the most recent display I2C operation.
     * @return Status and transferred-versus-expected byte counts for the last operation.
     */
    [[nodiscard]] I2cResult getLastI2cResult() const;

    /**
     * @brief Reports whether display initialization completed successfully.
     * @return @c true if the most recent initialization sequence succeeded.
     */
    [[nodiscard]] bool isInitialized() const;

    ~CharacterLcdDisplay() override;

    /**
     * @brief Stages a clean string into a target row index, automatically zero-padded.
     * @param rowIndex Target row to update.
     * @param nullTerminatedString Source string to copy into the row buffer.
     */
    void setLineBuffer(uint8_t rowIndex, const char* nullTerminatedString) const;

    /**
     * @brief Composite helper allowing multi-row string pointer arrays to be loaded concurrently.
     * @param multiLineArrayPointer Null-terminated array of C-string row values.
     */
    void setLineBuffer(const char* multiLineArrayPointer[]) const;

    // ALIGNED PARADIGM: Active interrogative queries are reserved exclusively for methods
    /**
     * @brief Reports whether the backlight is currently enabled.
     * @return `true` when backlight is on, otherwise `false`.
     */
    [[nodiscard]] bool isBacklightOn() const;

    /** @brief Returns the number of addressable rows on this display. */
    [[nodiscard]] uint8_t getMaximumRowCount() const;
    /** @brief Returns the number of addressable columns on this display. */
    [[nodiscard]] uint8_t getMaximumColumnCount() const;
};

#endif // CHARACTER_LCD_DISPLAY_H