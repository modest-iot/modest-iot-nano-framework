#ifndef COMMAND_H
#define COMMAND_H

/**
 * @file Command.h
 * @brief Defines the lightweight command value object used by actuators and devices.
 * @ingroup core_module
 * @date 2026-06-06
 * @version 1.0
 */

/**
 * @name Core: Event & Command Primitives
 * @ingroup core_module
 * @{
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
     * @brief Creates a Command with the provided identifier.
     * @param commandIdentifier Command type discriminator.
     */
    constexpr explicit Command(const int commandIdentifier) : identifier(commandIdentifier) {}

    /**
     * @brief Compares two commands by identifier.
     * @param other Command to compare against.
     * @return `true` when both identifiers match, otherwise `false`.
     */
    bool operator==(const Command& other) const {
        return this->identifier == other.identifier;
    }
};

/** @} */ // End of Core: Event & Command Primitives

#endif // COMMAND_H