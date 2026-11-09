/**
 * @file test.cpp
 * @brief Host unit tests for SamplingScheduler.
 *
 * Tests cover: single slot dispatch, multi-slot counting, period rounding,
 * clamping, staggered phases, elapsed tick handling, overflow safety,
 * dispatch order, and edge cases.
 */

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <vector>

#include "SamplingScheduler.h"
#include "Event.h"

// ---------------------------------------------------------------------------
// Fake EventHandler that records dispatched event identifiers
// ---------------------------------------------------------------------------
namespace {

class FakeHandler : public EventHandler {
public:
    std::vector<int> dispatchedEvents;

    void on(Event event) override {
        dispatchedEvents.push_back(event.identifier);
    }
};

} // anonymous namespace

// ---------------------------------------------------------------------------
// Test helpers
// ---------------------------------------------------------------------------
namespace {

void test_single_slot_every_tick() {
    printf("  [test] single slot, period = tick ...\n");
    SamplingScheduler scheduler(100); // 100 ms tick

    FakeHandler handler;
    auto result = scheduler.add(&handler, 20, 100); // period = 1 tick
    assert(result == SamplingScheduler::AddResult::Ok);
    assert(scheduler.count() == 1);

    handler.dispatchedEvents.clear();
    assert(scheduler.advance(1) == 1);
    assert(handler.dispatchedEvents.size() == 1);
    assert(handler.dispatchedEvents[0] == 20);

    handler.dispatchedEvents.clear();
    assert(scheduler.advance(1) == 1);
    assert(handler.dispatchedEvents.size() == 1);
}

void test_two_slots_different_periods() {
    printf("  [test] two slots, periods 1 and 5 ticks ...\n");
    SamplingScheduler scheduler(100); // 100 ms tick

    FakeHandler handler1;
    FakeHandler handler2;

    scheduler.add(&handler1, 20, 100);  // 1 tick
    scheduler.add(&handler2, 30, 500);  // 5 ticks

    uint32_t totalTicks = 0;
    for (int i = 0; i < 100; i++) {
        totalTicks += scheduler.advance(1);
    }

    // handler1: 100 dispatches (every tick)
    // handler2: 20 dispatches (every 5 ticks)
    assert(totalTicks == 120);
    assert(handler1.dispatchedEvents.size() == 100);
    assert(handler2.dispatchedEvents.size() == 20);
}

void test_period_rounding() {
    printf("  [test] period rounding: 150 ms on 100 ms tick = 2 ticks ...\n");
    SamplingScheduler scheduler(100);

    FakeHandler handler;
    auto result = scheduler.add(&handler, 20, 150); // 150 ms -> ceil(150/100) = 2 ticks
    assert(result == SamplingScheduler::AddResult::Ok);

    // Effective period should be 200 ms (2 ticks * 100 ms)
    assert(scheduler.effectivePeriodInMilliseconds(0) == 200);

    handler.dispatchedEvents.clear();
    scheduler.advance(1); // Not due (ticksUntilDue starts at 1, after advance(1) it becomes 0, re-armed to 2)
    // Wait - initial ticksUntilDue = 1 + (0 % 2) = 1. advance(1) fires, re-arms.
    assert(handler.dispatchedEvents.size() == 1);

    handler.dispatchedEvents.clear();
    scheduler.advance(1); // ticksUntilDue was 2, now 1. Not due.
    assert(handler.dispatchedEvents.size() == 0);

    scheduler.advance(1); // ticksUntilDue was 1, now 0. Due.
    assert(handler.dispatchedEvents.size() == 1);
}

void test_period_below_tick_clamped() {
    printf("  [test] period below tick clamped to 1 tick ...\n");
    SamplingScheduler scheduler(100);

    FakeHandler handler;
    auto result = scheduler.add(&handler, 20, 50); // 50 ms < 100 ms tick
    assert(result == SamplingScheduler::AddResult::ClampedToTick);
    assert(scheduler.slotAt(0).periodInTicks == 1);
}

void test_below_minimum_clamped() {
    printf("  [test] below-minimum request clamped and reported ...\n");
    SamplingScheduler scheduler(100);

    FakeHandler handler;
    auto result = scheduler.add(&handler, 20, 50, 200); // 50 ms requested, 200 ms minimum
    assert(result == SamplingScheduler::AddResult::ClampedToMinimum);
    // 200 ms / 100 ms tick = 2 ticks
    assert(scheduler.slotAt(0).periodInTicks == 2);
}

void test_staggered_initial_phase() {
    printf("  [test] two period-5 slots staggered: fire on ticks 1 and 2 ...\n");
    SamplingScheduler scheduler(100);

    FakeHandler handler1;
    FakeHandler handler2;

    scheduler.add(&handler1, 20, 500); // 5 ticks, offset 0 -> ticksUntilDue = 1
    scheduler.add(&handler2, 30, 500); // 5 ticks, offset 1 -> ticksUntilDue = 2

    // Tick 1: handler1 fires
    assert(scheduler.advance(1) == 1);
    assert(handler1.dispatchedEvents.size() == 1);
    assert(handler2.dispatchedEvents.size() == 0);

    // Tick 2: handler2 fires
    assert(scheduler.advance(1) == 1);
    assert(handler1.dispatchedEvents.size() == 1);
    assert(handler2.dispatchedEvents.size() == 1);

    // Ticks 3-5: nothing fires
    assert(scheduler.advance(1) == 0);
    assert(scheduler.advance(1) == 0);
    assert(scheduler.advance(1) == 0);

    // Tick 6: handler1 fires again
    assert(scheduler.advance(1) == 1);
    assert(handler1.dispatchedEvents.size() == 2);
    assert(handler2.dispatchedEvents.size() == 1);

    // Tick 7: handler2 fires again
    assert(scheduler.advance(1) == 1);
    assert(handler1.dispatchedEvents.size() == 2);
    assert(handler2.dispatchedEvents.size() == 2);
}

void test_elapsed_tick_no_drift() {
    printf("  [test] advance(7) after advance(1): phase preserved, no drift ...\n");
    SamplingScheduler scheduler(100);

    FakeHandler handler;
    scheduler.add(&handler, 20, 500); // 5 ticks, offset 0 -> ticksUntilDue = 1

    // Tick 1: fires
    assert(scheduler.advance(1) == 1);
    assert(handler.dispatchedEvents.size() == 1);

    // advance(7): fires once at tick 8, next fire at tick 11
    // overshoot = 7 - 5 = 2, ticksUntilDue = 5 - (2 % 5) = 3
    assert(scheduler.advance(7) == 1);
    assert(handler.dispatchedEvents.size() == 2);
    assert(scheduler.slotAt(0).ticksUntilDue == 3);

    // advance(3): fires at tick 11
    assert(scheduler.advance(3) == 1);
    assert(handler.dispatchedEvents.size() == 3);
}

void test_advance_zero_no_op() {
    printf("  [test] advance(0) is a no-op ...\n");
    SamplingScheduler scheduler(100);

    FakeHandler handler;
    scheduler.add(&handler, 20, 100);

    assert(scheduler.advance(0) == 0);
    assert(handler.dispatchedEvents.size() == 0);
}

void test_table_full_and_invalid_args() {
    printf("  [test] table full and invalid arguments ...\n");
    SamplingScheduler scheduler(100);

    FakeHandler handler;
    for (int i = 0; i < 8; i++) {
        auto result = scheduler.add(&handler, 20, 100);
        assert(result == SamplingScheduler::AddResult::Ok);
    }
    assert(scheduler.count() == 8);

    // Table full
    auto result = scheduler.add(&handler, 20, 100);
    assert(result == SamplingScheduler::AddResult::TableFull);

    // Null handler
    result = SamplingScheduler(100).add(nullptr, 20, 100);
    assert(result == SamplingScheduler::AddResult::InvalidArgument);

    // Invalid event ID
    result = SamplingScheduler(100).add(&handler, -1, 100);
    assert(result == SamplingScheduler::AddResult::InvalidArgument);
}

void test_overflow_safety() {
    printf("  [test] large elapsed ticks don't overflow ...\n");
    SamplingScheduler scheduler(100);

    FakeHandler handler;
    scheduler.add(&handler, 20, 500); // 5 ticks

    // Simulate a huge elapsed tick count
    assert(scheduler.advance(UINT32_MAX) == 1);
    assert(handler.dispatchedEvents.size() == 1);
    // ticksUntilDue should still be >= 1
    assert(scheduler.slotAt(0).ticksUntilDue >= 1);
}

void test_dispatch_order() {
    printf("  [test] dispatch order equals registration order ...\n");
    SamplingScheduler scheduler(100);

    FakeHandler handler1;
    FakeHandler handler2;
    FakeHandler handler3;

    scheduler.add(&handler1, 20, 100); // 1 tick
    scheduler.add(&handler2, 30, 100); // 1 tick
    scheduler.add(&handler3, 40, 100); // 1 tick

    scheduler.advance(1);

    // All three should fire, in registration order
    assert(handler1.dispatchedEvents.size() == 1);
    assert(handler2.dispatchedEvents.size() == 1);
    assert(handler3.dispatchedEvents.size() == 1);
    assert(handler1.dispatchedEvents[0] == 20);
    assert(handler2.dispatchedEvents[0] == 30);
    assert(handler3.dispatchedEvents[0] == 40);
}

void test_period_zero_every_tick() {
    printf("  [test] period 0 means every tick ...\n");
    SamplingScheduler scheduler(100);

    FakeHandler handler;
    auto result = scheduler.add(&handler, 20, 0);
    assert(result == SamplingScheduler::AddResult::ClampedToTick);
    assert(scheduler.slotAt(0).periodInTicks == 1);

    handler.dispatchedEvents.clear();
    assert(scheduler.advance(1) == 1);
    assert(handler.dispatchedEvents.size() == 1);
}

void test_same_handler_twice() {
    printf("  [test] same handler registered twice dispatches twice ...\n");
    SamplingScheduler scheduler(100);

    FakeHandler handler;
    scheduler.add(&handler, 20, 100);
    scheduler.add(&handler, 30, 100);

    scheduler.advance(1);
    assert(handler.dispatchedEvents.size() == 2);
    assert(handler.dispatchedEvents[0] == 20);
    assert(handler.dispatchedEvents[1] == 30);
}

void test_zero_slots_advance() {
    printf("  [test] advance with zero slots is a no-op ...\n");
    SamplingScheduler scheduler(100);
    assert(scheduler.advance(1) == 0);
    assert(scheduler.advance(100) == 0);
}

void test_base_tick_zero_rejected() {
    printf("  [test] base tick zero throws ...\n");
    bool thrown = false;
    try {
        SamplingScheduler scheduler(0);
    } catch (const std::invalid_argument&) {
        thrown = true;
    }
    assert(thrown);
}

void test_ticks_until_next_due_empty() {
    printf("  [test] ticksUntilNextDue with no slots returns 0 ...\n");
    SamplingScheduler scheduler(100);
    assert(scheduler.ticksUntilNextDue() == 0);
}

void test_ticks_until_next_due_single_slot() {
    printf("  [test] ticksUntilNextDue with single slot returns its countdown ...\n");
    SamplingScheduler scheduler(100);
    FakeHandler handler;
    scheduler.add(&handler, 20, 500); // 5 ticks, initial ticksUntilDue = 1
    assert(scheduler.ticksUntilNextDue() == 1);

    // After advance(1), slot fires and re-arms to periodInTicks = 5
    scheduler.advance(1);
    assert(scheduler.ticksUntilNextDue() == 5);

    // After advance(2), countdown decrements to 3
    scheduler.advance(2);
    assert(scheduler.ticksUntilNextDue() == 3);
}

void test_ticks_until_next_due_multiple_slots() {
    printf("  [test] ticksUntilNextDue returns minimum across slots ...\n");
    SamplingScheduler scheduler(100);
    FakeHandler handler1;
    FakeHandler handler2;
    FakeHandler handler3;

    scheduler.add(&handler1, 20, 100);  // 1 tick, ticksUntilDue = 1
    scheduler.add(&handler2, 30, 500);  // 5 ticks, ticksUntilDue = 1
    scheduler.add(&handler3, 40, 1000); // 10 ticks, ticksUntilDue = 1

    // All start at 1 due to staggered phase
    assert(scheduler.ticksUntilNextDue() == 1);

    // After advance(1), all fire and re-arm
    scheduler.advance(1);
    // handler1 re-arms to 1, handler2 re-arms to 5, handler3 re-arms to 10
    assert(scheduler.ticksUntilNextDue() == 1);

    // After advance(1), handler1 fires again
    scheduler.advance(1);
    // handler1 re-arms to 1, handler2 countdown to 4, handler3 countdown to 9
    assert(scheduler.ticksUntilNextDue() == 1);
}

void test_ticks_until_next_due_after_advance() {
    printf("  [test] ticksUntilNextDue reflects re-armed slots after advance ...\n");
    SamplingScheduler scheduler(100);
    FakeHandler handler;
    scheduler.add(&handler, 20, 500); // 5 ticks, ticksUntilDue = 1

    // advance(1): fires, re-arms to 5
    scheduler.advance(1);
    assert(scheduler.ticksUntilNextDue() == 5);

    // advance(4): countdown to 1, not due
    scheduler.advance(4);
    assert(scheduler.ticksUntilNextDue() == 1);

    // advance(1): fires, re-arms to 5
    scheduler.advance(1);
    assert(scheduler.ticksUntilNextDue() == 5);
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
[[nodiscard]] int main() {
    printf("[suite] SamplingScheduler\n");

    test_single_slot_every_tick();
    test_two_slots_different_periods();
    test_period_rounding();
    test_period_below_tick_clamped();
    test_below_minimum_clamped();
    test_staggered_initial_phase();
    test_elapsed_tick_no_drift();
    test_advance_zero_no_op();
    test_table_full_and_invalid_args();
    test_overflow_safety();
    test_dispatch_order();
    test_period_zero_every_tick();
    test_same_handler_twice();
    test_zero_slots_advance();
    test_base_tick_zero_rejected();
    test_ticks_until_next_due_empty();
    test_ticks_until_next_due_single_slot();
    test_ticks_until_next_due_multiple_slots();
    test_ticks_until_next_due_after_advance();

    printf("[suite] SamplingScheduler: ALL PASSED\n\n");
    return 0;
}
