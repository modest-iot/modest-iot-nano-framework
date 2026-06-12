#ifndef BUZZER_H
#define BUZZER_H

/**
 * @file Buzzer.h
 * @brief Declares a Buzzer actuator for sound generation (on/off, tones).
 * @ingroup actuators_module
 * @date 2026-06-06
 * @version 1.0
 */

#include "Actuator.h"
#include <Arduino.h> // For tone() and noTone()

/**
 * @name Actuators: Sound Output
 * @ingroup actuators_module
 * @{
 */

/**
 * @brief Digital output actuator that controls a buzzer for simple sounds or tones.
 *
 * This class supports turning a buzzer on/off and playing specific tones for a duration.
 * Tone parameters (frequency and duration) are buffered via `setTone()` before
 * the `PLAY_TONE_COMMAND` is issued.
 */
class Buzzer : public Actuator {
private:
    /**
     * @brief Tracks if the buzzer is currently active (making sound).
     * This flag is set when a tone is started and cleared when `turnBuzzerOff()` is called.
     */
    bool active;
    /**
     * @brief The frequency in Hertz (Hz) buffered for the next `PLAY_TONE_COMMAND`.
     */
    int bufferedFrequency;
    /**
     * @brief The duration in milliseconds (ms) buffered for the next `PLAY_TONE_COMMAND`.
     */
    unsigned long bufferedDuration;

    /**
     * @brief Plays a default 1kHz tone continuously without a specified duration.
     * Sets the `active` flag to true.
     */
    void playDefaultToneWithoutDuration();

    /**
     * @brief Turns the buzzer off, stopping any current sound and ensuring the pin is LOW.
     * Clears the `active` flag.
     */
    void turnBuzzerOff();

    /**
     * @brief Plays the currently buffered tone (frequency and duration).
     * If buffered parameters are invalid (frequency or duration is zero), the buzzer is turned off.
     * Sets the `active` flag to true if a valid tone is played.
     */
    void playBufferedTone();

protected:
    /**
     * @brief Executes buzzer state commands (on/off/play tone) and optionally propagates them upstream.
     * This method dispatches to private helper methods based on the incoming command.
     * @param command Command to execute.
     */
    void executeCommand(Command command) override;

public:
    /** @brief Command identifier for turning the buzzer on (continuous sound). */
    static const int TURN_ON_COMMAND_IDENTIFIER = 250;
    /** @brief Command identifier for turning the buzzer off. */
    static const int TURN_OFF_COMMAND_IDENTIFIER = 251;
    /** @brief Command identifier for playing a buffered tone. */
    static const int PLAY_TONE_COMMAND_IDENTIFIER = 252;

    /** @brief Prebuilt command instance for turning the buzzer on. */
    static const Command TURN_ON_COMMAND;
    /** @brief Prebuilt command instance for turning the buzzer off. */
    static const Command TURN_OFF_COMMAND;
    /** @brief Prebuilt command instance for playing a buffered tone. */
    static const Command PLAY_TONE_COMMAND;

    /**
     * @brief Constructs a Buzzer actuator.
     * Initializes the buzzer pin as an OUTPUT and ensures it's off.
     * @param signalGpioPin Output GPIO pin connected to the buzzer.
     * @param parentHandler Optional upstream handler for propagated commands.
     */
    Buzzer(const int signalGpioPin, CommandHandler* const parentHandler = nullptr);

    /**
     * @brief Sets the frequency and duration for the next tone command.
     * These parameters are used when `PLAY_TONE_COMMAND` is received.
     * @param frequency The frequency of the tone in Hertz (Hz). Must be positive.
     * @param duration The duration of the tone in milliseconds (ms). Must be positive.
     */
    void setTone(const int frequency, const unsigned long duration);

    /**
     * @brief Reports if the buzzer is currently active (making sound).
     * Note: For tones played with a duration, this flag reflects if the tone was *started*,
     * not necessarily if it has finished playing.
     * @return `true` if the buzzer is on or playing a tone, otherwise `false`.
     */
    [[nodiscard]] bool isActive() const;

    /**
     * @brief Returns the currently buffered tone frequency.
     * @return The buffered frequency in Hertz.
     */
    [[nodiscard]] int getBufferedFrequency() const;

    /**
     * @brief Returns the currently buffered tone duration.
     * @return The buffered duration in milliseconds.
     */
    [[nodiscard]] unsigned long getBufferedDuration() const;
};

/** @} */ // End of Actuators: Sound Output

#endif // BUZZER_H
