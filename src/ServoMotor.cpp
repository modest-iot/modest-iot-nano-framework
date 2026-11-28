/**
 * @file ServoMotor.cpp
 * @brief Implements bounded-angle servo control and buffered-angle command execution
 *        using the ESP32 LEDC peripheral for native PWM output.
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-06-06
 * @version 1.0
 */

#include "ServoMotor.h"
#include <Arduino.h>


static constexpr int LEDC_PERIPHERAL_RESOLUTION_BITS = 13;
static constexpr int LEDC_PERIPHERAL_MAX_DUTY = (1 << LEDC_PERIPHERAL_RESOLUTION_BITS) - 1;

int ServoMotor::validateOrAdjustPhysicalAngle(const int physicalAngleInDegrees) {
    if (physicalAngleInDegrees < MINIMUM_PHYSICAL_ANGLE_IN_DEGREES) {
        return MINIMUM_PHYSICAL_ANGLE_IN_DEGREES;
    }
    if (physicalAngleInDegrees > MAXIMUM_PHYSICAL_ANGLE_IN_DEGREES) {
        return MAXIMUM_PHYSICAL_ANGLE_IN_DEGREES;
    }
    return physicalAngleInDegrees;
}

ServoMotor::ServoMotor(const int signalGpioPin, const int initialAngleInDegrees, CommandHandler* parentHandler)
    : Actuator(signalGpioPin, parentHandler),
      currentActiveAngleInDegrees(-1),
      bufferedTargetAngleInDegrees(validateOrAdjustPhysicalAngle(initialAngleInDegrees)),
      requestedTargetAngleInDegrees(validateOrAdjustPhysicalAngle(initialAngleInDegrees)),
      pwmAttached(false),
      initializationResult(CommandExecutionResult::failed())
{
    pwmAttached = ledcAttach(pin, HARDWARE_PWM_FREQUENCY_IN_HERTZ, LEDC_PERIPHERAL_RESOLUTION_BITS);
    if (!pwmAttached) {
        return;
    }

    if (!writeAngle(requestedTargetAngleInDegrees)) {
        ledcDetach(pin);
        pwmAttached = false;
        return;
    }

    currentActiveAngleInDegrees = requestedTargetAngleInDegrees;
    initializationResult = CommandExecutionResult::succeeded();
}

ServoMotor::~ServoMotor() {
    if (pwmAttached) {
        ledcDetach(pin);
    }
}

void ServoMotor::setTargetAngleBufferInDegrees(const int targetAngleInDegrees) {
    bufferedTargetAngleInDegrees = validateOrAdjustPhysicalAngle(targetAngleInDegrees);
}

bool ServoMotor::writeAngle(const int angleInDegrees) const {
    const int clampedAngle = validateOrAdjustPhysicalAngle(angleInDegrees);

    static constexpr int pulseWidthRange = MAXIMUM_PULSE_WIDTH_IN_MICROSECONDS - MINIMUM_PULSE_WIDTH_IN_MICROSECONDS;
    const int duty = (clampedAngle * pulseWidthRange) / MAXIMUM_PHYSICAL_ANGLE_IN_DEGREES;
    const int scaledDuty = (duty * LEDC_PERIPHERAL_MAX_DUTY) / pulseWidthRange;

    return ledcWrite(pin, scaledDuty);
}

void ServoMotor::executeCommand(const Command command) {
    static_cast<void>(executeCommandChecked(command));
}

CommandExecutionResult ServoMotor::executeCommandChecked(const Command command) {
    int requestedAngle = -1;
    if (command == LOCK_TO_MINIMUM_LIMIT_COMMAND) {
        requestedAngle = MINIMUM_PHYSICAL_ANGLE_IN_DEGREES;
    }
    else if (command == LOCK_TO_MAXIMUM_LIMIT_COMMAND) {
        requestedAngle = MAXIMUM_PHYSICAL_ANGLE_IN_DEGREES;
    }
    else if (command == DRIVE_TO_BUFFERED_ANGLE_COMMAND) {
        requestedAngle = bufferedTargetAngleInDegrees;
    }
    else if (command.identifier == DRIVE_TO_ANGLE_COMMAND_IDENTIFIER) {
        requestedAngle = validateOrAdjustPhysicalAngle(command.payload);
    } else {
        return CommandExecutionResult::failed();
    }

    requestedTargetAngleInDegrees = requestedAngle;
    if (!pwmAttached || !writeAngle(requestedAngle)) {
        return CommandExecutionResult::failed();
    }

    currentActiveAngleInDegrees = requestedAngle;
    return CommandExecutionResult::succeeded();
}

int ServoMotor::getCurrentActiveAngleInDegrees() const { return currentActiveAngleInDegrees; }
int ServoMotor::getRequestedTargetAngleInDegrees() const { return requestedTargetAngleInDegrees; }
bool ServoMotor::isInitialized() const { return initializationResult.status == CommandExecutionStatus::Succeeded; }
CommandExecutionResult ServoMotor::getInitializationResult() const { return initializationResult; }