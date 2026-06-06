#ifndef COMMAND_H
#define COMMAND_H

/**
 * @file Command.h
 * @brief Defines the lightweight command value object used by actuators and devices.
 */

/**
 * @brief Immutable-like command descriptor used to route actuator actions.
 */
class Command {
public:
    /**
     * @brief Numeric identifier used for command type matching.
     */
    int identifier;

    /**
     * @brief Creates a Command with the provided identifier.
     * @param commandIdentifier Command type discriminator.
     */
    explicit Command(int commandIdentifier) : identifier(commandIdentifier) {}

    /**
     * @brief Compares two commands by identifier.
     * @param other Command to compare against.
     * @return `true` when both identifiers match, otherwise `false`.
     */
    bool operator==(const Command& other) const {
        return this->identifier == other.identifier;
    }
};

#endif // COMMAND_H