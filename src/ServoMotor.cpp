/**
 * @file ServoMotor.cpp
 * @brief Implements bounded-angle servo control and buffered-angle command execution
 *        using the ESP32 LEDC peripheral for native PWM output.
 * @date 2026-06-06
 * @version 1.0
 */

#include "ServoMotor.h"
#include <Arduino.h>


static constexpr int LEDC_RESOLUTION_BITS = 13;
static constexpr int LEDC_MAX_DUTY = (1 << LEDC_RESOLUTION_BITS) - 1;

ServoMotor::ServoMotor(const int signalGpioPin, const int initialAngleInDegrees, CommandHandler* parentHandler)
    : Actuator(signalGpioPin, parentHandler),
      currentActiveAngleInDegrees(initialAngleInDegrees),
      bufferedTargetAngleInDegrees(initialAngleInDegrees)
{
    ledcAttach(pin, HARDWARE_PWM_FREQUENCY_IN_HERTZ, LEDC_RESOLUTION_BITS);
    writeAngle(currentActiveAngleInDegrees);
}

ServoMotor::~ServoMotor() {
    ledcDetach(pin);
}

void ServoMotor::setTargetAngleBufferInDegrees(const int targetAngleInDegrees) {
    if (targetAngleInDegrees < MINIMUM_PHYSICAL_ANGLE_IN_DEGREES) {
        bufferedTargetAngleInDegrees = MINIMUM_PHYSICAL_ANGLE_IN_DEGREES;
    } else if (targetAngleInDegrees > MAXIMUM_PHYSICAL_ANGLE_IN_DEGREES) {
        bufferedTargetAngleInDegrees = MAXIMUM_PHYSICAL_ANGLE_IN_DEGREES;
    } else {
        bufferedTargetAngleInDegrees = targetAngleInDegrees;
    }
}

void ServoMotor::writeAngle(const int angleInDegrees) const {
    int clampedAngle = angleInDegrees;
    if (clampedAngle < MINIMUM_PHYSICAL_ANGLE_IN_DEGREES) {
        clampedAngle = MINIMUM_PHYSICAL_ANGLE_IN_DEGREES;
    } else if (clampedAngle > MAXIMUM_PHYSICAL_ANGLE_IN_DEGREES) {
        clampedAngle = MAXIMUM_PHYSICAL_ANGLE_IN_DEGREES;
    }

    static constexpr int pulseWidthRange = MAXIMUM_PULSE_WIDTH_IN_MICROSECONDS - MINIMUM_PULSE_WIDTH_IN_MICROSECONDS;
    const int duty = (clampedAngle * pulseWidthRange) / MAXIMUM_PHYSICAL_ANGLE_IN_DEGREES;
    const int scaledDuty = (duty * LEDC_MAX_DUTY) / pulseWidthRange;

    ledcWrite(pin, scaledDuty);
}

void ServoMotor::executeCommand(const Command command) {
    if (command == LOCK_TO_MINIMUM_LIMIT_COMMAND) {
        currentActiveAngleInDegrees = MINIMUM_PHYSICAL_ANGLE_IN_DEGREES;
        writeAngle(currentActiveAngleInDegrees);
    }
    else if (command == LOCK_TO_MAXIMUM_LIMIT_COMMAND) {
        currentActiveAngleInDegrees = MAXIMUM_PHYSICAL_ANGLE_IN_DEGREES;
        writeAngle(currentActiveAngleInDegrees);
    }
    else if (command == DRIVE_TO_BUFFERED_ANGLE_COMMAND) {
        currentActiveAngleInDegrees = bufferedTargetAngleInDegrees;
        writeAngle(currentActiveAngleInDegrees);
    }
}

int ServoMotor::getCurrentActiveAngleInDegrees() const { return currentActiveAngleInDegrees; }