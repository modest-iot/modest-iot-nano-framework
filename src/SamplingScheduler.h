#ifndef SAMPLING_SCHEDULER_H
#define SAMPLING_SCHEDULER_H

/**
 * @file SamplingScheduler.h
 * @brief Declares a platform-independent per-slot sampling scheduler.
 * @ingroup core_module
 * @date 2026-11-04
 * @version 1.0
 *
 * `SamplingScheduler` replaces the fixed-tick round-robin dispatch with a countdown-based
 * scheduler where each slot declares its own period in milliseconds. It compiles on the
 * host (no Arduino or FreeRTOS includes) for unit testing.
 *
 * Key features:
 * - Per-slot periods with integer ceiling rounding to whole ticks
 * - Staggered initial phases so equal-period slots fire on consecutive ticks
 * - Elapsed tick accumulation: `advance(n)` handles missed ticks without drift
 * - Phase-aligned re-arming: cadence stays locked to the original phase
 * - Minimum period enforcement with clamping reports
 */

#include <cstdint>
#include <cstddef>
#include "EventHandler.h"

/**
 * @brief Platform-independent per-slot sampling scheduler.
 * @ingroup core_module
 *
 * Each slot declares its own period in milliseconds. The scheduler converts periods
 * to tick counts using integer ceiling division, so a 150 ms period on a 100 ms tick
 * becomes 2 ticks.
 *
 * Countdown semantics: `ticksUntilDue` is always >= 1. A slot fires when its countdown
 * reaches zero. After firing, the slot is re-armed with phase alignment so the cadence
 * never drifts, even when `advance(n)` spans multiple periods.
 *
 * Initial phase: `ticksUntilDue = 1 + (offset % periodInTicks)`, where `offset` is the
 * number of slots already registered with the same period. Equal-period slots therefore
 * first fire on consecutive ticks (1, 2, 3, ...), then each repeats every `periodInTicks`.
 *
 * Dispatch order: slot registration order.
 * Reentrancy: handlers must not call `add()` during dispatch.
 */
class SamplingScheduler {
public:
    /// Maximum number of schedulable slots.
    static constexpr uint8_t MAXIMUM_SLOTS = 8;

    /**
     * @brief Result of adding a slot to the scheduler.
     */
    enum class AddResult : uint8_t {
        Ok, ///< Slot added successfully with the requested period.
        ClampedToMinimum, ///< Period was below the minimum; clamped up to the minimum.
        ClampedToTick, ///< Period was below one tick; clamped to 1 tick.
        TableFull, ///< No more slots available.
        InvalidArgument ///< Null handler, invalid event ID, or zero base tick.
    };

    /**
     * @brief One schedulable slot mapping a handler to an event and period.
     */
    struct Slot {
        EventHandler* handler = nullptr; ///< Sensor or any EventHandler to invoke.
        int eventIdentifier = -1; ///< Event identifier to pass to the handler.
        uint32_t periodInTicks = 1; ///< Period in ticks (>= 1).
        uint32_t ticksUntilDue = 1; ///< Countdown to next dispatch (>= 1).
    };

    /**
     * @brief Constructs a scheduler with the given base tick interval.
     * @param baseTickInMilliseconds Hardware timer tick period in milliseconds.
     * @throw std::invalid_argument if `baseTickInMilliseconds` is zero.
     */
    explicit SamplingScheduler(uint32_t baseTickInMilliseconds);

    /**
     * @brief Adds a handler to the scheduler with an explicit period.
     * @param handler The EventHandler to invoke when the slot is due.
     * @param eventIdentifier Event identifier to pass to the handler.
     * @param periodInMilliseconds Desired period in milliseconds.
     * @param minimumPeriodInMilliseconds Optional minimum period; if the requested period
     *        is below this value, it is clamped up and `ClampedToMinimum` is returned.
     * @return Result indicating success, clamping, or rejection.
     *
     * Period 0 means "every tick" (clamped to 1 tick).
     * Periods are rounded up to whole ticks using integer ceiling division.
     * Equal-period slots are staggered so they first fire on consecutive ticks.
     */
    AddResult add(EventHandler* handler, int eventIdentifier, uint32_t periodInMilliseconds,
                  uint32_t minimumPeriodInMilliseconds = 0);

    /**
     * @brief Advances time by `elapsedTicks` and dispatches every due slot.
     * @param elapsedTicks Number of ticks since the last advance call (>= 1).
     * @return Number of slots dispatched.
     *
     * If `elapsedTicks` is 0, this is a no-op.
     * Each due slot is dispatched exactly once (no replay of missed samples).
     * Slots are dispatched in registration order.
     * After dispatch, slots are re-armed with phase alignment (no drift).
     */
    uint8_t advance(uint32_t elapsedTicks);

    /**
     * @brief Returns the number of registered slots.
     */
    uint8_t count() const;

    /**
     * @brief Returns a reference to the slot at the given index.
     * @param index Slot index (0-based).
     * @return Reference to the slot.
     * @pre index < count()
     */
    const Slot& slotAt(uint8_t index) const;

    /**
     * @brief Returns the base tick interval in milliseconds.
     */
    uint32_t baseTickInMilliseconds() const;

    /**
     * @brief Returns the effective period in milliseconds for a slot.
     * @param index Slot index (0-based).
     * @return Effective period in milliseconds (periodInTicks * baseTickInMilliseconds).
     * @pre index < count()
     */
    uint32_t effectivePeriodInMilliseconds(uint8_t index) const;

private:
    uint32_t baseTickMs; ///< Hardware timer tick period in milliseconds.
    Slot slots[MAXIMUM_SLOTS]; ///< Slot table.
    uint8_t slotCount; ///< Number of registered slots.

    /**
     * @brief Computes the ceiling of a / b using integer arithmetic.
     * @param a Numerator.
     * @param b Denominator (must be > 0).
     * @return ceil(a / b)
     */
    static uint32_t ceilingDiv(uint32_t a, uint32_t b);
};

#endif // SAMPLING_SCHEDULER_H
