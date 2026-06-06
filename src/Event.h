#ifndef EVENT_H
#define EVENT_H

/**
 * @file Event.h
 * @brief Defines the lightweight event value object used by the framework.
 * @ingroup core_module
 */

/**
 * @name Core: Event & Command Primitives
 * @ingroup core_module
 * @{
 */

/**
 * @brief Immutable-like event descriptor used to route sensor notifications.
 * @ingroup core_module
 */
class Event {
public:
    /**
     * @brief Numeric identifier used for event type matching.
     */
    int identifier;

    /**
     * @brief Creates an Event with the provided identifier.
     * @param eventIdentifier Event type discriminator.
     */
    explicit Event(int eventIdentifier) : identifier(eventIdentifier) {}

    /**
     * @brief Compares two events by identifier.
     * @param other Event to compare against.
     * @return `true` when both identifiers match, otherwise `false`.
     */
    bool operator==(const Event& other) const {
        return this->identifier == other.identifier;
    }
};

/** @} */ // End of Core: Event & Command Primitives

#endif // EVENT_H