/**
 * @file SamplingScheduler.cpp
 * @brief Implements the platform-independent per-slot sampling scheduler.
 * @ingroup core_module
 * @date 2026-11-04
 * @version 1.0
 */

#include "SamplingScheduler.h"
#include "EventHandler.h"
#include <stdexcept>

SamplingScheduler::SamplingScheduler(uint32_t baseTickInMilliseconds)
    : baseTickMs(baseTickInMilliseconds), slotCount(0)
{
    if (baseTickMs == 0) {
        throw std::invalid_argument("Base tick interval must be greater than zero");
    }
    for (auto& slot : slots) {
        slot = {};
    }
}

uint32_t SamplingScheduler::ceilingDiv(uint32_t a, uint32_t b) {
    return (a + b - 1) / b;
}

SamplingScheduler::AddResult SamplingScheduler::add(EventHandler* handler, int eventIdentifier,
                                                     uint32_t periodInMilliseconds,
                                                     uint32_t minimumPeriodInMilliseconds) {
    // Validate arguments
    if (handler == nullptr || eventIdentifier == -1) {
        return AddResult::InvalidArgument;
    }
    if (slotCount >= MAXIMUM_SLOTS) {
        return AddResult::TableFull;
    }

    // Enforce minimum period if specified
    AddResult clampingResult = AddResult::Ok;
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
        periodInTicks = ceilingDiv(periodInMilliseconds, baseTickMs);
        if (periodInTicks < 1) {
            periodInTicks = 1;
        }
        // Check if the period was clamped to a single tick (below tick granularity)
        if (periodInTicks == 1 && periodInMilliseconds < baseTickMs && clampingResult == AddResult::Ok) {
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
    uint32_t ticksUntilDue = 1 + (samePeriodOffset % periodInTicks);

    slots[slotCount] = {
        handler,
        eventIdentifier,
        periodInTicks,
        ticksUntilDue
    };
    slotCount++;
    return clampingResult;
}

uint8_t SamplingScheduler::advance(uint32_t elapsedTicks) {
    if (elapsedTicks == 0 || slotCount == 0) {
        return 0;
    }

    uint8_t dispatched = 0;

    for (uint8_t i = 0; i < slotCount; i++) {
        Slot& slot = slots[i];

        if (elapsedTicks >= slot.ticksUntilDue) {
            // Slot is due: dispatch once (no replay of missed samples)
            if (slot.handler != nullptr) {
                slot.handler->on(Event(slot.eventIdentifier));
                dispatched++;
            }

            // Re-arm with phase alignment
            // overshoot = elapsedTicks - ticksUntilDue
            // new ticksUntilDue = periodInTicks - (overshoot % periodInTicks)
            uint32_t overshoot = elapsedTicks - slot.ticksUntilDue;
            slot.ticksUntilDue = slot.periodInTicks - (overshoot % slot.periodInTicks);

            // Ensure ticksUntilDue is always >= 1
            if (slot.ticksUntilDue == 0) {
                slot.ticksUntilDue = slot.periodInTicks;
            }
        } else {
            // Not yet due: decrement countdown
            slot.ticksUntilDue -= elapsedTicks;
        }
    }

    return dispatched;
}

uint8_t SamplingScheduler::count() const {
    return slotCount;
}

const SamplingScheduler::Slot& SamplingScheduler::slotAt(uint8_t index) const {
    return slots[index];
}

uint32_t SamplingScheduler::baseTickInMilliseconds() const {
    return baseTickMs;
}

uint32_t SamplingScheduler::effectivePeriodInMilliseconds(uint8_t index) const {
    return slots[index].periodInTicks * baseTickMs;
}
