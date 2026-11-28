/**
 * @file test.cpp
 * @brief Host tests for EventAggregator.
 * @date 2026-11-19
 * @version 1.0
 */

#include <cstdio>
#include <cassert>

// Minimal millis() mock for this test
static unsigned long testFakeMillis = 0;
extern "C" unsigned long millis() { return testFakeMillis; }

#include "EventAggregator.h"
#include "RulesEngine.h"
#include "TimeSynchronizationDriver.h"

// --- Test Helpers ---

/**
 * @brief Mock output handler that captures composite events.
 */
class MockOutputHandler : public EventHandler {
public:
    int lastEventId = -1;
    int eventCount = 0;
    int expectedCompositeEventIds[4] = {};
    int expectedCompositeEventCount = 0;

    void on(Event event) override {
        if (event.sourceId != EventAggregator::DEVICE_SOURCE_ID) {
            return;
        }
        if (expectedCompositeEventCount > 0) {
            bool expected = false;
            for (int i = 0; i < expectedCompositeEventCount; ++i) {
                if (event.identifier == expectedCompositeEventIds[i]) {
                    expected = true;
                    break;
                }
            }
            if (!expected) {
                return;
            }
        }
        lastEventId = event.identifier;
        eventCount++;
    }

    void expectCompositeEventId(const int eventId) {
        if (expectedCompositeEventCount < 4) {
            expectedCompositeEventIds[expectedCompositeEventCount++] = eventId;
        }
    }

    void reset() {
        lastEventId = -1;
        eventCount = 0;
    }
};

class RecordingOutputHandler : public EventHandler {
public:
    int eventIds[16] = {};
    int sourceIds[16] = {};
    int eventCount = 0;

    void on(const Event event) override {
        if (eventCount < 16) {
            eventIds[eventCount] = event.identifier;
            sourceIds[eventCount] = event.sourceId;
            ++eventCount;
        }
    }
};

class CountingCommandHandler : public CommandHandler {
public:
    int handleCallCount = 0;

    void handle(const Command) override {
        ++handleCallCount;
    }
};

/**
 * @brief Advances the fake clock by the given seconds.
 */
void advanceTime(uint32_t seconds) {
    testFakeMillis += static_cast<unsigned long>(seconds) * 1000;
}

// --- Condition functions for pattern condition tests ---

static bool test24ConditionCalled = false;
static int test24ReceivedEventId = -1;
static bool test24Condition(int compositeEventId) {
    test24ConditionCalled = true;
    test24ReceivedEventId = compositeEventId;
    return true;
}

static bool test25ConditionCalled = false;
static int test25ReceivedEventId = -1;
static bool test25Condition(int compositeEventId) {
    test25ConditionCalled = true;
    test25ReceivedEventId = compositeEventId;
    return false;
}

static bool test27CargoBayEmpty = true;
static int test27ReceivedEventId = -1;
static bool test27IsCargoBayEmpty(int compositeEventId) {
    test27ReceivedEventId = compositeEventId;
    return test27CargoBayEmpty;
}

/// @brief Shared condition for test 28: handles multiple patterns via compositeEventId.
static int test28ReceivedEventId = -1;
static bool test28SharedCondition(int compositeEventId) {
    test28ReceivedEventId = compositeEventId;
    // Allow pattern 500, suppress pattern 501
    return compositeEventId == 500;
}

// --- Tests ---

void test_single_event_pattern() {
    std::printf("  Test 1: Single event pattern...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;
    MockOutputHandler handler;
    aggregator.setOutputHandler(&handler);

    // Pattern: event 30 → composite 400
    int ids[] = {30};
    aggregator.addAllMatchPattern(ids, 1, 5, 400);

    // Fire event 30
    aggregator.on(Event(30));

    assert(handler.eventCount == 1);
    assert(handler.lastEventId == 400);
    std::printf("    Single event pattern: PASS\n");
}

void test_two_event_and_pattern_within_window() {
    std::printf("  Test 2: Two-event AND pattern within time window...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;
    MockOutputHandler handler;
    aggregator.setOutputHandler(&handler);

    // Pattern: motion(30) AND door(72) within 5s → composite 400
    int ids[] = {30, 72};
    aggregator.addAllMatchPattern(ids, 2, 5, 400);

    // Fire motion at t=0
    aggregator.on(Event(30));
    assert(handler.eventCount == 0); // door not yet seen

    // Advance 2 seconds, fire door
    advanceTime(2);
    aggregator.on(Event(72));

    assert(handler.eventCount == 1);
    assert(handler.lastEventId == 400);
    std::printf("    Two-event AND within window: PASS\n");
}

void test_two_event_and_pattern_outside_window() {
    std::printf("  Test 3: Two-event AND pattern outside time window...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;
    MockOutputHandler handler;
    aggregator.setOutputHandler(&handler);

    // Pattern: motion(30) AND door(72) within 5s → composite 400
    int ids[] = {30, 72};
    aggregator.addAllMatchPattern(ids, 2, 5, 400);

    // Fire motion at t=0
    aggregator.on(Event(30));

    // Advance 10 seconds (outside 5s window), fire door
    advanceTime(10);
    aggregator.on(Event(72));

    assert(handler.eventCount == 0);
    std::printf("    Two-event AND outside window: PASS\n");
}

void test_three_event_pattern() {
    std::printf("  Test 4: Three-event pattern...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;
    MockOutputHandler handler;
    aggregator.setOutputHandler(&handler);

    // Pattern: motion(30) AND door(72) AND leak(70) within 10s → composite 401
    int ids[] = {30, 72, 70};
    aggregator.addAllMatchPattern(ids, 3, 10, 401);

    // Fire motion at t=0
    aggregator.on(Event(30));
    assert(handler.eventCount == 0);

    // Fire door at t=3
    advanceTime(3);
    aggregator.on(Event(72));
    assert(handler.eventCount == 0);

    // Fire leak at t=5 (within 10s of motion)
    advanceTime(2);
    aggregator.on(Event(70));

    assert(handler.eventCount == 1);
    assert(handler.lastEventId == 401);
    std::printf("    Three-event pattern: PASS\n");
}

void test_multiple_patterns_registered() {
    std::printf("  Test 5: Multiple patterns registered...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;
    MockOutputHandler handler;
    aggregator.setOutputHandler(&handler);

    // Pattern 1: motion(30) AND door(72) within 5s → 400
    int ids1[] = {30, 72};
    aggregator.addAllMatchPattern(ids1, 2, 5, 400);

    // Pattern 2: leak(70) AND gas(40) within 10s → 401
    int ids2[] = {70, 40};
    aggregator.addAllMatchPattern(ids2, 2, 10, 401);

    // Fire motion at t=0
    aggregator.on(Event(30));

    // Fire door at t=2 → pattern 1 matches
    advanceTime(2);
    aggregator.on(Event(72));
    assert(handler.eventCount == 1);
    assert(handler.lastEventId == 400);

    // Fire leak at t=7 (motion at t=0 is now outside 5s window)
    advanceTime(5);
    aggregator.on(Event(70));
    assert(handler.eventCount == 1); // gas not yet seen

    // Fire gas at t=12 → pattern 2 matches (leak at t=7, gas at t=12, within 10s)
    advanceTime(5);
    aggregator.on(Event(40));
    assert(handler.eventCount == 2);
    assert(handler.lastEventId == 401);
    std::printf("    Multiple patterns: PASS\n");
}

void test_captured_event_cleanup() {
    std::printf("  Test 6: Captured event cleanup...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;
    MockOutputHandler handler;
    aggregator.setOutputHandler(&handler);

    // Pattern: motion(30) within 5s → 400
    int ids[] = {30};
    aggregator.addAllMatchPattern(ids, 1, 5, 400);

    // Fire motion at t=0
    aggregator.on(Event(30));
    assert(handler.eventCount == 1);

    // Advance 10 seconds (outside 5s window)
    advanceTime(10);

    // Fire motion again — the old captured event should be cleaned up,
    // but this new event should still match (single event pattern)
    aggregator.on(Event(30));
    assert(handler.eventCount == 2);
    std::printf("    Captured event cleanup: PASS\n");
}

void test_circular_buffer_overflow() {
    std::printf("  Test 7: Circular buffer overflow...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;
    MockOutputHandler handler;
    aggregator.setOutputHandler(&handler);

    // Pattern: event 99 within 100s → 400
    int ids[] = {99};
    aggregator.addAllMatchPattern(ids, 1, 100, 400);

    // Fill the buffer with 33 events (1 more than max 32)
    for (int i = 0; i < 33; i++) {
        advanceTime(1);
        aggregator.on(Event(99));
    }

    // The aggregator should not crash and should still process events
    assert(handler.eventCount == 33);
    std::printf("    Circular buffer overflow: PASS\n");
}

void test_no_patterns_registered() {
    std::printf("  Test 8: No patterns registered...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;
    MockOutputHandler handler;
    aggregator.setOutputHandler(&handler);

    // No patterns — events should be captured but no composite emitted
    aggregator.on(Event(30));
    aggregator.on(Event(72));

    assert(handler.eventCount == 0);
    std::printf("    No patterns registered: PASS\n");
}

void test_pattern_array_full() {
    std::printf("  Test 9: Pattern array full...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;

    // Fill all 4 pattern slots
    assert(aggregator.addAllMatchPattern((int[]){30}, 1, 5, 400) == true);
    assert(aggregator.addAllMatchPattern((int[]){72}, 1, 5, 401) == true);
    assert(aggregator.addAllMatchPattern((int[]){70}, 1, 5, 402) == true);
    assert(aggregator.addAllMatchPattern((int[]){40}, 1, 5, 403) == true);

    // 5th pattern should fail
    assert(aggregator.addAllMatchPattern((int[]){99}, 1, 5, 404) == false);
    std::printf("    Pattern array full: PASS\n");
}

void test_invalid_pattern_parameters() {
    std::printf("  Test 10: Invalid pattern parameters...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;

    // Zero identifiers should fail
    assert(aggregator.addAllMatchPattern((int[]){}, 0, 5, 400) == false);

    // Too many identifiers should fail
    assert(aggregator.addAllMatchPattern((int[]){1, 2, 3, 4, 5}, 5, 5, 400) == false);

    // Valid pattern should succeed
    assert(aggregator.addAllMatchPattern((int[]){30}, 1, 5, 400) == true);
    std::printf("    Invalid pattern parameters: PASS\n");
}

void test_no_output_handler() {
    std::printf("  Test 11: No output handler set...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;
    // No output handler set

    int ids[] = {30};
    aggregator.addAllMatchPattern(ids, 1, 5, 400);

    // Should not crash even without output handler
    aggregator.on(Event(30));
    std::printf("    No output handler: PASS\n");
}

void test_events_arrive_reverse_order() {
    std::printf("  Test 12: Events arrive in reverse order...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;
    MockOutputHandler handler;
    aggregator.setOutputHandler(&handler);

    // Pattern: motion(30) AND door(72) within 5s → composite 400
    int ids[] = {30, 72};
    aggregator.addAllMatchPattern(ids, 2, 5, 400);

    // Fire door first (reverse order) at t=0
    aggregator.on(Event(72));
    assert(handler.eventCount == 0);  // motion not yet seen

    // Fire motion at t=2 (within 5s window)
    advanceTime(2);
    aggregator.on(Event(30));

    assert(handler.eventCount == 1);
    assert(handler.lastEventId == 400);
    std::printf("    Reverse order: PASS\n");
}

void test_time_window_boundary_exact() {
    std::printf("  Test 13: Time window boundary (exact limit)...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;
    MockOutputHandler handler;
    aggregator.setOutputHandler(&handler);

    // Pattern: motion(30) AND door(72) within 5s → composite 400
    int ids[] = {30, 72};
    aggregator.addAllMatchPattern(ids, 2, 5, 400);

    // Fire motion at t=0
    aggregator.on(Event(30));

    // Advance exactly 5 seconds (at the boundary)
    advanceTime(5);
    aggregator.on(Event(72));

    // Event at exactly the time window boundary should match
    assert(handler.eventCount == 1);
    assert(handler.lastEventId == 400);
    std::printf("    Time window boundary exact: PASS\n");
}

void test_time_window_boundary_just_over() {
    std::printf("  Test 14: Time window boundary (just over limit)...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;
    MockOutputHandler handler;
    aggregator.setOutputHandler(&handler);

    // Pattern: motion(30) AND door(72) within 5s → composite 400
    int ids[] = {30, 72};
    aggregator.addAllMatchPattern(ids, 2, 5, 400);

    // Fire motion at t=0
    aggregator.on(Event(30));

    // Advance 6 seconds (1 second over the 5s window)
    advanceTime(6);
    aggregator.on(Event(72));

    // Event just over the time window should not match
    assert(handler.eventCount == 0);
    std::printf("    Time window boundary just over: PASS\n");
}

void test_duplicate_events_in_pattern() {
    std::printf("  Test 15: Duplicate event identifiers in pattern...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;
    MockOutputHandler handler;
    aggregator.setOutputHandler(&handler);

    // Pattern with duplicate identifier: motion(30) AND motion(30) within 5s → 400
    // The pattern matching logic looks for at least one occurrence of each identifier,
    // so a duplicate identifier should still match with a single event.
    int ids[] = {30, 30};
    aggregator.addAllMatchPattern(ids, 2, 5, 400);

    // Fire motion once
    aggregator.on(Event(30));

    // The pattern has two entries for identifier 30, but both are satisfied
    // by the single captured event (the loop breaks after finding the first match)
    assert(handler.eventCount == 1);
    assert(handler.lastEventId == 400);
    std::printf("    Duplicate event identifiers: PASS\n");
}

void test_pattern_matches_multiple_times() {
    std::printf("  Test 16: Pattern matches multiple times...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;
    MockOutputHandler handler;
    aggregator.setOutputHandler(&handler);

    // Pattern: motion(30) AND door(72) within 5s → composite 400
    int ids[] = {30, 72};
    aggregator.addAllMatchPattern(ids, 2, 5, 400);

    // First match: motion at t=0, door at t=2
    aggregator.on(Event(30));
    advanceTime(2);
    aggregator.on(Event(72));
    assert(handler.eventCount == 1);

    // Advance past the time window so old events expire
    advanceTime(10);

    // Second match: motion at t=12, door at t=14
    aggregator.on(Event(30));
    advanceTime(2);
    aggregator.on(Event(72));
    assert(handler.eventCount == 2);
    assert(handler.lastEventId == 400);
    std::printf("    Pattern matches multiple times: PASS\n");
}

void test_same_event_contributes_to_multiple_patterns() {
    std::printf("  Test 17: Same event contributes to multiple patterns...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;
    MockOutputHandler handler;
    aggregator.setOutputHandler(&handler);

    // Pattern 1: motion(30) AND door(72) within 5s → 400
    int ids1[] = {30, 72};
    aggregator.addAllMatchPattern(ids1, 2, 5, 400);

    // Pattern 2: door(72) AND leak(70) within 10s → 401
    int ids2[] = {72, 70};
    aggregator.addAllMatchPattern(ids2, 2, 10, 401);

    // Fire motion at t=0
    aggregator.on(Event(30));
    assert(handler.eventCount == 0);

    // Fire door at t=2 — contributes to pattern 1
    advanceTime(2);
    aggregator.on(Event(72));
    assert(handler.eventCount == 1);  // pattern 1 matches
    assert(handler.lastEventId == 400);

    // Advance past pattern 1's window so motion expires, but door is still
    // within pattern 2's window (door at t=2, current t=6, age=4 <= 10)
    advanceTime(4);
    aggregator.on(Event(70));
    // At t=6: pattern 1 does NOT match (motion at t=0, age=6 > 5),
    // pattern 2 matches (door at t=2, age=4 <= 10; leak at t=6)
    assert(handler.eventCount == 2);
    assert(handler.lastEventId == 401);
    std::printf("    Same event in multiple patterns: PASS\n");
}

void test_source_aware_pattern_exact_match() {
    std::printf("  Test 18: Source-aware pattern (exact match)...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;
    MockOutputHandler handler;
    aggregator.setOutputHandler(&handler);

    // Pattern: DATA_READ(20) from source 5 AND door(72) from source 3 within 5s → 400
    int ids[] = {20, 72};
    int sources[] = {5, 3};
    aggregator.addAllMatchPattern(ids, 2, sources, 5, 400);

    // Fire DATA_READ from source 5 at t=0
    aggregator.on(Event(20, 5));
    assert(handler.eventCount == 0);

    // Fire door from source 3 at t=2
    advanceTime(2);
    aggregator.on(Event(72, 3));

    assert(handler.eventCount == 1);
    assert(handler.lastEventId == 400);
    std::printf("    Source-aware exact match: PASS\n");
}

void test_source_aware_pattern_wrong_source() {
    std::printf("  Test 19: Source-aware pattern (wrong source rejected)...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;
    MockOutputHandler handler;
    aggregator.setOutputHandler(&handler);

    // Pattern: DATA_READ(20) from source 5 AND door(72) from any source within 5s → 400
    int ids[] = {20, 72};
    int sources[] = {5, EventAggregator::ANY_SOURCE};
    aggregator.addAllMatchPattern(ids, 2, sources, 5, 400);

    // Fire DATA_READ from source 7 (wrong source) at t=0
    aggregator.on(Event(20, 7));
    assert(handler.eventCount == 0);

    // Fire door from source 3 at t=2
    advanceTime(2);
    aggregator.on(Event(72, 3));

    // Pattern should NOT match because DATA_READ came from source 7, not 5
    assert(handler.eventCount == 0);
    std::printf("    Source-aware wrong source rejected: PASS\n");
}

void test_source_aware_pattern_wildcard() {
    std::printf("  Test 20: Source-aware pattern (wildcard source)...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;
    MockOutputHandler handler;
    aggregator.setOutputHandler(&handler);

    // Pattern: DATA_READ(20) from any source AND door(72) from source 3 within 5s → 400
    int ids[] = {20, 72};
    int sources[] = {EventAggregator::ANY_SOURCE, 3};
    aggregator.addAllMatchPattern(ids, 2, sources, 5, 400);

    // Fire DATA_READ from source 99 (any source is fine) at t=0
    aggregator.on(Event(20, 99));
    assert(handler.eventCount == 0);

    // Fire door from source 3 at t=2
    advanceTime(2);
    aggregator.on(Event(72, 3));

    assert(handler.eventCount == 1);
    assert(handler.lastEventId == 400);
    std::printf("    Source-aware wildcard: PASS\n");
}

void test_source_aware_pattern_mixed_sources() {
    std::printf("  Test 21: Source-aware pattern (mixed exact and wildcard)...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;
    MockOutputHandler handler;
    aggregator.setOutputHandler(&handler);

    // Pattern: DATA_READ(20) from source 5 AND door(72) from any source within 5s → 400
    int ids[] = {20, 72};
    int sources[] = {5, EventAggregator::ANY_SOURCE};
    aggregator.addAllMatchPattern(ids, 2, sources, 5, 400);

    // Fire DATA_READ from source 5 at t=0
    aggregator.on(Event(20, 5));
    assert(handler.eventCount == 0);

    // Fire door from source 99 (wildcard, any source) at t=2
    advanceTime(2);
    aggregator.on(Event(72, 99));

    assert(handler.eventCount == 1);
    assert(handler.lastEventId == 400);
    std::printf("    Source-aware mixed sources: PASS\n");
}

void test_source_aware_two_patterns_different_sources() {
    std::printf("  Test 22: Two source-aware patterns (different sources)...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;
    MockOutputHandler handler;
    aggregator.setOutputHandler(&handler);

    // Pattern 1: DATA_READ(20) from source 5 AND door(72) within 5s → 400
    int ids1[] = {20, 72};
    int sources1[] = {5, EventAggregator::ANY_SOURCE};
    aggregator.addAllMatchPattern(ids1, 2, sources1, 5, 400);

    // Pattern 2: DATA_READ(20) from source 8 AND door(72) within 5s → 401
    int ids2[] = {20, 72};
    int sources2[] = {8, EventAggregator::ANY_SOURCE};
    aggregator.addAllMatchPattern(ids2, 2, sources2, 5, 401);

    // Fire DATA_READ from source 8 at t=0
    aggregator.on(Event(20, 8));
    assert(handler.eventCount == 0);

    // Fire door at t=2 — should only match pattern 2
    advanceTime(2);
    aggregator.on(Event(72, 3));
    assert(handler.eventCount == 1);
    assert(handler.lastEventId == 401);

    // Advance past window
    advanceTime(10);

    // Fire DATA_READ from source 5 at t=12
    aggregator.on(Event(20, 5));
    assert(handler.eventCount == 1);

    // Fire door at t=14 — should only match pattern 1
    advanceTime(2);
    aggregator.on(Event(72, 3));
    assert(handler.eventCount == 2);
    assert(handler.lastEventId == 400);
    std::printf("    Two source-aware patterns: PASS\n");
}

void test_backward_compat_no_source_ids() {
    std::printf("  Test 23: Backward compatibility (no source IDs)...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;
    MockOutputHandler handler;
    aggregator.setOutputHandler(&handler);

    // Use the old API (no sourceIds) — should match any source
    int ids[] = {20, 72};
    aggregator.addAllMatchPattern(ids, 2, 5, 400);

    // Fire DATA_READ from source 42
    aggregator.on(Event(20, 42));
    assert(handler.eventCount == 0);

    // Fire door from source 7
    advanceTime(2);
    aggregator.on(Event(72, 7));

    // Should match because old API defaults to wildcard source
    assert(handler.eventCount == 1);
    assert(handler.lastEventId == 400);
    std::printf("    Backward compatibility: PASS\n");
}

void test_condition_returns_true() {
    std::printf("  Test 24: Condition function receives compositeEventId (emits)...\n");
    testFakeMillis = 0;
    test24ConditionCalled = false;
    test24ReceivedEventId = -1;

    EventAggregator aggregator;
    MockOutputHandler handler;
    aggregator.setOutputHandler(&handler);

    int ids[] = {30, 72};
    aggregator.addAllMatchPattern(ids, 2, nullptr, 5, 400, test24Condition);

    // Fire motion at t=0
    aggregator.on(Event(30));
    assert(handler.eventCount == 0);

    // Fire door at t=2 — pattern matches, condition returns true
    advanceTime(2);
    aggregator.on(Event(72));

    assert(handler.eventCount == 1);
    assert(handler.lastEventId == 400);
    assert(test24ConditionCalled == true);
    assert(test24ReceivedEventId == 400);  // Condition received the composite event ID
    std::printf("    Condition receives compositeEventId: PASS\n");
}

void test_condition_returns_false() {
    std::printf("  Test 25: Condition function returns false (suppresses)...\n");
    testFakeMillis = 0;
    test25ConditionCalled = false;
    test25ReceivedEventId = -1;

    EventAggregator aggregator;
    MockOutputHandler handler;
    aggregator.setOutputHandler(&handler);

    int ids[] = {30, 72};
    aggregator.addAllMatchPattern(ids, 2, nullptr, 5, 400, test25Condition);

    // Fire motion at t=0
    aggregator.on(Event(30));
    assert(handler.eventCount == 0);

    // Fire door at t=2 — pattern matches, but condition returns false
    advanceTime(2);
    aggregator.on(Event(72));

    assert(handler.eventCount == 0);  // Composite event suppressed
    assert(test25ConditionCalled == true);  // Condition was called
    assert(test25ReceivedEventId == 400);  // Condition received the composite event ID even when suppressing
    std::printf("    Condition returns false: PASS\n");
}

void test_condition_nullptr_always_emits() {
    std::printf("  Test 26: Nullptr condition (always emits)...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;
    MockOutputHandler handler;
    aggregator.setOutputHandler(&handler);

    // Explicitly pass nullptr condition — should always emit
    int ids[] = {30, 72};
    aggregator.addAllMatchPattern(ids, 2, nullptr, 5, 400, nullptr);

    // Fire motion at t=0
    aggregator.on(Event(30));
    assert(handler.eventCount == 0);

    // Fire door at t=2
    advanceTime(2);
    aggregator.on(Event(72));

    assert(handler.eventCount == 1);
    assert(handler.lastEventId == 400);
    std::printf("    Nullptr condition: PASS\n");
}

void test_condition_sensor_value_check() {
    std::printf("  Test 27: Condition checks sensor value (practical scenario)...\n");
    testFakeMillis = 0;
    test27CargoBayEmpty = true;
    test27ReceivedEventId = -1;

    EventAggregator aggregator;
    MockOutputHandler handler;
    aggregator.setOutputHandler(&handler);

    int ids[] = {72, 20};  // door opened AND weight change
    aggregator.addAllMatchPattern(ids, 2, nullptr, 30, 500, test27IsCargoBayEmpty);

    // Scenario 1: Cargo bay is empty — condition returns true, event emitted
    test27CargoBayEmpty = true;
    aggregator.on(Event(72));  // door opened at t=0
    assert(handler.eventCount == 0);
    advanceTime(2);
    aggregator.on(Event(20));  // weight change at t=2
    assert(handler.eventCount == 1);
    assert(handler.lastEventId == 500);
    assert(test27ReceivedEventId == 500);  // Condition received the composite event ID

    // Advance past time window
    advanceTime(30);

    // Scenario 2: Cargo bay has cargo — condition returns false, event suppressed
    test27CargoBayEmpty = false;
    aggregator.on(Event(72));  // door opened at t=32
    assert(handler.eventCount == 1);
    advanceTime(2);
    aggregator.on(Event(20));  // weight change at t=34
    assert(handler.eventCount == 1);  // Still 1 — event suppressed
    assert(test27ReceivedEventId == 500);  // Condition still received the ID
    std::printf("    Condition sensor value check: PASS\n");
}

void test_condition_shared_function_multiple_patterns() {
    std::printf("  Test 28: Shared condition function handles multiple patterns...\n");
    testFakeMillis = 0;
    test28ReceivedEventId = -1;

    EventAggregator aggregator;
    MockOutputHandler handler;
    aggregator.setOutputHandler(&handler);

    // Two patterns share the same condition function.
    // The condition allows composite event 500 but suppresses 501.
    int ids1[] = {30, 72};  // motion + door → 500
    aggregator.addAllMatchPattern(ids1, 2, nullptr, 10, 500, test28SharedCondition);

    int ids2[] = {70, 40};  // leak + gas → 501
    aggregator.addAllMatchPattern(ids2, 2, nullptr, 10, 501, test28SharedCondition);

    // Fire motion at t=0
    aggregator.on(Event(30));
    assert(handler.eventCount == 0);

    // Fire door at t=2 — pattern 500 matches, condition allows it
    advanceTime(2);
    aggregator.on(Event(72));
    assert(handler.eventCount == 1);
    assert(handler.lastEventId == 500);
    assert(test28ReceivedEventId == 500);

    // Advance past time window
    advanceTime(10);

    // Fire leak at t=14
    aggregator.on(Event(70));
    assert(handler.eventCount == 1);

    // Fire gas at t=16 — pattern 501 matches, but condition suppresses it
    advanceTime(2);
    aggregator.on(Event(40));
    assert(handler.eventCount == 1);  // Still 1 — event 501 suppressed
    assert(test28ReceivedEventId == 501);  // Condition was called with 501
    std::printf("    Shared condition function: PASS\n");
}

void test_remove_existing_pattern() {
    std::printf("  Test 29: Remove existing pattern...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;
    MockOutputHandler handler;
    aggregator.setOutputHandler(&handler);

    // Register two patterns
    int ids1[] = {30, 72};  // motion + door → 400
    aggregator.addAllMatchPattern(ids1, 2, 5, 400);

    int ids2[] = {70, 40};  // leak + gas → 401
    aggregator.addAllMatchPattern(ids2, 2, 10, 401);

    // Verify pattern 400 fires
    aggregator.on(Event(30));
    advanceTime(2);
    aggregator.on(Event(72));
    assert(handler.eventCount == 1);
    assert(handler.lastEventId == 400);

    // Remove pattern 400
    assert(aggregator.removePattern(400) == true);

    // Advance past time window
    advanceTime(10);

    // Fire motion + door again — pattern 400 should NOT fire
    aggregator.on(Event(30));
    advanceTime(2);
    aggregator.on(Event(72));
    assert(handler.eventCount == 1);  // Still 1 — pattern 400 removed

    // Verify pattern 401 still works
    advanceTime(10);
    aggregator.on(Event(70));
    advanceTime(2);
    aggregator.on(Event(40));
    assert(handler.eventCount == 2);
    assert(handler.lastEventId == 401);
    std::printf("    Remove existing pattern: PASS\n");
}

void test_remove_non_existent_pattern() {
    std::printf("  Test 30: Remove non-existent pattern...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;

    // Register one pattern
    int ids[] = {30};
    aggregator.addAllMatchPattern(ids, 1, 5, 400);

    // Try to remove a pattern that doesn't exist
    assert(aggregator.removePattern(999) == false);

    // Original pattern should still be registered
    MockOutputHandler handler;
    aggregator.setOutputHandler(&handler);
    aggregator.on(Event(30));
    assert(handler.eventCount == 1);
    assert(handler.lastEventId == 400);
    std::printf("    Remove non-existent pattern: PASS\n");
}

void test_remove_and_readd_pattern() {
    std::printf("  Test 31: Remove and re-add pattern (slot reuse)...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;
    MockOutputHandler handler;
    aggregator.setOutputHandler(&handler);

    // Fill all 4 pattern slots
    assert(aggregator.addAllMatchPattern((int[]){30}, 1, 5, 400) == true);
    assert(aggregator.addAllMatchPattern((int[]){72}, 1, 5, 401) == true);
    assert(aggregator.addAllMatchPattern((int[]){70}, 1, 5, 402) == true);
    assert(aggregator.addAllMatchPattern((int[]){40}, 1, 5, 403) == true);

    // 5th pattern should fail (array full)
    assert(aggregator.addAllMatchPattern((int[]){99}, 1, 5, 404) == false);

    // Remove pattern 400
    assert(aggregator.removePattern(400) == true);

    // Now a new pattern should succeed (slot freed)
    assert(aggregator.addAllMatchPattern((int[]){99}, 1, 5, 404) == true);

    // Verify the new pattern works
    aggregator.on(Event(99));
    assert(handler.eventCount == 1);
    assert(handler.lastEventId == 404);

    // Verify old pattern 400 no longer fires
    advanceTime(10);
    aggregator.on(Event(30));
    assert(handler.eventCount == 1);  // Still 1 — pattern 400 was removed
    std::printf("    Remove and re-add pattern: PASS\n");
}

void test_remove_middle_pattern() {
    std::printf("  Test 32: Remove middle pattern (compaction)...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;
    MockOutputHandler handler;
    aggregator.setOutputHandler(&handler);

    // Register three patterns
    int ids1[] = {30};  // → 400
    aggregator.addAllMatchPattern(ids1, 1, 5, 400);

    int ids2[] = {72};  // → 401
    aggregator.addAllMatchPattern(ids2, 1, 5, 401);

    int ids3[] = {70};  // → 402
    aggregator.addAllMatchPattern(ids3, 1, 5, 402);

    // Remove the middle pattern (401)
    assert(aggregator.removePattern(401) == true);

    // Verify pattern 400 still works
    aggregator.on(Event(30));
    assert(handler.eventCount == 1);
    assert(handler.lastEventId == 400);

    // Advance past time window so event 30 expires and pattern 400 won't re-fire
    advanceTime(10);

    // Verify pattern 402 still works
    aggregator.on(Event(70));
    assert(handler.eventCount == 2);
    assert(handler.lastEventId == 402);

    // Advance past time window so event 70 expires
    advanceTime(10);

    // Verify pattern 401 does NOT fire
    aggregator.on(Event(72));
    assert(handler.eventCount == 2);  // Still 2 — pattern 401 removed

    // Re-add a new pattern should succeed (slot freed)
    assert(aggregator.addAllMatchPattern((int[]){99}, 1, 5, 403) == true);
    aggregator.on(Event(99));
    assert(handler.eventCount == 3);
    assert(handler.lastEventId == 403);
    std::printf("    Remove middle pattern: PASS\n");
}

void test_cleanup_patterns() {
    std::printf("  Test 33: Cleanup all patterns...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;
    MockOutputHandler handler;
    aggregator.setOutputHandler(&handler);

    // Register three patterns
    aggregator.addAllMatchPattern((int[]){30}, 1, 5, 400);
    aggregator.addAllMatchPattern((int[]){72}, 1, 5, 401);
    aggregator.addAllMatchPattern((int[]){70}, 1, 5, 402);

    // Verify patterns fire
    aggregator.on(Event(30));
    assert(handler.eventCount == 1);
    advanceTime(10);  // Expire event 30 so pattern 400 won't re-fire
    aggregator.on(Event(72));
    assert(handler.eventCount == 2);
    advanceTime(10);  // Expire event 72 so pattern 401 won't re-fire
    aggregator.on(Event(70));
    assert(handler.eventCount == 3);

    // Cleanup all patterns
    aggregator.cleanupPatterns();

    // Advance past time windows
    advanceTime(10);

    // None of the patterns should fire anymore
    aggregator.on(Event(30));
    assert(handler.eventCount == 3);  // Still 3
    aggregator.on(Event(72));
    assert(handler.eventCount == 3);
    aggregator.on(Event(70));
    assert(handler.eventCount == 3);

    // Can re-add patterns after cleanup
    assert(aggregator.addAllMatchPattern((int[]){99}, 1, 5, 403) == true);
    aggregator.on(Event(99));
    assert(handler.eventCount == 4);
    assert(handler.lastEventId == 403);
    std::printf("    Cleanup all patterns: PASS\n");
}

void test_or_pattern_single_event_triggers() {
    std::printf("  Test 34: OR pattern — any one event triggers...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;
    MockOutputHandler handler;
    aggregator.setOutputHandler(&handler);

    // OR pattern: motion(30) OR door(72) within 10s → composite 500
    int ids[] = {30, 72};
    aggregator.addAnyMatchPattern(ids, 2, 10, 500);

    // Fire motion only — should trigger the OR pattern
    aggregator.on(Event(30));
    assert(handler.eventCount == 1);
    assert(handler.lastEventId == 500);

    // Advance past time window
    advanceTime(10);

    // Fire door only — should also trigger the OR pattern
    aggregator.on(Event(72));
    assert(handler.eventCount == 2);
    assert(handler.lastEventId == 500);
    std::printf("    OR pattern single event triggers: PASS\n");
}

void test_or_pattern_neither_event_fires() {
    std::printf("  Test 35: OR pattern — neither event fires...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;
    MockOutputHandler handler;
    aggregator.setOutputHandler(&handler);

    // OR pattern: motion(30) OR door(72) within 10s → composite 500
    int ids[] = {30, 72};
    aggregator.addAnyMatchPattern(ids, 2, 10, 500);

    // Fire an unrelated event
    aggregator.on(Event(99));
    assert(handler.eventCount == 0);
    std::printf("    OR pattern neither event: PASS\n");
}

void test_or_pattern_source_aware() {
    std::printf("  Test 36: OR pattern with source filtering...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;
    MockOutputHandler handler;
    aggregator.setOutputHandler(&handler);

    // OR pattern: DATA_READ(20) from source 5 OR door(72) from any source → 500
    int ids[] = {20, 72};
    int sources[] = {5, EventAggregator::ANY_SOURCE};
    aggregator.addAnyMatchPattern(ids, 2, sources, 10, 500);

    // Fire DATA_READ from source 7 (wrong source) — should NOT trigger
    aggregator.on(Event(20, 7));
    assert(handler.eventCount == 0);

    // Fire DATA_READ from source 5 (correct source) — should trigger
    aggregator.on(Event(20, 5));
    assert(handler.eventCount == 1);
    assert(handler.lastEventId == 500);

    // Advance past time window
    advanceTime(10);

    // Fire door from any source — should trigger
    aggregator.on(Event(72, 99));
    assert(handler.eventCount == 2);
    assert(handler.lastEventId == 500);
    std::printf("    OR pattern source-aware: PASS\n");
}

void test_or_pattern_time_window_expiry() {
    std::printf("  Test 37: OR pattern time window expiry...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;
    MockOutputHandler handler;
    aggregator.setOutputHandler(&handler);

    // OR pattern: motion(30) OR door(72) within 5s → 500
    int ids[] = {30, 72};
    aggregator.addAnyMatchPattern(ids, 2, 5, 500);

    // Fire motion at t=0 — triggers immediately
    aggregator.on(Event(30));
    assert(handler.eventCount == 1);

    // Advance past time window
    advanceTime(10);

    // Fire a different event — no match
    aggregator.on(Event(99));
    assert(handler.eventCount == 1);  // Still 1, event 30 expired
    std::printf("    OR pattern time window expiry: PASS\n");
}

void test_mixed_and_or_patterns() {
    std::printf("  Test 38: Mixed AND and OR patterns...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;
    MockOutputHandler handler;
    aggregator.setOutputHandler(&handler);

    // AND pattern: motion(30) AND door(72) within 10s → 400
    int andIds[] = {30, 72};
    aggregator.addAllMatchPattern(andIds, 2, 10, 400);

    // OR pattern: leak(70) OR gas(40) within 10s → 500
    int orIds[] = {70, 40};
    aggregator.addAnyMatchPattern(orIds, 2, 10, 500);

    // Fire motion at t=0 — no pattern triggers (AND needs door, OR doesn't care about motion)
    aggregator.on(Event(30));
    assert(handler.eventCount == 0);

    // Fire leak at t=2 — OR pattern triggers (leak alone)
    advanceTime(2);
    aggregator.on(Event(70));
    assert(handler.eventCount == 1);
    assert(handler.lastEventId == 500);

    // Advance past both time windows so all events expire
    advanceTime(10);  // t=12: leak at t=2 expired, motion at t=0 expired

    // Fire gas at t=14 — OR pattern triggers (gas alone)
    advanceTime(2);
    aggregator.on(Event(40));
    assert(handler.eventCount == 2);
    assert(handler.lastEventId == 500);

    // Advance past time window so gas expires (gas at t=14, need age > 10)
    advanceTime(11);  // t=25: gas at t=14 expired (age=11 > 10)

    // Fire motion at t=25 — no trigger yet (AND needs door)
    aggregator.on(Event(30));
    assert(handler.eventCount == 2);

    // Fire door at t=26 — AND pattern triggers (motion + door within 10s)
    advanceTime(2);
    aggregator.on(Event(72));
    assert(handler.eventCount == 3);
    assert(handler.lastEventId == 400);
    std::printf("    Mixed AND and OR patterns: PASS\n");
}

void test_or_pattern_array_full() {
    std::printf("  Test 39: OR pattern array full...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;

    // Fill all 4 pattern slots with OR patterns
    assert(aggregator.addAnyMatchPattern((int[]){30, 72}, 2, 5, 500) == true);
    assert(aggregator.addAnyMatchPattern((int[]){70, 40}, 2, 5, 501) == true);
    assert(aggregator.addAnyMatchPattern((int[]){99}, 1, 5, 502) == true);
    assert(aggregator.addAnyMatchPattern((int[]){100}, 1, 5, 503) == true);

    // 5th OR pattern should fail (array full)
    assert(aggregator.addAnyMatchPattern((int[]){200}, 1, 5, 504) == false);

    // Also verify AND pattern fails when array is full
    assert(aggregator.addAllMatchPattern((int[]){200}, 1, 5, 600) == false);
    std::printf("    OR pattern array full: PASS\n");
}

void test_or_pattern_remove_and_cleanup() {
    std::printf("  Test 40: OR pattern remove and cleanup...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;
    MockOutputHandler handler;
    aggregator.setOutputHandler(&handler);

    // Register an OR pattern
    int ids[] = {30, 72};
    aggregator.addAnyMatchPattern(ids, 2, 5, 500);

    // Verify it fires
    aggregator.on(Event(30));
    assert(handler.eventCount == 1);
    assert(handler.lastEventId == 500);

    // Remove the OR pattern
    assert(aggregator.removePattern(500) == true);

    // Advance past time window
    advanceTime(10);

    // Should no longer fire
    aggregator.on(Event(30));
    assert(handler.eventCount == 1);  // Still 1

    // Re-add as AND pattern to verify slot reuse
    aggregator.addAllMatchPattern(ids, 2, 5, 500);

    // Fire only motion — AND pattern should NOT fire (needs both)
    aggregator.on(Event(30));
    assert(handler.eventCount == 1);  // Still 1

    // Fire door — AND pattern should fire
    advanceTime(2);
    aggregator.on(Event(72));
    assert(handler.eventCount == 2);
    assert(handler.lastEventId == 500);
    std::printf("    OR pattern remove and cleanup: PASS\n");
}

void test_nested_aggregators_basic_chain() {
    std::printf("  Test 41: Nested aggregators — basic chain...\n");
    testFakeMillis = 0;

    // Aggregator A: motion(30) AND door(72) within 5s → composite 500
    // Aggregator B: composite 500 within 10s → composite 600
    // Final handler receives composite 600

    EventAggregator aggregatorA;
    EventAggregator aggregatorB;
    MockOutputHandler handler;
    handler.expectCompositeEventId(600);

    // Chain: A → B → handler
    aggregatorA.setOutputHandler(&aggregatorB);
    aggregatorB.setOutputHandler(&handler);

    // Register pattern in A
    int idsA[] = {30, 72};
    aggregatorA.addAllMatchPattern(idsA, 2, 5, 500);

    // Register pattern in B (matches composite event from A)
    int idsB[] = {500};
    aggregatorB.addAllMatchPattern(idsB, 1, 10, 600);

    // Fire motion at t=0
    aggregatorA.on(Event(30));
    assert(handler.eventCount == 0);  // door not yet seen

    // Fire door at t=2 — A matches, emits 500 → B matches, emits 600
    advanceTime(2);
    aggregatorA.on(Event(72));

    assert(handler.eventCount == 1);
    assert(handler.lastEventId == 600);
    std::printf("    Nested aggregators basic chain: PASS\n");
}

void test_nested_aggregators_source_filtering() {
    std::printf("  Test 42: Nested aggregators — source filtering by DEVICE_SOURCE_ID...\n");
    testFakeMillis = 0;

    // Aggregator A: motion(30) AND door(72) within 5s → composite 500 (sourceId = DEVICE_SOURCE_ID)
    // Aggregator B: composite 500 from DEVICE_SOURCE_ID within 10s → composite 600
    // The source filter ensures B only matches composite events from A, not raw sensor events.

    EventAggregator aggregatorA;
    EventAggregator aggregatorB;
    MockOutputHandler handler;
    handler.expectCompositeEventId(600);

    aggregatorA.setOutputHandler(&aggregatorB);
    aggregatorB.setOutputHandler(&handler);

    int idsA[] = {30, 72};
    aggregatorA.addAllMatchPattern(idsA, 2, 5, 500);

    // B's pattern requires sourceId = DEVICE_SOURCE_ID (the default for composite events)
    int idsB[] = {500};
    int sourcesB[] = {EventAggregator::DEVICE_SOURCE_ID};
    aggregatorB.addAllMatchPattern(idsB, 1, sourcesB, 10, 600);

    // Fire motion + door through A — composite 500 has sourceId = DEVICE_SOURCE_ID
    aggregatorA.on(Event(30));
    advanceTime(2);
    aggregatorA.on(Event(72));

    assert(handler.eventCount == 1);
    assert(handler.lastEventId == 600);

    // Advance past B's time window so the composite event from the chain expires
    advanceTime(11);

    // Directly fire event 500 with a sensor sourceId — B should NOT match
    aggregatorB.on(Event(500, 5));  // sourceId 5 is a sensor pin, not DEVICE_SOURCE_ID
    assert(handler.eventCount == 1);  // Still 1 — rejected by source filter
    std::printf("    Nested aggregators source filtering: PASS\n");
}

void test_nested_aggregators_three_level_chain() {
    std::printf("  Test 43: Nested aggregators — three-level chain...\n");
    testFakeMillis = 0;

    // Level 1: motion(30) AND door(72) within 5s → composite 500
    // Level 2: composite 500 within 10s → composite 600
    // Level 3: composite 600 within 15s → composite 700

    EventAggregator aggregator1;
    EventAggregator aggregator2;
    EventAggregator aggregator3;
    MockOutputHandler handler;
    handler.expectCompositeEventId(700);

    aggregator1.setOutputHandler(&aggregator2);
    aggregator2.setOutputHandler(&aggregator3);
    aggregator3.setOutputHandler(&handler);

    int ids1[] = {30, 72};
    aggregator1.addAllMatchPattern(ids1, 2, 5, 500);

    int ids2[] = {500};
    aggregator2.addAllMatchPattern(ids2, 1, 10, 600);

    int ids3[] = {600};
    aggregator3.addAllMatchPattern(ids3, 1, 15, 700);

    // Fire motion at t=0
    aggregator1.on(Event(30));
    assert(handler.eventCount == 0);

    // Fire door at t=2 — chain reaction: 500 → 600 → 700
    advanceTime(2);
    aggregator1.on(Event(72));

    assert(handler.eventCount == 1);
    assert(handler.lastEventId == 700);
    std::printf("    Nested aggregators three-level chain: PASS\n");
}

void test_raw_passthrough_order_and_new_match_contribution() {
    std::printf("  Test 44: Raw pass-through ordering and fresh match contribution...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;
    RecordingOutputHandler handler;
    aggregator.setOutputHandler(&handler);
    int ids[] = {30, 72};
    assert(aggregator.addAllMatchPattern(ids, 2, 5, 400));

    aggregator.on(Event(30, 7));
    assert(handler.eventCount == 1);
    assert(handler.eventIds[0] == 30);
    assert(handler.sourceIds[0] == 7);

    aggregator.on(Event(72, 8));
    assert(handler.eventCount == 3);
    assert(handler.eventIds[1] == 72);
    assert(handler.sourceIds[1] == 8);
    assert(handler.eventIds[2] == 400);
    assert(handler.sourceIds[2] == EventAggregator::DEVICE_SOURCE_ID);

    aggregator.on(Event(99, 9));
    assert(handler.eventCount == 4);
    assert(handler.eventIds[3] == 99);

    aggregator.on(Event(30, 10));
    assert(handler.eventCount == 6);
    assert(handler.eventIds[4] == 30);
    assert(handler.eventIds[5] == 400);
    std::printf("    Raw pass-through ordering and fresh match contribution: PASS\n");
}

void test_raw_passthrough_respects_source_filtered_contribution() {
    std::printf("  Test 45: Source-filtered matches require a new contributing event...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;
    RecordingOutputHandler handler;
    aggregator.setOutputHandler(&handler);
    int ids[] = {20, 72};
    int sources[] = {4, 9};
    assert(aggregator.addAllMatchPattern(ids, 2, sources, 5, 401));

    aggregator.on(Event(20, 3));
    aggregator.on(Event(72, 9));
    assert(handler.eventCount == 2);

    aggregator.on(Event(20, 4));
    assert(handler.eventCount == 4);
    assert(handler.eventIds[2] == 20);
    assert(handler.eventIds[3] == 401);

    aggregator.on(Event(20, 3));
    assert(handler.eventCount == 5);
    assert(handler.eventIds[4] == 20);
    std::printf("    Source-filtered match contribution: PASS\n");
}

void test_aggregator_rules_engine_action_count() {
    std::printf("  Test 46: Aggregator-to-rules action count...\n");
    testFakeMillis = 0;

    EventAggregator aggregator;
    RulesEngine rulesEngine;
    CountingCommandHandler commandHandler;
    assert(aggregator.addAllMatchPattern((int[]){30, 72}, 2, 5, 400));
    assert(rulesEngine.addRule(400, &commandHandler, 1));
    aggregator.setOutputHandler(&rulesEngine);

    aggregator.on(Event(30));
    aggregator.on(Event(72));
    assert(commandHandler.handleCallCount == 1);

    aggregator.on(Event(99));
    assert(commandHandler.handleCallCount == 1);

    aggregator.on(Event(30));
    assert(commandHandler.handleCallCount == 2);
    std::printf("    Aggregator-to-rules action count: PASS\n");
}

// --- Main ---

int main() {
    std::printf("Running EventAggregator tests...\n");

    test_single_event_pattern();
    test_two_event_and_pattern_within_window();
    test_two_event_and_pattern_outside_window();
    test_three_event_pattern();
    test_multiple_patterns_registered();
    test_captured_event_cleanup();
    test_circular_buffer_overflow();
    test_no_patterns_registered();
    test_pattern_array_full();
    test_invalid_pattern_parameters();
    test_no_output_handler();
    test_events_arrive_reverse_order();
    test_time_window_boundary_exact();
    test_time_window_boundary_just_over();
    test_duplicate_events_in_pattern();
    test_pattern_matches_multiple_times();
    test_same_event_contributes_to_multiple_patterns();
    test_source_aware_pattern_exact_match();
    test_source_aware_pattern_wrong_source();
    test_source_aware_pattern_wildcard();
    test_source_aware_pattern_mixed_sources();
    test_source_aware_two_patterns_different_sources();
    test_backward_compat_no_source_ids();
    test_condition_returns_true();
    test_condition_returns_false();
    test_condition_nullptr_always_emits();
    test_condition_sensor_value_check();
    test_condition_shared_function_multiple_patterns();
    test_remove_existing_pattern();
    test_remove_non_existent_pattern();
    test_remove_and_readd_pattern();
    test_remove_middle_pattern();
    test_cleanup_patterns();
    test_or_pattern_single_event_triggers();
    test_or_pattern_neither_event_fires();
    test_or_pattern_source_aware();
    test_or_pattern_time_window_expiry();
    test_mixed_and_or_patterns();
    test_or_pattern_array_full();
    test_or_pattern_remove_and_cleanup();
    test_nested_aggregators_basic_chain();
    test_nested_aggregators_source_filtering();
    test_nested_aggregators_three_level_chain();
    test_raw_passthrough_order_and_new_match_contribution();
    test_raw_passthrough_respects_source_filtered_contribution();
    test_aggregator_rules_engine_action_count();

    std::printf("All EventAggregator tests passed.\n");
    return 0;
}
