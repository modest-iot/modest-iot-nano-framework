#ifndef SERVO_MOTOR_H
#define SERVO_MOTOR_H

/**
 * @file ServoMotor.h
 * @brief Declares a servo actuator with bounded-angle control commands.
 * @ingroup actuators_module
 * @date 2026-06-06
 * @version 1.0
 */

#include "Actuator.h"
#include <ESP32Servo.h>

/**
 * @name Actuators: Motion Control
 * @ingroup actuators_module
 * @{
 */

/**
 * @brief Actuator adapter for hobby servo motors driven through ESP32 PWM.
 * @ingroup actuators_module
 */
class ServoMotor : public Actuator {
private:
    Servo embeddedServoDriver; ///< Underlying ESP32 servo driver instance.
    int currentActiveAngleInDegrees; ///< Currently active angle in degrees.       
    int bufferedTargetAngleInDegrees; ///< Buffered target angle in degrees.      

    static const int MINIMUM_PHYSICAL_ANGLE_IN_DEGREES = 0; ///< Minimum physical angle.
    static const int MAXIMUM_PHYSICAL_ANGLE_IN_DEGREES = 180; ///< Maximum physical angle.
    static const int MINIMUM_PULSE_WIDTH_IN_MICROSECONDS = 500; ///< Minimum pulse width in microseconds.
    static const int MAXIMUM_PULSE_WIDTH_IN_MICROSECONDS = 2400; ///< Maximum pulse width in microseconds.
    static const int HARDWARE_PWM_FREQUENCY_IN_HERTZ = 50; ///< PWM frequency in Hz.

    static const int PRIMARY_HARDWARE_TIMER_CHANNEL = 0; ///< Primary hardware timer channel.
    static const int SECONDARY_HARDWARE_TIMER_CHANNEL = 1; ///< Secondary hardware timer channel.

protected:
    /**
     * @brief Executes servo movement commands.
     * @param command Command to execute.
     */
    void executeCommand(Command command) override;

public:
    static const int LOCK_TO_MINIMUM_LIMIT_COMMAND_IDENTIFIER = 150;
    static const int LOCK_TO_MAXIMUM_LIMIT_COMMAND_IDENTIFIER = 151;
    static const int DRIVE_TO_BUFFERED_ANGLE_COMMAND_IDENTIFIER = 152;

    static const Command LOCK_TO_MINIMUM_LIMIT_COMMAND;
    static const Command LOCK_TO_MAXIMUM_LIMIT_COMMAND;
    static const Command DRIVE_TO_BUFFERED_ANGLE_COMMAND;

    /**
     * @brief Constructs a servo actuator.
     * @param signalGpioPin PWM-capable output pin for the servo signal wire.
     * @param initialAngleInDegrees Initial angle applied during setup.
     * @param parentHandler Optional upstream handler for propagated commands.
     */
    ServoMotor(int signalGpioPin, int initialAngleInDegrees = MINIMUM_PHYSICAL_ANGLE_IN_DEGREES, CommandHandler* parentHandler = nullptr);
    virtual ~ServoMotor() override;

    /**
     * @brief Stores a target angle used by `DRIVE_TO_BUFFERED_ANGLE_COMMAND`.
     * @param targetAngleInDegrees Target angle in degrees.
     */
    void setTargetAngleBufferInDegrees(int targetAngleInDegrees);

    /**
     * @brief Returns the currently applied servo angle.
     * @return Active servo angle in degrees.
     */
    int getCurrentActiveAngleInDegrees() const;
};

/** @} */ // End of Actuators: Motion Control

#endif // SERVO_MOTOR_H