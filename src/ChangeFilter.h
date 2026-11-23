#ifndef CHANGE_FILTER_H
#define CHANGE_FILTER_H

/**
 * @file ChangeFilter.h
 * @brief Deadband-based telemetry suppression for change-only mode.
 * @ingroup core_module
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-11-22
 * @version 1.0
 *
 * Suppresses telemetry when sensor values are stable within a configurable
 * deadband threshold. Each slot tracks its own last enqueued value and
 * deadband, enabling per-sensor tuning (e.g., 0.5°C for temperature, 0 for
 * motion sensors).
 *
 * All state is stored in fixed-size arrays — zero heap allocation.
 */

#include <cstdint>
#include <cstddef>
#include <cmath>
#include <cstring>

/**
 * @brief Maximum number of change filter slots.
 * @note Matches SamplingScheduler::MAXIMUM_SLOTS for 1:1 slot mapping.
 */
static constexpr uint8_t CHANGE_FILTER_MAXIMUM_SLOTS = 8;

/**
 * @brief Deadband-based telemetry suppression for a single sensor value type.
 * @tparam T Sensor value type (float for temperature/humidity, int for ADC/pulse counts).
 * @ingroup core_module
 *
 * Template class that tracks the last enqueued value per slot and suppresses
 * telemetry when the new value hasn't changed beyond the configured deadband.
 *
 * Key behaviors:
 * - First reading always passes (no last value to compare).
 * - Deadband of 0.0 (or 0 for int) disables filtering — all values pass.
 * - NaN and Inf float values are always suppressed to avoid undefined behavior.
 * - Out-of-bounds slot indices return false without crashing.
 *
 * @par Zero Heap Allocation
 * Uses a fixed-size array of 8 slots. No dynamic allocation.
 *
 * @par Usage Example
 * @code
 * ChangeFilter<float> filter;
 * filter.configureSlotWithDeadband(0, 0.5f);  // 0.5°C deadband for slot 0
 *
 * if (filter.isSignificantChange(0, 22.3f)) {
 *     // Change exceeds 0.5°C — enqueue telemetry
 * }
 * @endcode
 */
template<typename T>
class ChangeFilter {
public:
    /// One tracked slot: last value, deadband, and state flag.
    struct Slot {
        T lastValue; ///< Last enqueued value for this slot.
        T deadband; ///< Deadband threshold (0 disables filtering).
        bool hasLastValue; ///< True after the first reading passes.
    };

    /**
     * @brief Constructs a ChangeFilter with all slots reset.
     */
    ChangeFilter() {
        reset();
    }

    /**
     * @brief Configures the deadband for a slot.
     * @param slotIndex Slot index (0 to CHANGE_FILTER_MAXIMUM_SLOTS - 1).
     * @param deadband Deadband threshold; 0 disables filtering for this slot.
     *
     * A deadband of 0 means all values pass (filtering disabled).
     * For float types, a deadband of 0.5f means the value must change by
     * more than 0.5 from the last enqueued value to pass.
     *
     * Out-of-bounds slot indices are silently ignored.
     */
    void configureSlotWithDeadband(uint8_t slotIndex, T deadband) {
        if (slotIndex >= CHANGE_FILTER_MAXIMUM_SLOTS) return;
        slots[slotIndex].deadband = deadband;
    }

    /**
     * @brief Checks if a new value is significant enough to enqueue.
     * @param slotIndex Slot index (0 to CHANGE_FILTER_MAXIMUM_SLOTS - 1).
     * @param newValue The new sensor reading.
     * @return True if the value should be enqueued, false if suppressed.
     *
     * Returns true when:
     * - This is the first reading for this slot (no last value).
     * - Deadband is 0 (filtering disabled for this slot).
     * - Absolute change exceeds the deadband.
     *
     * Returns false when:
     * - The change is within the deadband (inclusive).
     * - The value is NaN or Inf (float types only).
     * - The slot index is out of bounds.
     *
     * On return, if the value passes, the slot's lastValue is updated.
     */
    bool isSignificantChange(uint8_t slotIndex, T newValue) {
        if (slotIndex >= CHANGE_FILTER_MAXIMUM_SLOTS) return false;

        // Suppress NaN and Inf for float types to avoid undefined behavior
        if (!isValidValue(newValue)) return false;

        Slot& slot = slots[slotIndex];

        // Deadband of 0 disables filtering — all values pass
        if (slot.deadband == static_cast<T>(0)) {
            slot.lastValue = newValue;
            slot.hasLastValue = true;
            return true;
        }

        // First reading always passes
        if (!slot.hasLastValue) {
            slot.lastValue = newValue;
            slot.hasLastValue = true;
            return true;
        }

        // Check if change exceeds deadband (strictly greater than)
        T change = absoluteDifference(newValue, slot.lastValue);
        if (change > slot.deadband) {
            slot.lastValue = newValue;
            return true;
        }

        // Change is within deadband — suppress
        return false;
    }

    /**
     * @brief Resets all slots to their initial state.
     *
     * Clears all last values and deadbands. Useful on device reboot
     * or when reconfiguring the filter from scratch.
     */
    void reset() {
        std::memset(slots, 0, sizeof(slots));
    }

    /**
     * @brief Returns the number of slots.
     * @return CHANGE_FILTER_MAXIMUM_SLOTS.
     */
    static uint8_t maximumSlots() {
        return CHANGE_FILTER_MAXIMUM_SLOTS;
    }

private:
    Slot slots[CHANGE_FILTER_MAXIMUM_SLOTS]; ///< Fixed-size slot array.

    /**
     * @brief Checks if a value is valid (not NaN or Inf).
     * @param value The value to check.
     * @return True if the value is a normal number.
     *
     * For float types, returns false for NaN and Inf.
     * For int types, always returns true.
     */
    bool isValidValue(T /*value*/) {
        // Default implementation for non-float types: always valid
        return true;
    }

    /**
     * @brief Computes the absolute difference between two values.
     * @param a First value.
     * @param b Second value.
     * @return |a - b|
     *
     * Default implementation for non-float types.
     */
    T absoluteDifference(T a, T b) {
        T diff = a - b;
        return diff >= static_cast<T>(0) ? diff : -diff;
    }
};

// Explicit specialization for float: isValidValue
template<>
inline bool ChangeFilter<float>::isValidValue(float value) {
    return std::isfinite(value);
}

// Explicit specialization for double: isValidValue
template<>
inline bool ChangeFilter<double>::isValidValue(double value) {
    return std::isfinite(value);
}

#endif // CHANGE_FILTER_H
