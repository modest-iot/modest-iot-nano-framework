/**
 * @file EventAggregator.cpp
 * @brief Implementation of composite event pattern matching.
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-11-19
 * @version 1.0
 */

#include "EventAggregator.h"
#include "TimeSynchronizationDriver.h"

// --- Constructor ---

EventAggregator::EventAggregator()
    : capturedEvents{}, capturedEventCount(0),
      writeIndex(0),
      patternCount(0),
      outputHandler(nullptr) {
    for (auto &pattern: patterns) {
        pattern.active = false;
        pattern.identifierCount = 0;
        pattern.condition = nullptr;
        pattern.logic = PatternLogic::And;
        pattern.compositeEventSourceId = DEVICE_SOURCE_ID;
        for (int &sid : pattern.sourceIds) {
            sid = ANY_SOURCE;
        }
    }
}

// --- Public Methods ---

bool EventAggregator::addAllMatchPattern(const int* eventIdentifiers, const uint8_t eventIdentifierCount,
                                          const uint32_t timeWindowSeconds, const int compositeEventId) {
    // Delegate to the condition overload with nullptr condition (always emit)
    return addAllMatchPattern(eventIdentifiers, eventIdentifierCount, nullptr, timeWindowSeconds, compositeEventId, nullptr);
}

bool EventAggregator::addAllMatchPattern(const int* eventIdentifiers, const uint8_t eventIdentifierCount,
                                          const int* sourceIds, const uint32_t timeWindowSeconds, const int compositeEventId) {
    // Delegate to the condition overload with nullptr condition (always emit)
    return addAllMatchPattern(eventIdentifiers, eventIdentifierCount, sourceIds, timeWindowSeconds, compositeEventId, nullptr);
}

bool EventAggregator::addAllMatchPattern(const int* eventIdentifiers, const uint8_t eventIdentifierCount,
                                          const int* sourceIds, const uint32_t timeWindowSeconds, const int compositeEventId,
                                          const PatternCondition condition) {
    if (patternCount >= EVENT_AGGREGATOR_MAX_PATTERNS) {
        return false;
    }
    if (eventIdentifierCount == 0 || eventIdentifierCount > EVENT_AGGREGATOR_MAX_PATTERN_SIZE) {
        return false;
    }

    EventPattern& pattern = patterns[patternCount];
    for (uint8_t i = 0; i < eventIdentifierCount; i++) {
        pattern.identifiers[i] = eventIdentifiers[i];
        pattern.sourceIds[i] = (sourceIds != nullptr) ? sourceIds[i] : ANY_SOURCE;
    }
    pattern.identifierCount = eventIdentifierCount;
    pattern.timeWindowSeconds = timeWindowSeconds;
    pattern.compositeEventId = compositeEventId;
    pattern.compositeEventSourceId = DEVICE_SOURCE_ID;
    pattern.condition = condition;
    pattern.logic = PatternLogic::And;
    pattern.active = true;

    patternCount++;
    return true;
}

bool EventAggregator::addAnyMatchPattern(const int* eventIdentifiers, const uint8_t eventIdentifierCount,
                                          const uint32_t timeWindowSeconds, const int compositeEventId) {
    return addAnyMatchPattern(eventIdentifiers, eventIdentifierCount, nullptr, timeWindowSeconds, compositeEventId);
}

bool EventAggregator::addAnyMatchPattern(const int* eventIdentifiers, const uint8_t eventIdentifierCount,
                                          const int* sourceIds, const uint32_t timeWindowSeconds, const int compositeEventId) {
    if (patternCount >= EVENT_AGGREGATOR_MAX_PATTERNS) {
        return false;
    }
    if (eventIdentifierCount == 0 || eventIdentifierCount > EVENT_AGGREGATOR_MAX_PATTERN_SIZE) {
        return false;
    }

    EventPattern& pattern = patterns[patternCount];
    for (uint8_t i = 0; i < eventIdentifierCount; i++) {
        pattern.identifiers[i] = eventIdentifiers[i];
        pattern.sourceIds[i] = (sourceIds != nullptr) ? sourceIds[i] : ANY_SOURCE;
    }
    pattern.identifierCount = eventIdentifierCount;
    pattern.timeWindowSeconds = timeWindowSeconds;
    pattern.compositeEventId = compositeEventId;
    pattern.compositeEventSourceId = DEVICE_SOURCE_ID;
    pattern.condition = nullptr;
    pattern.logic = PatternLogic::Or;
    pattern.active = true;

    patternCount++;
    return true;
}

bool EventAggregator::removePattern(int compositeEventId) {
    // Find the pattern with the matching compositeEventId
    for (uint8_t p = 0; p < patternCount; p++) {
        if (patterns[p].active && patterns[p].compositeEventId == compositeEventId) {
            // Shift all patterns after this one down by one slot to keep the array compact
            for (uint8_t q = p; q < patternCount - 1; q++) {
                patterns[q] = patterns[q + 1];
            }
            // Clear the last slot and decrement count
            patterns[patternCount - 1].active = false;
            patterns[patternCount - 1].identifierCount = 0;
            patterns[patternCount - 1].condition = nullptr;
            patterns[patternCount - 1].logic = PatternLogic::And;
            patterns[patternCount - 1].compositeEventSourceId = DEVICE_SOURCE_ID;
            for (int &sid : patterns[patternCount - 1].sourceIds) {
                sid = ANY_SOURCE;
            }
            patternCount--;
            return true;
        }
    }
    return false;
}

void EventAggregator::cleanupPatterns() {
    for (uint8_t p = 0; p < patternCount; p++) {
        patterns[p].active = false;
        patterns[p].identifierCount = 0;
        patterns[p].condition = nullptr;
        patterns[p].logic = PatternLogic::And;
        patterns[p].compositeEventSourceId = DEVICE_SOURCE_ID;
        for (int &sid : patterns[p].sourceIds) {
            sid = ANY_SOURCE;
        }
    }
    patternCount = 0;
}

void EventAggregator::setOutputHandler(EventHandler* handler) {
    outputHandler = handler;
}

void EventAggregator::on(const Event event) {
    const uint32_t currentTimestamp = TimeSynchronizationDriver::getCurrentTimestamp();

    // Forward the incoming event once before any composites derived from it.
    if (outputHandler != nullptr) {
        outputHandler->on(event);
    }

    // Capture the event in the circular buffer
    captureEvent(event, currentTimestamp);

    // Check all patterns for a match
    checkForEventPatternMatch(currentTimestamp, event);

    // Remove expired captured events
    removeExpiredCapturedEvents(currentTimestamp);
}

// --- Private Helpers ---

// --- Pattern Matching ---

void EventAggregator::checkForEventPatternMatch(const uint32_t currentTimestamp, const Event& newEvent) const {
    if (outputHandler == nullptr || patternCount == 0) {
        return;
    }

    for (uint8_t p = 0; p < patternCount; p++) {
        const EventPattern& pattern = patterns[p];
        if (!pattern.active) {
            continue;
        }

        bool newEventContributes = false;
        for (uint8_t i = 0; i < pattern.identifierCount; ++i) {
            if (newEvent.identifier == pattern.identifiers[i] &&
                (pattern.sourceIds[i] == ANY_SOURCE || newEvent.sourceId == pattern.sourceIds[i])) {
                newEventContributes = true;
                break;
            }
        }
        if (!newEventContributes) {
            continue;
        }

        bool matched = false;

        if (pattern.logic == PatternLogic::And) {
            // AND logic: all identifiers must be present within the time window
            bool allFound = true;

            for (uint8_t i = 0; i < pattern.identifierCount; i++) {
                const int targetId = pattern.identifiers[i];
                const int targetSource = pattern.sourceIds[i];
                bool found = false;

                // Search captured events for this identifier within the time window
                for (uint8_t eventOffset = 0; eventOffset < capturedEventCount; eventOffset++) {
                    const uint8_t idx = getCapturedEventIndex(eventOffset);
                    const CapturedEvent& capturedEvent = capturedEvents[idx];

                    // Check if this captured event is within the time window
                    if (currentTimestamp - capturedEvent.timestamp > pattern.timeWindowSeconds) {
                        continue;
                    }

                    if (capturedEvent.identifier == targetId) {
                        // Check sourceId: ANY_SOURCE means "match any source"
                        if (targetSource == ANY_SOURCE || capturedEvent.sourceId == targetSource) {
                            found = true;
                            break;
                        }
                    }
                }

                if (!found) {
                    allFound = false;
                    break;
                }
            }

            matched = allFound;
        } else {
            // OR logic: any one identifier present within the time window triggers the pattern
            for (uint8_t i = 0; i < pattern.identifierCount; i++) {
                const int targetId = pattern.identifiers[i];
                const int targetSource = pattern.sourceIds[i];

                // Search captured events for this identifier within the time window
                for (uint8_t eventOffset = 0; eventOffset < capturedEventCount; eventOffset++) {
                    const uint8_t idx = getCapturedEventIndex(eventOffset);
                    const CapturedEvent& capturedEvent = capturedEvents[idx];

                    // Check if this captured event is within the time window
                    if (currentTimestamp - capturedEvent.timestamp > pattern.timeWindowSeconds) {
                        continue;
                    }

                    if (capturedEvent.identifier == targetId) {
                        // Check sourceId: ANY_SOURCE means "match any source"
                        if (targetSource == ANY_SOURCE || capturedEvent.sourceId == targetSource) {
                            matched = true;
                            break;
                        }
                    }
                }

                if (matched) {
                    break;
                }
            }
        }

        // If pattern matched, check condition and emit
        if (matched) {
            // Check the triggering condition if set, passing the composite event ID as context
            if (pattern.condition == nullptr || pattern.condition(pattern.compositeEventId)) {
                // Create and emit the composite event with the pattern's source ID
                const Event compositeEvent(pattern.compositeEventId, pattern.compositeEventSourceId);
                outputHandler->on(compositeEvent);
            }
        }
    }
}

// --- Captured Event Cleanup ---

void EventAggregator::removeExpiredCapturedEvents(const uint32_t currentTimestamp) {
    const uint32_t maxWindow = getMaxTimeWindow();
    if (maxWindow == 0 || capturedEventCount == 0) {
        return;
    }

    // Remove captured events older than the maximum time window
    uint8_t newCapturedEventCount = 0;
    for (uint8_t eventOffset = 0; eventOffset < capturedEventCount; eventOffset++) {
        const uint8_t idx = getCapturedEventIndex(eventOffset);

        if (const CapturedEvent& capturedEvent = capturedEvents[idx]; currentTimestamp - capturedEvent.timestamp <= maxWindow) {
            // Keep this event: compact it to the front
            if (newCapturedEventCount != eventOffset) {
                const uint8_t destIdx = getCapturedEventIndex(newCapturedEventCount);
                capturedEvents[destIdx] = capturedEvent;
            }
            newCapturedEventCount++;
        }
    }

    capturedEventCount = newCapturedEventCount;
}

uint32_t EventAggregator::getMaxTimeWindow() const {
    uint32_t maxWindow = 0;
    for (uint8_t p = 0; p < patternCount; p++) {
        if (patterns[p].active && patterns[p].timeWindowSeconds > maxWindow) {
            maxWindow = patterns[p].timeWindowSeconds;
        }
    }
    return maxWindow;
}

void EventAggregator::captureEvent(const Event event, const uint32_t timestamp) {
    capturedEvents[writeIndex].identifier = event.identifier;
    capturedEvents[writeIndex].sourceId = event.sourceId;
    capturedEvents[writeIndex].timestamp = timestamp;
    writeIndex = (writeIndex + 1) % EVENT_AGGREGATOR_MAX_CAPTURED_EVENTS;
    if (capturedEventCount < EVENT_AGGREGATOR_MAX_CAPTURED_EVENTS) {
        capturedEventCount++;
    }
}

uint8_t EventAggregator::getCapturedEventIndex(const uint8_t offsetFromOldest) const {
    return static_cast<uint8_t>((writeIndex - capturedEventCount + offsetFromOldest + EVENT_AGGREGATOR_MAX_CAPTURED_EVENTS) % EVENT_AGGREGATOR_MAX_CAPTURED_EVENTS);
}
