/**
 * @file RulesEngine.cpp
 * @brief Implementation of on-device automation rules engine.
 * @ingroup core_module
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-11-23
 * @version 1.0
 */

#include "RulesEngine.h"

RulesEngine::RulesEngine()
    : ruleCount(0), actionResultObserver(nullptr), actionResultObserverContext(nullptr) {
    for (uint8_t i = 0; i < RULES_ENGINE_MAX_RULES; i++) {
        rules[i].active = false;
        rules[i].compositeEventId = -1;
        rules[i].actionCount = 0;
        for (uint8_t j = 0; j < RULES_ENGINE_MAX_ACTIONS_PER_RULE; j++) {
            rules[i].actions[j].active = false;
        }
    }
}

bool RulesEngine::addRule(const int compositeEventId, CommandHandler* target, const int commandId) {
    return addRule(compositeEventId, target, commandId, 0);
}

bool RulesEngine::addRule(const int compositeEventId, CommandHandler* target, const int commandId, const int32_t payload) {
    // Reject if rule already exists for this composite event ID
    if (findRule(compositeEventId)) {
        return false;
    }

    // Find an inactive slot
    Rule* rule = findInactiveSlot();
    if (!rule) {
        return false;
    }

    // Initialize the rule
    rule->compositeEventId = compositeEventId;
    rule->active = true;
    rule->actionCount = 0;

    // Add the first action
    rule->actions[0].target = target;
    rule->actions[0].commandId = commandId;
    rule->actions[0].payload = payload;
    rule->actions[0].active = true;
    rule->actionCount = 1;

    ruleCount++;
    return true;
}

bool RulesEngine::addActionToRule(const int compositeEventId, CommandHandler* target, const int commandId) {
    return addActionToRule(compositeEventId, target, commandId, 0);
}

bool RulesEngine::addActionToRule(const int compositeEventId, CommandHandler* target, const int commandId, const int32_t payload) {
    Rule* rule = findRule(compositeEventId);
    if (!rule) {
        return false;
    }

    // Check if action array is full
    if (rule->actionCount >= RULES_ENGINE_MAX_ACTIONS_PER_RULE) {
        return false;
    }

    // Add the action to the next available slot
    uint8_t actionIndex = rule->actionCount;
    rule->actions[actionIndex].target = target;
    rule->actions[actionIndex].commandId = commandId;
    rule->actions[actionIndex].payload = payload;
    rule->actions[actionIndex].active = true;
    rule->actionCount++;

    return true;
}

bool RulesEngine::removeRule(const int compositeEventId) {
    Rule* rule = findRule(compositeEventId);
    if (!rule) {
        return false;
    }

    rule->active = false;
    rule->compositeEventId = -1;
    rule->actionCount = 0;
    for (uint8_t j = 0; j < RULES_ENGINE_MAX_ACTIONS_PER_RULE; j++) {
        rule->actions[j].active = false;
    }

    ruleCount--;
    return true;
}

void RulesEngine::cleanupRules() {
    for (uint8_t i = 0; i < RULES_ENGINE_MAX_RULES; i++) {
        rules[i].active = false;
        rules[i].compositeEventId = -1;
        rules[i].actionCount = 0;
        for (uint8_t j = 0; j < RULES_ENGINE_MAX_ACTIONS_PER_RULE; j++) {
            rules[i].actions[j].active = false;
        }
    }
    ruleCount = 0;
}

void RulesEngine::setActionResultObserver(const RuleActionResultObserver observer, void* context) {
    actionResultObserver = observer;
    actionResultObserverContext = context;
}

void RulesEngine::on(const Event event) {
    Rule* rule = findRule(event.identifier);
    if (!rule) {
        // No rule for this event — no-op
        return;
    }

    // Dispatch all actions in order
    for (uint8_t i = 0; i < rule->actionCount; i++) {
        RuleAction& action = rule->actions[i];
        if (action.active && action.target) {
            const Command cmd(action.commandId, action.payload);
            const CommandExecutionResult result = action.target->handleChecked(cmd);
            if (actionResultObserver != nullptr) {
                actionResultObserver(event.identifier, action.target, cmd, result, actionResultObserverContext);
            }
        }
    }
}

Rule* RulesEngine::findRule(const int compositeEventId) {
    for (uint8_t i = 0; i < RULES_ENGINE_MAX_RULES; i++) {
        if (rules[i].active && rules[i].compositeEventId == compositeEventId) {
            return &rules[i];
        }
    }
    return nullptr;
}

Rule* RulesEngine::findInactiveSlot() {
    for (uint8_t i = 0; i < RULES_ENGINE_MAX_RULES; i++) {
        if (!rules[i].active) {
            return &rules[i];
        }
    }
    return nullptr;
}
