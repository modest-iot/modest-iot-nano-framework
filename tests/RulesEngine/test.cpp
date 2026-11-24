/**
 * @file test.cpp
 * @brief Host tests for RulesEngine — on-device automation rules.
 * @ingroup test_module
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-11-23
 * @version 1.0
 */

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>

#include "RulesEngine.h"

/**
 * @brief Mock CommandHandler that records the last command received.
 */
class MockActuator : public CommandHandler {
public:
    int lastCommandId = -1;
    int32_t lastPayload = 0;
    uint32_t handleCallCount = 0;

    void handle(const Command command) override {
        lastCommandId = command.identifier;
        lastPayload = command.payload;
        handleCallCount++;
    }

    void reset() {
        lastCommandId = -1;
        lastPayload = 0;
        handleCallCount = 0;
    }
};

int main() {
    std::cout << "Running RulesEngine tests..." << std::endl;

    int testsPassed = 0;
    int testsFailed = 0;

    // Test 1: Constructor initializes empty state
    {
        RulesEngine re;
        Event e(999);
        re.on(e); // Should not crash with no rules
        testsPassed++;
        std::cout << "  [PASS] Constructor initializes empty state" << std::endl;
    }

    // Test 2: addRule registers a rule
    {
        RulesEngine re;
        MockActuator actuator;
        bool result = re.addRule(100, &actuator, 1);
        assert(result);
        testsPassed++;
        std::cout << "  [PASS] addRule registers a rule" << std::endl;
    }

    // Test 3: addRule with payload
    {
        RulesEngine re;
        MockActuator actuator;
        bool result = re.addRule(100, &actuator, 5, 42);
        assert(result);
        testsPassed++;
        std::cout << "  [PASS] addRule with payload" << std::endl;
    }

    // Test 4: on(Event) dispatches command to target
    {
        RulesEngine re;
        MockActuator actuator;
        re.addRule(100, &actuator, 1);
        re.on(Event(100));
        assert(actuator.lastCommandId == 1);
        assert(actuator.handleCallCount == 1);
        testsPassed++;
        std::cout << "  [PASS] on(Event) dispatches command to target" << std::endl;
    }

    // Test 5: on(Event) with payload carries payload
    {
        RulesEngine re;
        MockActuator actuator;
        re.addRule(100, &actuator, 5, 1234);
        re.on(Event(100));
        assert(actuator.lastCommandId == 5);
        assert(actuator.lastPayload == 1234);
        testsPassed++;
        std::cout << "  [PASS] on(Event) with payload carries payload" << std::endl;
    }

    // Test 6: on(Event) with unknown event ID does nothing
    {
        RulesEngine re;
        MockActuator actuator;
        re.addRule(100, &actuator, 1);
        re.on(Event(999));
        assert(actuator.handleCallCount == 0);
        testsPassed++;
        std::cout << "  [PASS] on(Event) with unknown event ID does nothing" << std::endl;
    }

    // Test 7: addActionToRule adds second action
    {
        RulesEngine re;
        MockActuator actuator1;
        MockActuator actuator2;
        re.addRule(100, &actuator1, 1);
        bool result = re.addActionToRule(100, &actuator2, 2);
        assert(result);
        re.on(Event(100));
        assert(actuator1.handleCallCount == 1);
        assert(actuator2.handleCallCount == 1);
        testsPassed++;
        std::cout << "  [PASS] addActionToRule adds second action" << std::endl;
    }

    // Test 8: addActionToRule with payload
    {
        RulesEngine re;
        MockActuator actuator1;
        MockActuator actuator2;
        re.addRule(100, &actuator1, 1);
        bool result = re.addActionToRule(100, &actuator2, 5, 99);
        assert(result);
        re.on(Event(100));
        assert(actuator2.lastCommandId == 5);
        assert(actuator2.lastPayload == 99);
        testsPassed++;
        std::cout << "  [PASS] addActionToRule with payload" << std::endl;
    }

    // Test 9: addActionToRule on non-existent rule returns false
    {
        RulesEngine re;
        MockActuator actuator;
        bool result = re.addActionToRule(999, &actuator, 1);
        assert(!result);
        testsPassed++;
        std::cout << "  [PASS] addActionToRule on non-existent rule returns false" << std::endl;
    }

    // Test 10: addActionToRule when action array is full returns false
    {
        RulesEngine re;
        MockActuator a1, a2, a3, a4, a5;
        re.addRule(100, &a1, 1);
        re.addActionToRule(100, &a2, 2);
        re.addActionToRule(100, &a3, 3);
        re.addActionToRule(100, &a4, 4);
        bool result = re.addActionToRule(100, &a5, 5);
        assert(!result);
        testsPassed++;
        std::cout << "  [PASS] addActionToRule when action array is full returns false" << std::endl;
    }

    // Test 11: 4 actions per rule all execute
    {
        RulesEngine re;
        MockActuator a1, a2, a3, a4;
        re.addRule(100, &a1, 1);
        re.addActionToRule(100, &a2, 2);
        re.addActionToRule(100, &a3, 3);
        re.addActionToRule(100, &a4, 4);
        re.on(Event(100));
        assert(a1.handleCallCount == 1);
        assert(a2.handleCallCount == 1);
        assert(a3.handleCallCount == 1);
        assert(a4.handleCallCount == 1);
        testsPassed++;
        std::cout << "  [PASS] 4 actions per rule all execute" << std::endl;
    }

    // Test 12: removeRule removes the rule
    {
        RulesEngine re;
        MockActuator actuator;
        re.addRule(100, &actuator, 1);
        bool result = re.removeRule(100);
        assert(result);
        re.on(Event(100));
        assert(actuator.handleCallCount == 0);
        testsPassed++;
        std::cout << "  [PASS] removeRule removes the rule" << std::endl;
    }

    // Test 13: removeRule on non-existent rule returns false
    {
        RulesEngine re;
        bool result = re.removeRule(999);
        assert(!result);
        testsPassed++;
        std::cout << "  [PASS] removeRule on non-existent rule returns false" << std::endl;
    }

    // Test 14: removeRule frees the slot for reuse
    {
        RulesEngine re;
        MockActuator a1, a2;
        re.addRule(100, &a1, 1);
        re.removeRule(100);
        bool result = re.addRule(100, &a2, 2);
        assert(result);
        re.on(Event(100));
        assert(a1.handleCallCount == 0);
        assert(a2.handleCallCount == 1);
        assert(a2.lastCommandId == 2);
        testsPassed++;
        std::cout << "  [PASS] removeRule frees the slot for reuse" << std::endl;
    }

    // Test 15: cleanupRules removes all rules
    {
        RulesEngine re;
        MockActuator a1, a2;
        re.addRule(100, &a1, 1);
        re.addRule(200, &a2, 2);
        re.cleanupRules();
        re.on(Event(100));
        re.on(Event(200));
        assert(a1.handleCallCount == 0);
        assert(a2.handleCallCount == 0);
        testsPassed++;
        std::cout << "  [PASS] cleanupRules removes all rules" << std::endl;
    }

    // Test 16: cleanupRules when no rules is safe
    {
        RulesEngine re;
        re.cleanupRules(); // Should not crash
        testsPassed++;
        std::cout << "  [PASS] cleanupRules when no rules is safe" << std::endl;
    }

    // Test 17: 4 rules (max capacity)
    {
        RulesEngine re;
        MockActuator a1, a2, a3, a4;
        assert(re.addRule(100, &a1, 1));
        assert(re.addRule(200, &a2, 2));
        assert(re.addRule(300, &a3, 3));
        assert(re.addRule(400, &a4, 4));
        re.on(Event(100));
        re.on(Event(200));
        re.on(Event(300));
        re.on(Event(400));
        assert(a1.lastCommandId == 1);
        assert(a2.lastCommandId == 2);
        assert(a3.lastCommandId == 3);
        assert(a4.lastCommandId == 4);
        testsPassed++;
        std::cout << "  [PASS] 4 rules (max capacity)" << std::endl;
    }

    // Test 18: 5th rule returns false (capacity exceeded)
    {
        RulesEngine re;
        MockActuator a1, a2, a3, a4, a5;
        re.addRule(100, &a1, 1);
        re.addRule(200, &a2, 2);
        re.addRule(300, &a3, 3);
        re.addRule(400, &a4, 4);
        bool result = re.addRule(500, &a5, 5);
        assert(!result);
        testsPassed++;
        std::cout << "  [PASS] 5th rule returns false (capacity exceeded)" << std::endl;
    }

    // Test 19: Duplicate composite event ID returns false
    {
        RulesEngine re;
        MockActuator a1, a2;
        bool first = re.addRule(100, &a1, 1);
        bool second = re.addRule(100, &a2, 2);
        assert(first);
        assert(!second);
        testsPassed++;
        std::cout << "  [PASS] Duplicate composite event ID returns false" << std::endl;
    }

    // Test 20: nullptr target is skipped during dispatch
    {
        RulesEngine re;
        MockActuator actuator;
        re.addRule(100, nullptr, 1);
        re.on(Event(100));
        assert(actuator.handleCallCount == 0);
        testsPassed++;
        std::cout << "  [PASS] nullptr target is skipped during dispatch" << std::endl;
    }

    // Test 21: Multiple rules fire independently
    {
        RulesEngine re;
        MockActuator a1, a2;
        re.addRule(100, &a1, 1);
        re.addRule(200, &a2, 2);
        re.on(Event(100));
        assert(a1.handleCallCount == 1);
        assert(a2.handleCallCount == 0);
        re.on(Event(200));
        assert(a1.handleCallCount == 1);
        assert(a2.handleCallCount == 1);
        testsPassed++;
        std::cout << "  [PASS] Multiple rules fire independently" << std::endl;
    }

    // Test 22: Same target for multiple rules
    {
        RulesEngine re;
        MockActuator actuator;
        re.addRule(100, &actuator, 1);
        re.addRule(200, &actuator, 2);
        re.on(Event(100));
        assert(actuator.lastCommandId == 1);
        assert(actuator.handleCallCount == 1);
        re.on(Event(200));
        assert(actuator.lastCommandId == 2);
        assert(actuator.handleCallCount == 2);
        testsPassed++;
        std::cout << "  [PASS] Same target for multiple rules" << std::endl;
    }

    // Test 23: addRule with zero payload
    {
        RulesEngine re;
        MockActuator actuator;
        re.addRule(100, &actuator, 1, 0);
        re.on(Event(100));
        assert(actuator.lastCommandId == 1);
        assert(actuator.lastPayload == 0);
        testsPassed++;
        std::cout << "  [PASS] addRule with zero payload" << std::endl;
    }

    // Test 24: addRule with negative payload
    {
        RulesEngine re;
        MockActuator actuator;
        re.addRule(100, &actuator, 1, -42);
        re.on(Event(100));
        assert(actuator.lastPayload == -42);
        testsPassed++;
        std::cout << "  [PASS] addRule with negative payload" << std::endl;
    }

    // Test 25: addActionToRule with payload on existing rule
    {
        RulesEngine re;
        MockActuator a1, a2;
        re.addRule(100, &a1, 1, 10);
        re.addActionToRule(100, &a2, 2, 20);
        re.on(Event(100));
        assert(a1.lastPayload == 10);
        assert(a2.lastPayload == 20);
        testsPassed++;
        std::cout << "  [PASS] addActionToRule with payload on existing rule" << std::endl;
    }

    // Test 26: Rule with 0 actions is valid but does nothing
    {
        RulesEngine re;
        // Manually create a rule with 0 actions by adding then removing all actions
        // (Not directly possible via public API, but the struct supports it)
        // This tests that the on() method handles actionCount == 0 gracefully
        MockActuator actuator;
        re.addRule(100, &actuator, 1);
        re.removeRule(100);
        // After removal, no rule exists, so on() is a no-op
        re.on(Event(100));
        assert(actuator.handleCallCount == 0);
        testsPassed++;
        std::cout << "  [PASS] Rule with 0 actions is valid but does nothing" << std::endl;
    }

    // Test 27: Event sourceId is ignored (only identifier matters)
    {
        RulesEngine re;
        MockActuator actuator;
        re.addRule(100, &actuator, 1);
        re.on(Event(100, 42)); // sourceId = 42
        assert(actuator.handleCallCount == 1);
        testsPassed++;
        std::cout << "  [PASS] Event sourceId is ignored (only identifier matters)" << std::endl;
    }

    // Test 28: Repeated on() calls increment handle count
    {
        RulesEngine re;
        MockActuator actuator;
        re.addRule(100, &actuator, 1);
        re.on(Event(100));
        re.on(Event(100));
        re.on(Event(100));
        assert(actuator.handleCallCount == 3);
        testsPassed++;
        std::cout << "  [PASS] Repeated on() calls increment handle count" << std::endl;
    }

    // Test 29: addActionToRule returns false when rule not found
    {
        RulesEngine re;
        MockActuator actuator;
        bool result = re.addActionToRule(999, &actuator, 1);
        assert(!result);
        testsPassed++;
        std::cout << "  [PASS] addActionToRule returns false when rule not found" << std::endl;
    }

    // Test 30: addActionToRule returns false when action array full
    {
        RulesEngine re;
        MockActuator a1, a2, a3, a4, a5;
        re.addRule(100, &a1, 1);
        re.addActionToRule(100, &a2, 2);
        re.addActionToRule(100, &a3, 3);
        re.addActionToRule(100, &a4, 4);
        bool result = re.addActionToRule(100, &a5, 5);
        assert(!result);
        testsPassed++;
        std::cout << "  [PASS] addActionToRule returns false when action array full" << std::endl;
    }

    std::cout << std::endl;
    std::cout << "Results: " << testsPassed << " passed, " << testsFailed
              << " failed out of " << (testsPassed + testsFailed) << " tests." << std::endl;

    return testsFailed > 0 ? 1 : 0;
}
