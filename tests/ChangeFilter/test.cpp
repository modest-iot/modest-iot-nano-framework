/**
 * @file test.cpp
 * @brief Host tests for ChangeFilter deadband-based telemetry suppression.
 * @date 2026-11-22
 * @version 1.0
 */

#include <cstdio>
#include <cassert>
#include <cmath>

#include "ChangeFilter.h"

// --- Test counters ---
static int testsPassed = 0;
static int testsFailed = 0;

#define ASSERT_TRUE(expr) do { \
    if (!(expr)) { \
        std::printf("    FAIL: %s (line %d)\n", #expr, __LINE__); \
        testsFailed++; \
        return; \
    } \
} while(0)

#define ASSERT_FALSE(expr) ASSERT_TRUE(!(expr))
#define ASSERT_EQ(a, b) ASSERT_TRUE((a) == (b))

// ============================================================
// Float ChangeFilter Tests
// ============================================================

void test_first_reading_always_passes() {
    std::printf("  Test 1: First reading always passes...\n");
    ChangeFilter<float> filter;
    filter.configureSlotWithDeadband(0, 0.5f);

    bool result = filter.isSignificantChange(0, 22.0f);
    ASSERT_TRUE(result);
    testsPassed++;
    std::printf("    PASS\n");
}

void test_value_within_deadband_suppressed() {
    std::printf("  Test 2: Value within deadband is suppressed...\n");
    ChangeFilter<float> filter;
    filter.configureSlotWithDeadband(0, 0.5f);

    filter.isSignificantChange(0, 22.0f);  // First reading passes
    bool result = filter.isSignificantChange(0, 22.3f);  // Change is 0.3, within 0.5
    ASSERT_FALSE(result);
    testsPassed++;
    std::printf("    PASS\n");
}

void test_value_exceeding_deadband_passes() {
    std::printf("  Test 3: Value exceeding deadband passes...\n");
    ChangeFilter<float> filter;
    filter.configureSlotWithDeadband(0, 0.5f);

    filter.isSignificantChange(0, 22.0f);  // First reading passes
    bool result = filter.isSignificantChange(0, 22.6f);  // Change is 0.6, exceeds 0.5
    ASSERT_TRUE(result);
    testsPassed++;
    std::printf("    PASS\n");
}

void test_zero_deadband_disables_filtering() {
    std::printf("  Test 4: Zero deadband disables filtering...\n");
    ChangeFilter<float> filter;
    filter.configureSlotWithDeadband(0, 0.0f);

    bool result1 = filter.isSignificantChange(0, 22.0f);
    bool result2 = filter.isSignificantChange(0, 22.0f);  // Same value, should pass
    bool result3 = filter.isSignificantChange(0, 22.001f);  // Tiny change, should pass
    ASSERT_TRUE(result1);
    ASSERT_TRUE(result2);
    ASSERT_TRUE(result3);
    testsPassed++;
    std::printf("    PASS\n");
}

void test_exact_deadband_boundary_suppressed() {
    std::printf("  Test 5: Exact deadband boundary is suppressed...\n");
    ChangeFilter<float> filter;
    filter.configureSlotWithDeadband(0, 0.5f);

    filter.isSignificantChange(0, 22.0f);  // First reading passes
    bool result = filter.isSignificantChange(0, 22.5f);  // Change is exactly 0.5
    ASSERT_FALSE(result);  // Deadband is inclusive — exactly 0.5 should suppress
    testsPassed++;
    std::printf("    PASS\n");
}

void test_multiple_slots_independent() {
    std::printf("  Test 6: Multiple slots are independent...\n");
    ChangeFilter<float> filter;
    filter.configureSlotWithDeadband(0, 0.5f);
    filter.configureSlotWithDeadband(1, 1.0f);

    // Slot 0: first reading
    filter.isSignificantChange(0, 22.0f);
    // Slot 1: first reading
    filter.isSignificantChange(1, 50.0f);

    // Slot 0: within deadband
    bool slot0Result = filter.isSignificantChange(0, 22.3f);
    // Slot 1: within deadband
    bool slot1Result = filter.isSignificantChange(1, 50.5f);

    ASSERT_FALSE(slot0Result);
    ASSERT_FALSE(slot1Result);

    // Slot 0: exceeds deadband
    slot0Result = filter.isSignificantChange(0, 22.6f);
    ASSERT_TRUE(slot0Result);

    // Slot 1: still within deadband (last value is 50.0, change is 0.6)
    slot1Result = filter.isSignificantChange(1, 50.6f);
    ASSERT_FALSE(slot1Result);

    testsPassed++;
    std::printf("    PASS\n");
}

void test_float_precision_small_differences() {
    std::printf("  Test 7: Float precision with small differences...\n");
    ChangeFilter<float> filter;
    filter.configureSlotWithDeadband(0, 0.002f);

    filter.isSignificantChange(0, 22.0f);
    bool result = filter.isSignificantChange(0, 22.001f);  // Change is 0.001, within 0.002
    ASSERT_FALSE(result);

    result = filter.isSignificantChange(0, 22.003f);  // Change is 0.003, exceeds 0.002
    ASSERT_TRUE(result);

    testsPassed++;
    std::printf("    PASS\n");
}

void test_negative_values() {
    std::printf("  Test 8: Negative temperature values...\n");
    ChangeFilter<float> filter;
    filter.configureSlotWithDeadband(0, 0.5f);

    filter.isSignificantChange(0, -40.0f);  // First reading: -40°C
    bool result = filter.isSignificantChange(0, -39.6f);  // Change is 0.4, within 0.5
    ASSERT_FALSE(result);

    result = filter.isSignificantChange(0, -39.4f);  // Change is 0.6, exceeds 0.5
    ASSERT_TRUE(result);

    testsPassed++;
    std::printf("    PASS\n");
}

void test_negative_to_positive_crossover() {
    std::printf("  Test 9: Negative to positive crossover...\n");
    ChangeFilter<float> filter;
    filter.configureSlotWithDeadband(0, 1.0f);

    filter.isSignificantChange(0, -0.4f);  // First reading
    bool result = filter.isSignificantChange(0, 0.4f);  // Change is 0.8, within 1.0
    ASSERT_FALSE(result);

    result = filter.isSignificantChange(0, 0.6f);  // Change is 1.0, exactly at boundary
    ASSERT_FALSE(result);  // Inclusive boundary

    result = filter.isSignificantChange(0, 0.7f);  // Change is 1.1, exceeds 1.0
    ASSERT_TRUE(result);

    testsPassed++;
    std::printf("    PASS\n");
}

void test_nan_suppressed() {
    std::printf("  Test 10: NaN values are suppressed...\n");
    ChangeFilter<float> filter;
    filter.configureSlotWithDeadband(0, 0.5f);

    bool result = filter.isSignificantChange(0, NAN);
    ASSERT_FALSE(result);

    testsPassed++;
    std::printf("    PASS\n");
}

void test_positive_inf_suppressed() {
    std::printf("  Test 11: Positive infinity is suppressed...\n");
    ChangeFilter<float> filter;
    filter.configureSlotWithDeadband(0, 0.5f);

    bool result = filter.isSignificantChange(0, INFINITY);
    ASSERT_FALSE(result);

    testsPassed++;
    std::printf("    PASS\n");
}

void test_negative_inf_suppressed() {
    std::printf("  Test 12: Negative infinity is suppressed...\n");
    ChangeFilter<float> filter;
    filter.configureSlotWithDeadband(0, 0.5f);

    bool result = filter.isSignificantChange(0, -INFINITY);
    ASSERT_FALSE(result);

    testsPassed++;
    std::printf("    PASS\n");
}

void test_slot_index_out_of_bounds() {
    std::printf("  Test 13: Slot index out of bounds returns false...\n");
    ChangeFilter<float> filter;
    filter.configureSlotWithDeadband(0, 0.5f);

    bool result = filter.isSignificantChange(8, 22.0f);  // MAXIMUM_SLOTS = 8, so index 8 is out of bounds
    ASSERT_FALSE(result);

    result = filter.isSignificantChange(100, 22.0f);
    ASSERT_FALSE(result);

    testsPassed++;
    std::printf("    PASS\n");
}

void test_value_equal_to_last_value() {
    std::printf("  Test 14: Value equal to last value is suppressed...\n");
    ChangeFilter<float> filter;
    filter.configureSlotWithDeadband(0, 0.5f);

    filter.isSignificantChange(0, 22.0f);  // First reading passes
    bool result = filter.isSignificantChange(0, 22.0f);  // Same value, change is 0
    ASSERT_FALSE(result);

    testsPassed++;
    std::printf("    PASS\n");
}

void test_reset_clears_all_state() {
    std::printf("  Test 15: Reset clears all state...\n");
    ChangeFilter<float> filter;
    filter.configureSlotWithDeadband(0, 0.5f);

    filter.isSignificantChange(0, 22.0f);  // First reading passes
    bool beforeReset = filter.isSignificantChange(0, 22.3f);  // Within deadband
    ASSERT_FALSE(beforeReset);

    filter.reset();
    // After reset, deadband is cleared (0.0), so all values pass
    bool afterReset = filter.isSignificantChange(0, 22.0f);
    ASSERT_TRUE(afterReset);

    testsPassed++;
    std::printf("    PASS\n");
}

void test_max_slot_index() {
    std::printf("  Test 16: Max slot index (7) works correctly...\n");
    ChangeFilter<float> filter;
    filter.configureSlotWithDeadband(7, 0.5f);

    bool result = filter.isSignificantChange(7, 22.0f);  // First reading passes
    ASSERT_TRUE(result);

    result = filter.isSignificantChange(7, 22.3f);  // Within deadband
    ASSERT_FALSE(result);

    testsPassed++;
    std::printf("    PASS\n");
}

void test_configure_out_of_bounds_ignored() {
    std::printf("  Test 17: Configure out of bounds is ignored...\n");
    ChangeFilter<float> filter;
    filter.configureSlotWithDeadband(100, 0.5f);  // Should be silently ignored

    // Slot 0 should still have default deadband (0.0)
    bool result = filter.isSignificantChange(0, 22.0f);
    ASSERT_TRUE(result);

    testsPassed++;
    std::printf("    PASS\n");
}

void test_changing_deadband_at_runtime() {
    std::printf("  Test 18: Changing deadband at runtime...\n");
    ChangeFilter<float> filter;
    filter.configureSlotWithDeadband(0, 0.5f);

    filter.isSignificantChange(0, 22.0f);  // First reading passes

    // Change is 0.3, within 0.5 — suppressed
    bool result1 = filter.isSignificantChange(0, 22.3f);
    ASSERT_FALSE(result1);

    // Reduce deadband to 0.2
    filter.configureSlotWithDeadband(0, 0.2f);

    // Change from 22.0 to 22.3 is 0.3, exceeds 0.2 — should pass
    bool result2 = filter.isSignificantChange(0, 22.3f);
    ASSERT_TRUE(result2);

    testsPassed++;
    std::printf("    PASS\n");
}

void test_unconfigured_slot_passes_all() {
    std::printf("  Test 19: Unconfigured slot (deadband 0) passes all...\n");
    ChangeFilter<float> filter;
    // Slot 0 is not configured — deadband is 0.0 by default

    bool result1 = filter.isSignificantChange(0, 22.0f);
    bool result2 = filter.isSignificantChange(0, 22.0f);  // Same value
    bool result3 = filter.isSignificantChange(0, 22.001f);  // Tiny change

    ASSERT_TRUE(result1);
    ASSERT_TRUE(result2);
    ASSERT_TRUE(result3);

    testsPassed++;
    std::printf("    PASS\n");
}

void test_maximum_slots_constant() {
    std::printf("  Test 20: Maximum slots constant matches SamplingScheduler...\n");
    ASSERT_EQ(ChangeFilter<float>::maximumSlots(), CHANGE_FILTER_MAXIMUM_SLOTS);
    ASSERT_EQ(CHANGE_FILTER_MAXIMUM_SLOTS, 8u);

    testsPassed++;
    std::printf("    PASS\n");
}

// ============================================================
// Int ChangeFilter Tests
// ============================================================

void test_int_first_reading_passes() {
    std::printf("  Test 21: Int first reading passes...\n");
    ChangeFilter<int> filter;
    filter.configureSlotWithDeadband(0, 5);

    bool result = filter.isSignificantChange(0, 100);
    ASSERT_TRUE(result);

    testsPassed++;
    std::printf("    PASS\n");
}

void test_int_within_deadband_suppressed() {
    std::printf("  Test 22: Int value within deadband suppressed...\n");
    ChangeFilter<int> filter;
    filter.configureSlotWithDeadband(0, 5);

    filter.isSignificantChange(0, 100);  // First reading
    bool result = filter.isSignificantChange(0, 103);  // Change is 3, within 5
    ASSERT_FALSE(result);

    testsPassed++;
    std::printf("    PASS\n");
}

void test_int_exceeding_deadband_passes() {
    std::printf("  Test 23: Int value exceeding deadband passes...\n");
    ChangeFilter<int> filter;
    filter.configureSlotWithDeadband(0, 5);

    filter.isSignificantChange(0, 100);  // First reading
    bool result = filter.isSignificantChange(0, 106);  // Change is 6, exceeds 5
    ASSERT_TRUE(result);

    testsPassed++;
    std::printf("    PASS\n");
}

void test_int_zero_deadband_passes_all() {
    std::printf("  Test 24: Int zero deadband passes all...\n");
    ChangeFilter<int> filter;
    filter.configureSlotWithDeadband(0, 0);

    bool result1 = filter.isSignificantChange(0, 100);
    bool result2 = filter.isSignificantChange(0, 100);  // Same value
    ASSERT_TRUE(result1);
    ASSERT_TRUE(result2);

    testsPassed++;
    std::printf("    PASS\n");
}

void test_int_exact_boundary_suppressed() {
    std::printf("  Test 25: Int exact deadband boundary suppressed...\n");
    ChangeFilter<int> filter;
    filter.configureSlotWithDeadband(0, 5);

    filter.isSignificantChange(0, 100);
    bool result = filter.isSignificantChange(0, 105);  // Change is exactly 5
    ASSERT_FALSE(result);  // Inclusive boundary

    testsPassed++;
    std::printf("    PASS\n");
}

void test_int_negative_values() {
    std::printf("  Test 26: Int negative values...\n");
    ChangeFilter<int> filter;
    filter.configureSlotWithDeadband(0, 10);

    filter.isSignificantChange(0, -50);
    bool result = filter.isSignificantChange(0, -45);  // Change is 5, within 10
    ASSERT_FALSE(result);

    result = filter.isSignificantChange(0, -39);  // Change is 11, exceeds 10
    ASSERT_TRUE(result);

    testsPassed++;
    std::printf("    PASS\n");
}

void test_int_large_values() {
    std::printf("  Test 27: Int large ADC values...\n");
    ChangeFilter<int> filter;
    filter.configureSlotWithDeadband(0, 100);

    filter.isSignificantChange(0, 4095);  // Max 12-bit ADC
    bool result = filter.isSignificantChange(0, 4100);  // Change is 5, within 100
    ASSERT_FALSE(result);

    result = filter.isSignificantChange(0, 4200);  // Change is 105, exceeds 100
    ASSERT_TRUE(result);

    testsPassed++;
    std::printf("    PASS\n");
}

void test_int_pulse_count() {
    std::printf("  Test 28: Int pulse count sensor...\n");
    ChangeFilter<int> filter;
    filter.configureSlotWithDeadband(0, 10);

    filter.isSignificantChange(0, 1000);
    bool result = filter.isSignificantChange(0, 1005);  // Change is 5, within 10
    ASSERT_FALSE(result);

    result = filter.isSignificantChange(0, 1011);  // Change is 11, exceeds 10
    ASSERT_TRUE(result);

    testsPassed++;
    std::printf("    PASS\n");
}

void test_int_reset() {
    std::printf("  Test 29: Int reset clears state...\n");
    ChangeFilter<int> filter;
    filter.configureSlotWithDeadband(0, 5);

    filter.isSignificantChange(0, 100);
    bool beforeReset = filter.isSignificantChange(0, 103);
    ASSERT_FALSE(beforeReset);

    filter.reset();
    bool afterReset = filter.isSignificantChange(0, 100);
    ASSERT_TRUE(afterReset);

    testsPassed++;
    std::printf("    PASS\n");
}

void test_int_out_of_bounds() {
    std::printf("  Test 30: Int slot out of bounds...\n");
    ChangeFilter<int> filter;
    filter.configureSlotWithDeadband(0, 5);

    bool result = filter.isSignificantChange(8, 100);
    ASSERT_FALSE(result);

    testsPassed++;
    std::printf("    PASS\n");
}

// ============================================================
// Edge Case Tests
// ============================================================

void test_all_slots_independent() {
    std::printf("  Test 31: All 8 slots independent...\n");
    ChangeFilter<float> filter;

    // Configure all slots with different deadbands
    for (int i = 0; i < 8; i++) {
        filter.configureSlotWithDeadband(static_cast<uint8_t>(i), 1.0f);
    }

    // First reading for all slots
    for (int i = 0; i < 8; i++) {
        bool result = filter.isSignificantChange(static_cast<uint8_t>(i), static_cast<float>(i * 10));
        ASSERT_TRUE(result);
    }

    // Within deadband for all slots
    for (int i = 0; i < 8; i++) {
        bool result = filter.isSignificantChange(static_cast<uint8_t>(i), static_cast<float>(i * 10 + 0.5f));
        ASSERT_FALSE(result);
    }

    testsPassed++;
    std::printf("    PASS\n");
}

void test_rapid_value_changes() {
    std::printf("  Test 32: Rapid value changes...\n");
    ChangeFilter<float> filter;
    filter.configureSlotWithDeadband(0, 0.5f);

    // First reading
    filter.isSignificantChange(0, 20.0f);

    // Oscillate within deadband
    bool r1 = filter.isSignificantChange(0, 20.3f);  // +0.3, suppressed
    bool r2 = filter.isSignificantChange(0, 19.8f);  // -0.2 from 20.0, suppressed
    bool r3 = filter.isSignificantChange(0, 20.4f);  // +0.4 from 20.0, suppressed
    ASSERT_FALSE(r1);
    ASSERT_FALSE(r2);
    ASSERT_FALSE(r3);

    // Break out of deadband
    bool r4 = filter.isSignificantChange(0, 20.6f);  // +0.6 from 20.0, passes
    ASSERT_TRUE(r4);

    // Now last value is 20.6
    bool r5 = filter.isSignificantChange(0, 20.8f);  // +0.2 from 20.6, suppressed
    ASSERT_FALSE(r5);

    testsPassed++;
    std::printf("    PASS\n");
}

void test_deadband_larger_than_value_range() {
    std::printf("  Test 33: Deadband larger than value range...\n");
    ChangeFilter<float> filter;
    filter.configureSlotWithDeadband(0, 100.0f);

    filter.isSignificantChange(0, 0.0f);  // First reading
    bool result = filter.isSignificantChange(0, 50.0f);  // Change is 50, within 100
    ASSERT_FALSE(result);

    result = filter.isSignificantChange(0, 101.0f);  // Change is 101, exceeds 100
    ASSERT_TRUE(result);

    testsPassed++;
    std::printf("    PASS\n");
}

void test_double_type_support() {
    std::printf("  Test 34: Double type support...\n");
    ChangeFilter<double> filter;
    filter.configureSlotWithDeadband(0, 0.0001);

    filter.isSignificantChange(0, 3.14159265);
    bool result = filter.isSignificantChange(0, 3.14159270);  // Change is ~5e-8, within 1e-4
    ASSERT_FALSE(result);

    result = filter.isSignificantChange(0, 3.1417);  // Change is ~1.3e-4, exceeds 1e-4
    ASSERT_TRUE(result);

    testsPassed++;
    std::printf("    PASS\n");
}

void test_double_nan_suppressed() {
    std::printf("  Test 35: Double NaN is suppressed...\n");
    ChangeFilter<double> filter;
    filter.configureSlotWithDeadband(0, 0.5);

    bool result = filter.isSignificantChange(0, NAN);
    ASSERT_FALSE(result);

    testsPassed++;
    std::printf("    PASS\n");
}

void test_double_inf_suppressed() {
    std::printf("  Test 36: Double infinity is suppressed...\n");
    ChangeFilter<double> filter;
    filter.configureSlotWithDeadband(0, 0.5);

    bool result = filter.isSignificantChange(0, INFINITY);
    ASSERT_FALSE(result);

    testsPassed++;
    std::printf("    PASS\n");
}

void test_temperature_sensor_realistic_scenario() {
    std::printf("  Test 37: Realistic temperature sensor scenario...\n");
    ChangeFilter<float> filter;
    filter.configureSlotWithDeadband(0, 0.5f);

    // Simulate a temperature sensor that changes slowly
    // 22.0°C, then 22.1, 22.2, 22.3, 22.4, 22.5, 22.6
    int passCount = 0;

    if (filter.isSignificantChange(0, 22.0f)) passCount++;  // First reading: PASS
    if (filter.isSignificantChange(0, 22.1f)) passCount++;  // +0.1: suppress
    if (filter.isSignificantChange(0, 22.2f)) passCount++;  // +0.2: suppress
    if (filter.isSignificantChange(0, 22.3f)) passCount++;  // +0.3: suppress
    if (filter.isSignificantChange(0, 22.4f)) passCount++;  // +0.4: suppress
    if (filter.isSignificantChange(0, 22.5f)) passCount++;  // +0.5: suppress (inclusive)
    if (filter.isSignificantChange(0, 22.6f)) passCount++;  // +0.6: PASS

    ASSERT_EQ(passCount, 2);  // Only first and 22.6 should pass

    testsPassed++;
    std::printf("    PASS\n");
}

void test_humidity_sensor_scenario() {
    std::printf("  Test 38: Humidity sensor scenario...\n");
    ChangeFilter<float> filter;
    filter.configureSlotWithDeadband(0, 2.0f);  // 2% deadband for humidity

    int passCount = 0;

    if (filter.isSignificantChange(0, 45.0f)) passCount++;  // First: PASS
    if (filter.isSignificantChange(0, 45.5f)) passCount++;  // +0.5: suppress
    if (filter.isSignificantChange(0, 46.0f)) passCount++;  // +1.0: suppress
    if (filter.isSignificantChange(0, 47.0f)) passCount++;  // +2.0: suppress (inclusive)
    if (filter.isSignificantChange(0, 48.0f)) passCount++;  // +3.0: PASS

    ASSERT_EQ(passCount, 2);

    testsPassed++;
    std::printf("    PASS\n");
}

void test_pressure_sensor_scenario() {
    std::printf("  Test 39: Pressure sensor scenario...\n");
    ChangeFilter<float> filter;
    filter.configureSlotWithDeadband(0, 1.0f);  // 1 hPa deadband

    int passCount = 0;

    if (filter.isSignificantChange(0, 1013.25f)) passCount++;  // First: PASS
    if (filter.isSignificantChange(0, 1013.50f)) passCount++;  // +0.25: suppress
    if (filter.isSignificantChange(0, 1014.00f)) passCount++;  // +0.75: suppress
    if (filter.isSignificantChange(0, 1014.26f)) passCount++;  // +1.01: PASS

    ASSERT_EQ(passCount, 2);

    testsPassed++;
    std::printf("    PASS\n");
}

void test_multiple_resets() {
    std::printf("  Test 40: Multiple resets...\n");
    ChangeFilter<float> filter;
    filter.configureSlotWithDeadband(0, 0.5f);

    // First cycle
    filter.isSignificantChange(0, 22.0f);  // First reading
    filter.reset();
    filter.configureSlotWithDeadband(0, 0.5f);  // Reconfigure after reset

    // Second cycle
    bool r1 = filter.isSignificantChange(0, 22.0f);  // First reading after reset
    bool r2 = filter.isSignificantChange(0, 22.3f);  // Within deadband
    ASSERT_TRUE(r1);
    ASSERT_FALSE(r2);

    testsPassed++;
    std::printf("    PASS\n");
}

void test_last_value_updated_only_on_pass() {
    std::printf("  Test 41: Last value updated only on pass...\n");
    ChangeFilter<float> filter;
    filter.configureSlotWithDeadband(0, 0.5f);

    filter.isSignificantChange(0, 22.0f);  // First reading, lastValue = 22.0

    // 22.3 is suppressed — lastValue should still be 22.0
    filter.isSignificantChange(0, 22.3f);

    // 22.5 is suppressed (exactly at boundary) — lastValue should still be 22.0
    filter.isSignificantChange(0, 22.5f);

    // 22.6 exceeds deadband from 22.0 — should pass
    bool result = filter.isSignificantChange(0, 22.6f);
    ASSERT_TRUE(result);

    // Now lastValue is 22.6
    // 22.8 is within 0.5 of 22.6 — should be suppressed
    result = filter.isSignificantChange(0, 22.8f);
    ASSERT_FALSE(result);

    testsPassed++;
    std::printf("    PASS\n");
}

void test_slot_index_zero() {
    std::printf("  Test 42: Slot index zero works correctly...\n");
    ChangeFilter<float> filter;
    filter.configureSlotWithDeadband(0, 0.5f);

    bool result = filter.isSignificantChange(0, 22.0f);
    ASSERT_TRUE(result);

    testsPassed++;
    std::printf("    PASS\n");
}

void test_configure_overwrites_deadband() {
    std::printf("  Test 43: Configure overwrites existing deadband...\n");
    ChangeFilter<float> filter;
    filter.configureSlotWithDeadband(0, 1.0f);

    filter.isSignificantChange(0, 22.0f);  // First reading

    // Change 0.6 is within 1.0 — suppressed
    bool r1 = filter.isSignificantChange(0, 22.6f);
    ASSERT_FALSE(r1);

    // Reduce deadband to 0.5
    filter.configureSlotWithDeadband(0, 0.5f);

    // Change from 22.0 to 22.6 is 0.6, exceeds 0.5 — should pass
    bool r2 = filter.isSignificantChange(0, 22.6f);
    ASSERT_TRUE(r2);

    testsPassed++;
    std::printf("    PASS\n");
}

void test_int_equal_values_suppressed() {
    std::printf("  Test 44: Int equal values are suppressed...\n");
    ChangeFilter<int> filter;
    filter.configureSlotWithDeadband(0, 1);

    filter.isSignificantChange(0, 100);  // First reading
    bool result = filter.isSignificantChange(0, 100);  // Same value
    ASSERT_FALSE(result);

    testsPassed++;
    std::printf("    PASS\n");
}

void test_deadband_one_int() {
    std::printf("  Test 45: Int deadband of 1...\n");
    ChangeFilter<int> filter;
    filter.configureSlotWithDeadband(0, 1);

    filter.isSignificantChange(0, 100);  // First reading
    bool r1 = filter.isSignificantChange(0, 101);  // Change is 1, exactly at boundary
    ASSERT_FALSE(r1);  // Inclusive

    bool r2 = filter.isSignificantChange(0, 102);  // Change is 2, exceeds 1
    ASSERT_TRUE(r2);

    testsPassed++;
    std::printf("    PASS\n");
}

// ============================================================
// Main
// ============================================================

int main() {
    std::printf("ChangeFilter Tests\n");
    std::printf("==================\n");

    // Float tests
    test_first_reading_always_passes();
    test_value_within_deadband_suppressed();
    test_value_exceeding_deadband_passes();
    test_zero_deadband_disables_filtering();
    test_exact_deadband_boundary_suppressed();
    test_multiple_slots_independent();
    test_float_precision_small_differences();
    test_negative_values();
    test_negative_to_positive_crossover();
    test_nan_suppressed();
    test_positive_inf_suppressed();
    test_negative_inf_suppressed();
    test_slot_index_out_of_bounds();
    test_value_equal_to_last_value();
    test_reset_clears_all_state();
    test_max_slot_index();
    test_configure_out_of_bounds_ignored();
    test_changing_deadband_at_runtime();
    test_unconfigured_slot_passes_all();
    test_maximum_slots_constant();

    // Int tests
    test_int_first_reading_passes();
    test_int_within_deadband_suppressed();
    test_int_exceeding_deadband_passes();
    test_int_zero_deadband_passes_all();
    test_int_exact_boundary_suppressed();
    test_int_negative_values();
    test_int_large_values();
    test_int_pulse_count();
    test_int_reset();
    test_int_out_of_bounds();

    // Edge case tests
    test_all_slots_independent();
    test_rapid_value_changes();
    test_deadband_larger_than_value_range();
    test_double_type_support();
    test_double_nan_suppressed();
    test_double_inf_suppressed();
    test_temperature_sensor_realistic_scenario();
    test_humidity_sensor_scenario();
    test_pressure_sensor_scenario();
    test_multiple_resets();
    test_last_value_updated_only_on_pass();
    test_slot_index_zero();
    test_configure_overwrites_deadband();
    test_int_equal_values_suppressed();
    test_deadband_one_int();

    std::printf("\n==================\n");
    std::printf("Results: %d passed, %d failed, %d total\n",
               testsPassed, testsFailed, testsPassed + testsFailed);

    return testsFailed > 0 ? 1 : 0;
}
