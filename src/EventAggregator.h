/**
 * @file EventAggregator.h
 * @brief Composite event pattern matching with time-windowed detection.
 * @ingroup core_module
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-11-19
 * @version 1.0
 *
 * The EventAggregator listens for events and matches composite patterns
 * (e.g., "motion AND door opened within 5 seconds"). When a pattern matches,
 * it emits a composite event that can trigger automation rules.
 *
 * All storage uses fixed-size arrays to avoid heap allocation.
 */

#ifndef MODEST_IOT_NANO_FRAMEWORK_EVENT_AGGREGATOR_H
#define MODEST_IOT_NANO_FRAMEWORK_EVENT_AGGREGATOR_H

#include <cstdint>

#include "Event.h"
#include "EventHandler.h"

/**
 * @brief Maximum number of captured events in the circular buffer.
 * @note 32 captured events is sufficient for typical use cases (4 sensors × 8 events).
 */
static constexpr uint8_t EVENT_AGGREGATOR_MAX_CAPTURED_EVENTS = 32;

/**
 * @brief Maximum number of composite event patterns.
 * @note 4 patterns covers typical automation rules (motion+door, leak+door, etc.).
 */
static constexpr uint8_t EVENT_AGGREGATOR_MAX_PATTERNS = 4;

/**
 * @brief Maximum event identifiers per pattern.
 * @note 4 identifiers covers most composite patterns (e.g., motion + door + leak + gas).
 */
static constexpr uint8_t EVENT_AGGREGATOR_MAX_PATTERN_SIZE = 4;

/**
 * @brief Single captured event stored in the circular buffer.
 * @ingroup core_module
 */
struct CapturedEvent {
    int identifier; ///< Event identifier.
    int sourceId; ///< Event source identifier.
    uint32_t timestamp; ///< Absolute timestamp in seconds from TimeSynchronizationDriver.
};

/**
 * @brief Triggering condition callback for composite event patterns.
 * @ingroup core_module
 *
 * Returns true if the composite event should be emitted, false to suppress it.
 * The developer defines this function in the Device descendant to check sensor
 * values at the moment the pattern matches (e.g., "is cargo bay empty?").
 *
 * @param compositeEventId The event identifier that would be emitted if the
 *        condition returns true. Allows a single function to handle multiple
 *        patterns by distinguishing which pattern matched.
 */
typedef bool (*PatternCondition)(int compositeEventId);

/**
 * @brief Boolean logic for composite event pattern matching.
 * @ingroup core_module
 */
enum class PatternLogic : uint8_t {
    And, ///< All identifiers must occur within the time window.
    Or ///< Any one identifier occurring within the time window triggers the pattern.
};

/**
 * @brief Composite event pattern definition.
 * @ingroup core_module
 *
 * A pattern matches when the required event identifiers occur within the
 * specified time window according to the pattern logic (AND or OR).
 * When matched, a composite event is emitted.
 */
struct EventPattern {
    int identifiers[EVENT_AGGREGATOR_MAX_PATTERN_SIZE]; ///< Event identifiers to match.
    int sourceIds[EVENT_AGGREGATOR_MAX_PATTERN_SIZE]; ///< Source IDs to match (EventAggregator::ANY_SOURCE = any source).
    uint8_t identifierCount; ///< Number of identifiers in this pattern.
    uint32_t timeWindowSeconds; ///< Maximum seconds between first and last event.
    int compositeEventId; ///< Event identifier emitted when the pattern matches.
    int compositeEventSourceId; ///< Source ID attached to the composite event (default: EventAggregator::DEVICE_SOURCE_ID).
    bool active; ///< True if this pattern slot is registered.
    PatternCondition condition; ///< Optional triggering condition (nullptr = always emit).
    PatternLogic logic; ///< AND (all identifiers required) or OR (any one identifier triggers).
};

/**
 * @brief Matches composite event patterns with time-windowed detection.
 * @ingroup core_module
 *
 * Implements EventHandler to receive events from sensors. Forwards each input
 * event exactly once before capturing and checking it against registered patterns
 * using AND or OR logic. A composite is emitted only when the current input matches
 * at least one source-filtered pattern term and the full pattern matches; unrelated
 * later events cannot replay an earlier match. An optional condition can further
 * filter matches based on sensor values. Composite events carry DEVICE_SOURCE_ID,
 * enabling nested aggregators to distinguish them from raw sensor events.
 *
 * @par Zero Heap Allocation
 * Uses fixed-size circular buffer (32 captured events) and pattern array (4 patterns).
 * No dynamic allocation.
 */
class EventAggregator : public EventHandler {
public:
    /**
     * @brief Wildcard source ID for pattern matching.
     * @note Use this value in a sourceIds array to match any sensor source
     *       for that pattern position. A specific pin number requires an exact
     *       source match (e.g., only DATA_READ from the LoadCellAmplifier on pin 4).
     */
    static constexpr int ANY_SOURCE = -1;

    /**
     * @brief Default source ID for composite events emitted by the EventAggregator.
     * @note Sensors use positive pin numbers as sourceId. This constant (0) distinguishes
     *       composite events from sensor events, enabling source-aware pattern matching
     *       in nested aggregators. When chaining aggregators, the second aggregator can
     *       filter patterns by DEVICE_SOURCE_ID to match only composite events from
     *       the first aggregator, not raw sensor events.
     */
    static constexpr int DEVICE_SOURCE_ID = 0;

    /**
     * @brief Constructs an EventAggregator with no patterns registered.
     */
    EventAggregator();

    /**
     * @brief Registers a composite event pattern with AND logic (all identifiers must occur).
     * @param eventIdentifiers Array of event identifiers that must all occur.
     * @param eventIdentifierCount Number of identifiers in the array.
     * @param timeWindowSeconds Maximum seconds between the first and last
     *        event in the pattern. All events must occur within this window.
     * @param compositeEventId Event identifier emitted when the pattern matches.
     * @return True if the pattern was registered, false if the pattern array is full.
     * @note All identifiers must occur within the time window for the pattern to match.
     */
    bool addAllMatchPattern(const int* eventIdentifiers, uint8_t eventIdentifierCount,
                            uint32_t timeWindowSeconds, int compositeEventId);

    /**
     * @brief Registers a composite event pattern with AND logic and source filtering.
     * @param eventIdentifiers Array of event identifiers that must all occur.
     * @param eventIdentifierCount Number of identifiers in the array.
     * @param sourceIds Array of source IDs to match for each identifier.
     *        Use EventAggregator::ANY_SOURCE to match any source for that position.
     * @param timeWindowSeconds Maximum seconds between the first and last
     *        event in the pattern. All events must occur within this window.
     * @param compositeEventId Event identifier emitted when the pattern matches.
     * @return True if the pattern was registered, false if the pattern array is full.
     * @note All identifiers must occur within the time window for the pattern to match.
     *       When sourceIds[i] != EventAggregator::ANY_SOURCE, the captured event must also match that source.
     */
    bool addAllMatchPattern(const int* eventIdentifiers, uint8_t eventIdentifierCount,
                            const int* sourceIds, uint32_t timeWindowSeconds, int compositeEventId);

    /**
     * @brief Registers a composite event pattern with AND logic and a triggering condition.
     * @param eventIdentifiers Array of event identifiers that must all occur.
     * @param eventIdentifierCount Number of identifiers in the array.
     * @param sourceIds Array of source IDs to match for each identifier.
     *        Use EventAggregator::ANY_SOURCE to match any source for that position.
     * @param timeWindowSeconds Maximum seconds between the first and last
     *        event in the pattern. All events must occur within this window.
     * @param compositeEventId Event identifier emitted when the pattern matches.
     * @param condition Optional function that receives the compositeEventId and
     *        returns true to emit the composite event or false to suppress it.
     *        Pass nullptr to always emit.
     * @return True if the pattern was registered, false if the pattern array is full.
     * @note All identifiers must occur within the time window for the pattern to match.
     *       The condition function is called after all identifiers match, receiving the
     *       composite event ID as context. This allows a single condition function to
     *       handle multiple patterns differently. It runs synchronously on the event-delivery
     *       context (normally the sampler task), so keep it short and non-blocking. Use this to
     *       check sensor values (e.g., "is cargo bay empty?")
 *       that determine whether the composite event should actually be emitted. The callback
 *       runs synchronously in the context that delivers the event (normally the sampler task),
 *       not in interrupt context.
     */
    bool addAllMatchPattern(const int* eventIdentifiers, uint8_t eventIdentifierCount,
                            const int* sourceIds, uint32_t timeWindowSeconds, int compositeEventId,
                            PatternCondition condition);

    /**
     * @brief Registers a composite event pattern with OR logic (any one identifier triggers).
     * @param eventIdentifiers Array of event identifiers; any one occurring triggers the pattern.
     * @param eventIdentifierCount Number of identifiers in the array.
     * @param timeWindowSeconds Maximum seconds within which any one event must occur.
     * @param compositeEventId Event identifier emitted when the pattern matches.
     * @return True if the pattern was registered, false if the pattern array is full.
     * @note Any single identifier occurring within the window triggers the composite event.
     *       This complements addAllMatchPattern() which requires all identifiers.
     *       For example, "motion OR door opened within 10 seconds" triggers an alert
     *       when either event occurs, not requiring both.
     */
    bool addAnyMatchPattern(const int* eventIdentifiers, uint8_t eventIdentifierCount,
                            uint32_t timeWindowSeconds, int compositeEventId);

    /**
     * @brief Registers a composite event pattern with OR logic and source filtering.
     * @param eventIdentifiers Array of event identifiers; any one occurring triggers the pattern.
     * @param eventIdentifierCount Number of identifiers in the array.
     * @param sourceIds Array of source IDs to match for each identifier.
     *        Use EventAggregator::ANY_SOURCE to match any source for that position.
     * @param timeWindowSeconds Maximum seconds within which any one event must occur.
     * @param compositeEventId Event identifier emitted when the pattern matches.
     * @return True if the pattern was registered, false if the pattern array is full.
     * @note Any single identifier occurring within the window triggers the composite event.
     *       Source filtering distinguishes sensors sharing the same event identifier
     *       (e.g., DATA_READ from LoadCellAmplifier vs. DhtSensor).
     */
    bool addAnyMatchPattern(const int* eventIdentifiers, uint8_t eventIdentifierCount,
                            const int* sourceIds, uint32_t timeWindowSeconds, int compositeEventId);

    /**
     * @brief Removes a registered composite event pattern.
     * @param compositeEventId The event identifier of the pattern to remove.
     * @return True if the pattern was found and removed, false if not found.
     * @note After removal, the freed slot can be reused by a subsequent addAllMatchPattern()
     *       or addAnyMatchPattern() call. The composite event will no longer be emitted.
     */
    bool removePattern(int compositeEventId);

    /**
     * @brief Removes all registered composite event patterns.
     * @note Convenience method for housekeeping: clears all active patterns in one call
     *       so developers do not need to track and remove each pattern individually.
     *       Typically called from the Device destructor before stopAsynchronousEngine().
     *       After this call, no composite events will be emitted until new patterns
     *       are registered via addAllMatchPattern() or addAnyMatchPattern().
     */
    void cleanupPatterns();

    /**
     * @brief Sets the output handler for composite events.
     * @param handler The EventHandler that receives composite events when patterns match.
     * @note Typically the Device instance, which dispatches to the developer's on(Event) override.
     */
    void setOutputHandler(EventHandler* handler);

    /**
     * @brief Handles incoming events from sensors.
     * @param event The event to capture and check against patterns.
     * @note Implements EventHandler::on(). Captures the event with its timestamp,
     *       checks all registered patterns, and emits composite events when matched.
     */
    void on(Event event) override;

private:
    /** @brief Captured events in a circular buffer. */
    CapturedEvent capturedEvents[EVENT_AGGREGATOR_MAX_CAPTURED_EVENTS];

    /** @brief Number of captured events currently in the buffer. */
    uint8_t capturedEventCount;

    /** @brief Index of the next write position in the circular buffer. */
    uint8_t writeIndex;

    /** @brief Registered composite event patterns. */
    EventPattern patterns[EVENT_AGGREGATOR_MAX_PATTERNS];

    /** @brief Number of active patterns. */
    uint8_t patternCount;

    /** @brief Output handler for composite events. */
    EventHandler* outputHandler;

    /**
     * @brief Checks all registered patterns for a match.
     * @param currentTimestamp The timestamp of the most recent event.
     * @note Iterates all active patterns. If a pattern matches, emits the
     *       composite event via the output handler. The newly received event must
     *       match at least one pattern term before a cached match can be emitted.
     */
    void checkForEventPatternMatch(const uint32_t currentTimestamp, const Event& newEvent) const;

    /**
     * @brief Removes captured events older than the maximum pattern time window.
     * @param currentTimestamp The current timestamp.
     * @note Prevents the circular buffer from filling with stale events.
     */
    void removeExpiredCapturedEvents(const uint32_t currentTimestamp);

    /**
     * @brief Captures an event in the circular buffer.
     * @param event The event to capture.
     * @param timestamp The current timestamp.
     * @note Handles both buffer-not-full and buffer-full (circular overwrite) cases.
     */
    void captureEvent(const Event event, const uint32_t timestamp);

    /**
     * @brief Returns the circular buffer index for a given offset from the oldest captured event.
     * @param offsetFromOldest The offset from the oldest captured event (0 = oldest).
     * @return The index into the capturedEvents array.
     * @note Encapsulates the circular buffer arithmetic to avoid formula duplication.
     */
    uint8_t getCapturedEventIndex(const uint8_t offsetFromOldest) const;

    /**
     * @brief Returns the maximum time window across all active patterns.
     * @return The largest timeWindowSeconds value, or 0 if no patterns are active.
     */
    uint32_t getMaxTimeWindow() const;
};

#endif // MODEST_IOT_NANO_FRAMEWORK_EVENT_AGGREGATOR_H
