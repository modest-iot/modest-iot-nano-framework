/**
 * @file SamplingScheduler.cpp
 * @brief Implements the platform-independent per-slot sampling scheduler.
 * @ingroup core_module
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-11-04
 * @version 1.0
 */

#include "SamplingScheduler.h"
#include "EventHandler.h"
#include <stdexcept>

SamplingScheduler::SamplingScheduler(const uint32_t baseTickInMilliseconds)
    : baseTickIntervalInMilliseconds(baseTickInMilliseconds), slotCount(0)
{
    if (baseTickIntervalInMilliseconds == 0) {
        throw std::invalid_argument("Base tick interval must be greater than zero");
    }
    for (auto& slot : slots) {
        slot = {};
    }
}

uint32_t SamplingScheduler::calculateCeilingDivision(const uint32_t numerator, const uint32_t denominator) {
    return (numerator + denominator - 1) / denominator;
}

SamplingScheduler::AddResult SamplingScheduler::add(EventHandler* handler, const int eventIdentifier,
                                                     uint32_t periodInMilliseconds,
                                                     const uint32_t minimumPeriodInMilliseconds) {
    // Validate arguments
    if (handler == nullptr || eventIdentifier == -1) {
        return AddResult::InvalidArgument;
    }
    if (slotCount >= MAXIMUM_SLOTS) {
        return AddResult::TableFull;
    }

    // Enforce minimum period if specified
    auto clampingResult = AddResult::Ok;
    if (minimumPeriodInMilliseconds > 0 && periodInMilliseconds < minimumPeriodInMilliseconds) {
        periodInMilliseconds = minimumPeriodInMilliseconds;
        clampingResult = AddResult::ClampedToMinimum;
    }

    // Convert milliseconds to ticks using ceiling division
    // Period 0 means "every tick"
    uint32_t periodInTicks;
    if (periodInMilliseconds == 0) {
        periodInTicks = 1;
        if (clampingResult == AddResult::Ok) {
            clampingResult = AddResult::ClampedToTick;
        }
    } else {
        periodInTicks = calculateCeilingDivision(periodInMilliseconds, baseTickIntervalInMilliseconds);
        if (periodInTicks < 1) {
            periodInTicks = 1;
        }
        // Check if the period was clamped to a single tick (below tick granularity)
        if (periodInTicks == 1 && periodInMilliseconds < baseTickIntervalInMilliseconds && clampingResult == AddResult::Ok) {
            clampingResult = AddResult::ClampedToTick;
        }
    }

    // Compute staggered initial phase
    // Count how many existing slots have the same period
    uint8_t samePeriodOffset = 0;
    for (uint8_t i = 0; i < slotCount; i++) {
        if (slots[i].periodInTicks == periodInTicks) {
            samePeriodOffset++;
        }
    }

    // ticksUntilDue = 1 + (offset % periodInTicks)
    // This ensures equal-period slots first fire on consecutive ticks
    const uint32_t ticksUntilDue = 1 + samePeriodOffset % periodInTicks;

    slots[slotCount] = {
        handler,
        eventIdentifier,
        periodInTicks,
        ticksUntilDue
    };
    slotCount++;
    return clampingResult;
}

uint8_t SamplingScheduler::advance(const uint32_t elapsedTicks) {
    if (elapsedTicks == 0 || slotCount == 0) {
        return 0;
    }

    uint8_t dispatched = 0;

    for (uint8_t i = 0; i < slotCount; i++) {
        if (auto&[handler, eventIdentifier, periodInTicks, ticksUntilDue] = slots[i]; elapsedTicks >= ticksUntilDue) {
            // Slot is due: dispatch once (no replay of missed samples)
            if (handler != nullptr) {
                handler->on(Event(eventIdentifier));
                dispatched++;
            }

            // Re-arm with phase alignment
            // overshoot = elapsedTicks - ticksUntilDue
            // new ticksUntilDue = periodInTicks - (overshoot % periodInTicks)
            const uint32_t overshoot = elapsedTicks - ticksUntilDue;
            ticksUntilDue = periodInTicks - overshoot % periodInTicks;

            // Ensure ticksUntilDue is always >= 1
            if (ticksUntilDue == 0) {
                ticksUntilDue = periodInTicks;
            }
        } else {
            // Not yet due: decrement countdown
            ticksUntilDue -= elapsedTicks;
        }
    }

    return dispatched;
}

uint8_t SamplingScheduler::count() const {
    return slotCount;
}

const SamplingScheduler::Slot& SamplingScheduler::slotAt(const uint8_t index) const {
    return slots[index];
}

uint32_t SamplingScheduler::baseTickInMilliseconds() const {
    return baseTickIntervalInMilliseconds;
}

uint32_t SamplingScheduler::effectivePeriodInMilliseconds(const uint8_t index) const {
    return slots[index].periodInTicks * baseTickIntervalInMilliseconds;
}

uint32_t SamplingScheduler::ticksUntilNextDue() const {
    if (slotCount == 0) {
        return 0;
    }
    uint32_t minTicks = slots[0].ticksUntilDue;
    for (uint8_t i = 1; i < slotCount; i++) {
        if (slots[i].ticksUntilDue < minTicks) {
            minTicks = slots[i].ticksUntilDue;
        }
    }
    return minTicks;
}
