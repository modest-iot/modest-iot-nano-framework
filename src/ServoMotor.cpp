/**
 * @file ServoMotor.cpp
 * @brief Implements bounded-angle servo control and buffered-angle command execution.
 * @date 2026-06-06
 * @version 1.0
 */

#include "ServoMotor.h"


ServoMotor::ServoMotor(int signalGpioPin, int initialAngleInDegrees, CommandHandler* parentHandler)
    : Actuator(signalGpioPin, parentHandler),
      currentActiveAngleInDegrees(initialAngleInDegrees),
      bufferedTargetAngleInDegrees(initialAngleInDegrees) 
{
    ESP32PWM::allocateTimer(PRIMARY_HARDWARE_TIMER_CHANNEL);
    ESP32PWM::allocateTimer(SECONDARY_HARDWARE_TIMER_CHANNEL);

    embeddedServoDriver.setPeriodHertz(HARDWARE_PWM_FREQUENCY_IN_HERTZ);
    embeddedServoDriver.attach(pin, MINIMUM_PULSE_WIDTH_IN_MICROSECONDS, MAXIMUM_PULSE_WIDTH_IN_MICROSECONDS);
    embeddedServoDriver.write(currentActiveAngleInDegrees);
}

ServoMotor::~ServoMotor() {
    embeddedServoDriver.detach();
}

void ServoMotor::setTargetAngleBufferInDegrees(int targetAngleInDegrees) {
    if (targetAngleInDegrees < MINIMUM_PHYSICAL_ANGLE_IN_DEGREES) {
        bufferedTargetAngleInDegrees = MINIMUM_PHYSICAL_ANGLE_IN_DEGREES;
    } else if (targetAngleInDegrees > MAXIMUM_PHYSICAL_ANGLE_IN_DEGREES) {
        bufferedTargetAngleInDegrees = MAXIMUM_PHYSICAL_ANGLE_IN_DEGREES;
    } else {
        bufferedTargetAngleInDegrees = targetAngleInDegrees;
    }
}

void ServoMotor::executeCommand(Command command) {
    if (command == LOCK_TO_MINIMUM_LIMIT_COMMAND) {
        currentActiveAngleInDegrees = MINIMUM_PHYSICAL_ANGLE_IN_DEGREES;
        embeddedServoDriver.write(currentActiveAngleInDegrees);
    } 
    else if (command == LOCK_TO_MAXIMUM_LIMIT_COMMAND) {
        currentActiveAngleInDegrees = MAXIMUM_PHYSICAL_ANGLE_IN_DEGREES;
        embeddedServoDriver.write(currentActiveAngleInDegrees);
    } 
    else if (command == DRIVE_TO_BUFFERED_ANGLE_COMMAND) {
        currentActiveAngleInDegrees = bufferedTargetAngleInDegrees;
        embeddedServoDriver.write(currentActiveAngleInDegrees);
    }
}

int ServoMotor::getCurrentActiveAngleInDegrees() const { return currentActiveAngleInDegrees; }