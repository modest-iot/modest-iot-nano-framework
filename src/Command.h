#ifndef COMMAND_H
#define COMMAND_H

#include <stdint.h>

/**
 * @file Command.h
 * @brief Defines the lightweight command value object used by actuators and devices.
 * @ingroup core_module
 * @copyright (c) 2025-2026 Angel Velasquez
 * @license Apache-2.0
 * @date 2026-06-06
 * @version 1.0
 */

/**
 * @brief Immutable-like command descriptor used to route actuator actions.
 * @ingroup core_module
 */
class Command {
public:
    /**
     * @brief Numeric identifier used for command type matching.
     */
    const int identifier;

    /**
     * @brief Optional payload carrying parameterized data (e.g., angle, brightness, frequency).
     * @note Defaults to 0 for backward compatibility with identifier-only commands.
     */
    const int32_t payload;

    /**
     * @brief Creates a Command with the provided identifier and no payload.
     * @param commandIdentifier Command type discriminator.
     */
    constexpr explicit Command(const int commandIdentifier) : identifier(commandIdentifier), payload(0) {}

    /**
     * @brief Creates a Command with the provided identifier and payload.
     * @param commandIdentifier Command type discriminator.
     * @param commandPayload Parameterized data (e.g., angle 0–180, brightness 0–255, frequency in Hz).
     */
    constexpr Command(const int commandIdentifier, const int32_t commandPayload)
            : identifier(commandIdentifier), payload(commandPayload) {}

    /**
     * @brief Compares two commands by identifier and payload.
     * @param other Command to compare against.
     * @return `true` when both identifiers and payloads match, otherwise `false`.
     */
    bool operator==(const Command& other) const {
        return this->identifier == other.identifier && this->payload == other.payload;
    }
};

#endif // COMMAND_H