/**
 * @file RulesEngine.h
 * @brief On-device automation rules that map composite events to actuator commands.
 * @ingroup core_module
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-11-23
 * @version 1.0
 *
 * The RulesEngine listens for composite events from EventAggregator and executes
 * predefined actions (actuator Commands) when a pattern matches. It can optionally
 * report checked action outcomes; legacy handlers report Unknown. It implements
 * EventHandler so EventAggregator calls RulesEngine::on(Event) when a pattern
 * matches. RulesEngine looks up the rule for that composite event ID and dispatches
 * the associated Commands to target actuators.
 *
 * All storage uses fixed-size arrays to avoid heap allocation.
 */

#ifndef MODEST_IOT_NANO_FRAMEWORK_RULES_ENGINE_H
#define MODEST_IOT_NANO_FRAMEWORK_RULES_ENGINE_H

#include <cstdint>

#include "Command.h"
#include "Event.h"
#include "EventHandler.h"
#include "CommandHandler.h"

/**
 * @brief Maximum number of rules in the RulesEngine.
 * @note Matches EventAggregator's 4 patterns for symmetry.
 */
static constexpr uint8_t RULES_ENGINE_MAX_RULES = 4;

/**
 * @brief Maximum number of actions per rule.
 * @note 4 actions covers typical automation (e.g., turn on buzzer, send telemetry, etc.).
 */
static constexpr uint8_t RULES_ENGINE_MAX_ACTIONS_PER_RULE = 4;

/**
 * @brief Single action within a rule.
 * @ingroup core_module
 *
 * When a rule's composite event matches, each RuleAction is executed by calling
 * `target->handle(Command(commandId, payload))` on the target CommandHandler.
 */
struct RuleAction {
    CommandHandler* target = nullptr; ///< CommandHandler (e.g., actuator) that receives the command.
    int commandId = -1; ///< Command identifier (e.g., TURN_ON, PLAY_TONE).
    int32_t payload = 0; ///< Command payload (e.g., angle, brightness, frequency).
    bool active = false; ///< True if this action slot is registered.
};

/**
 * @brief Rule that maps a composite event ID to one or more actions.
 * @ingroup core_module
 *
 * When the RulesEngine receives an event whose identifier matches this rule's
 * compositeEventId, it executes all active actions in order.
 */
struct Rule {
    int compositeEventId = -1; ///< Event identifier that triggers this rule.
    RuleAction actions[RULES_ENGINE_MAX_ACTIONS_PER_RULE]; ///< Up to 4 actions per rule.
    uint8_t actionCount = 0; ///< Number of active actions.
    bool active = false; ///< True if this rule slot is registered.
};

/**
 * @brief Callback used to observe the result of a dispatched rule action.
 * @param compositeEventId Event identifier that triggered the rule.
 * @param target Handler that received the command.
 * @param command Command that was dispatched.
 * @param result Driver-level execution result.
 * @param context Caller-provided observer context.
 */
using RuleActionResultObserver = void (*)(int compositeEventId, CommandHandler* target, Command command,
                                         CommandExecutionResult result, void* context);

/**
 * @brief Maps composite events to automated actuator Commands.
 * @ingroup core_module
 *
 * Implements EventHandler to receive composite events from EventAggregator.
 * When a composite event arrives, RulesEngine looks up the matching rule and
 * dispatches all associated Commands to their target actuators.
 *
 * @par Zero Heap Allocation
 * Uses fixed-size rule array (4 rules) and action arrays (4 actions per rule).
 * No dynamic allocation.
 *
 * @par Threading
 * The on(Event) method and result observer run synchronously on the sampler task,
 * not in interrupt context. Actions and observers must be fast and must not block.
 *
 * @par Example
 * @code
 * RulesEngine rulesEngine;
 * // When composite event 100 occurs, turn on the buzzer
 * rulesEngine.addRule(100, &buzzer, Buzzer::TURN_ON_COMMAND_IDENTIFIER);
 * // When composite event 101 occurs, play tone 1000 Hz for 500 ms
 * rulesEngine.addRule(101, &buzzer, Buzzer::PLAY_TONE_COMMAND_IDENTIFIER, 1000);
 *
 * // Wire EventAggregator to RulesEngine
 * eventAggregator.setOutputHandler(&rulesEngine);
 * @endcode
 */
class RulesEngine : public EventHandler {
public:
    /**
     * @brief Constructs a RulesEngine with no rules registered.
     */
    RulesEngine();

    /**
     * @brief Registers a rule: when compositeEventId occurs, execute the command.
     * @param compositeEventId The event identifier that triggers this rule.
     * @param target The CommandHandler (e.g., actuator) that receives the command.
     * @param commandId The command identifier (e.g., TURN_ON, PLAY_TONE).
     * @return True if the rule was registered, false if the rule array is full or
     *         the rule already exists (use addActionToRule to add more actions).
     * @note Creates a new rule slot. To add additional actions to the same rule,
     *       use addActionToRule().
     */
    bool addRule(int compositeEventId, CommandHandler* target, int commandId);

    /**
     * @brief Registers a rule with a payload.
     * @param compositeEventId The event identifier that triggers this rule.
     * @param target The CommandHandler (e.g., actuator) that receives the command.
     * @param commandId The command identifier (e.g., PLAY_TONE, DRIVE_TO_ANGLE).
     * @param payload Parameterized data (e.g., frequency, angle, brightness).
     * @return True if the rule was registered, false if the rule array is full or
     *         the rule already exists (use addActionToRule to add more actions).
     */
    bool addRule(int compositeEventId, CommandHandler* target, int commandId, int32_t payload);

    /**
     * @brief Adds an additional action to an existing rule.
     * @param compositeEventId The event identifier of the existing rule.
     * @param target The CommandHandler (e.g., actuator) that receives the command.
     * @param commandId The command identifier.
     * @return True if the action was added, false if the rule is not found or
     *         the action array is full.
     * @note Use this to add multiple actions to a single rule. For example,
     *       first addRule() to create the rule, then addActionToRule() to add
     *       a second action (e.g., turn on buzzer AND send telemetry).
     */
    bool addActionToRule(int compositeEventId, CommandHandler* target, int commandId);

    /**
     * @brief Adds an additional action with payload to an existing rule.
     * @param compositeEventId The event identifier of the existing rule.
     * @param target The CommandHandler (e.g., actuator) that receives the command.
     * @param commandId The command identifier.
     * @param payload Parameterized data.
     * @return True if the action was added, false if the rule is not found or
     *         the action array is full.
     */
    bool addActionToRule(int compositeEventId, CommandHandler* target, int commandId, int32_t payload);

    /**
     * @brief Removes a registered rule by composite event ID.
     * @param compositeEventId The event identifier of the rule to remove.
     * @return True if the rule was found and removed, false if not found.
     * @note After removal, the freed slot can be reused by a subsequent addRule() call.
     */
    bool removeRule(int compositeEventId);

    /**
     * @brief Removes all registered rules.
     * @note Convenience method for housekeeping: clears all active rules in one call
     *       so developers do not need to track and remove each rule individually.
     *       Typically called from the Device destructor before stopAsynchronousEngine().
     */
    void cleanupRules();

    /**
     * @brief Sets an optional observer for checked rule-action outcomes.
     * @param observer Callback to receive each dispatched action result, or `nullptr` to disable.
     * @param context Opaque value passed to the callback.
     */
    void setActionResultObserver(RuleActionResultObserver observer, void* context = nullptr);

    /**
     * @brief Handles incoming composite events from EventAggregator.
     * @param event The composite event to match against registered rules.
     * @note Implements EventHandler::on(). Looks up the rule by event identifier
     *       and dispatches all associated Commands to their target actuators.
     *       Runs on the sampler task context — actions must be fast.
     */
    void on(Event event) override;

private:
    /** @brief Registered rules. */
    Rule rules[RULES_ENGINE_MAX_RULES];

    /** @brief Number of active rules. */
    uint8_t ruleCount;
    RuleActionResultObserver actionResultObserver;
    void* actionResultObserverContext;

    /**
     * @brief Finds a rule by composite event ID.
     * @param compositeEventId The event identifier to search for.
     * @return Pointer to the rule, or nullptr if not found.
     */
    Rule* findRule(const int compositeEventId);

    /**
     * @brief Finds an inactive rule slot.
     * @return Pointer to an inactive rule slot, or nullptr if all slots are active.
     */
    Rule* findInactiveSlot();
};

#endif // MODEST_IOT_NANO_FRAMEWORK_RULES_ENGINE_H
