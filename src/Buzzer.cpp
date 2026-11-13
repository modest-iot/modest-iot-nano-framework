/**
 * @file Buzzer.cpp
 * @brief Implements command-driven buzzer control for simple sounds and tones.
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-06-06
 * @version 1.0
 */

#include "Buzzer.h"

// Define static const Command instances
const Command Buzzer::TURN_ON_COMMAND = Command(Buzzer::TURN_ON_COMMAND_IDENTIFIER);
const Command Buzzer::TURN_OFF_COMMAND = Command(Buzzer::TURN_OFF_COMMAND_IDENTIFIER);
const Command Buzzer::PLAY_TONE_COMMAND = Command(Buzzer::PLAY_TONE_COMMAND_IDENTIFIER);

/**
 * @brief Constructs a Buzzer actuator.
 */
Buzzer::Buzzer(const int signalGpioPin, CommandHandler* const parentHandler)
    : Actuator(signalGpioPin, parentHandler),
      active(false),
      bufferedFrequency(0),
      bufferedDuration(0)
{
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW); // Ensure buzzer is off initially
}

/**
 * @brief Plays the default tone without a duration.
 */
void Buzzer::playDefaultToneWithoutDuration() {
    tone(pin, DEFAULT_TONE_FREQUENCY_IN_HERTZ);
    active = true;
}

/**
 * @brief Turns the buzzer off.
 */
void Buzzer::turnBuzzerOff() {
    noTone(pin);
    digitalWrite(pin, LOW); // Ensure pin is low
    active = false;
}

/**
 * @brief Plays the buffered tone.
 */
void Buzzer::playBufferedTone() {
    if (bufferedFrequency > 0 && bufferedDuration > 0) {
        tone(pin, bufferedFrequency, bufferedDuration);
        active = true;
        // The tone() function is non-blocking, but the sound plays for bufferedDuration.
        // A more complex implementation might use a timer to track duration and set 'active' to false.
    } else {
        // If invalid tone parameters, ensure buzzer is off
        turnBuzzerOff();
    }
}

/**
 * @brief Executes a buzzer command.
 *
 * @details `TURN_ON_COMMAND` plays a default 1kHz tone continuously.
 *          `TURN_OFF_COMMAND` stops any sound.
 *          `PLAY_TONE_COMMAND` plays the buffered frequency and duration.
 *          `PLAY_FREQUENCY_COMMAND` assigns the payload as the buffered frequency,
 *          then plays using the buffered duration via `playBufferedTone()`.
 */
void Buzzer::executeCommand(Command command) {
    if (command == TURN_ON_COMMAND) {
        playDefaultToneWithoutDuration();
    } else if (command == TURN_OFF_COMMAND) {
        turnBuzzerOff();
    } else if (command == PLAY_TONE_COMMAND) {
        playBufferedTone();
    } else if (command.identifier == PLAY_FREQUENCY_COMMAND_IDENTIFIER) {
        bufferedFrequency = command.payload;
        playBufferedTone();
    }
}

/**
 * @brief Sets the tone parameters.
 */
void Buzzer::setTone(const int frequency, const unsigned long duration) {
    bufferedFrequency = frequency;
    bufferedDuration = duration;
}

/**
 * @brief Checks if the buzzer is active.
 * @return True if active, false otherwise.
 */
bool Buzzer::isActive() const {
    // For tone() with duration, the active state is tricky.
    // This currently reflects if a tone() or continuous tone was *started*.
    // A more accurate isActive would require tracking tone completion.
    return active;
}

/**
 * @brief Gets the buffered frequency.
 * @return The frequency in Hz.
 */
int Buzzer::getBufferedFrequency() const {
    return bufferedFrequency;
}

/**
 * @brief Gets the buffered duration.
 * @return The duration in ms.
 */
unsigned long Buzzer::getBufferedDuration() const {
    return bufferedDuration;
}
