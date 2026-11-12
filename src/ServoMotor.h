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

/**
 * @brief Actuator adapter for hobby servo motors driven through ESP32 PWM.
 * @ingroup actuators_module
 */
class ServoMotor : public Actuator {
private:
    int currentActiveAngleInDegrees; ///< Currently active angle in degrees.       
    int bufferedTargetAngleInDegrees; ///< Buffered target angle in degrees.      

    static constexpr int MINIMUM_PHYSICAL_ANGLE_IN_DEGREES = 0; ///< Minimum physical angle.
    static constexpr int MAXIMUM_PHYSICAL_ANGLE_IN_DEGREES = 180; ///< Maximum physical angle.
    static constexpr int MINIMUM_PULSE_WIDTH_IN_MICROSECONDS = 500; ///< Minimum pulse width in microseconds.
    static constexpr int MAXIMUM_PULSE_WIDTH_IN_MICROSECONDS = 2400; ///< Maximum pulse width in microseconds.
    static constexpr int HARDWARE_PWM_FREQUENCY_IN_HERTZ = 50; ///< PWM frequency in Hz.

    static constexpr int PRIMARY_HARDWARE_TIMER_CHANNEL = 0; ///< Primary hardware timer channel.
    static constexpr int SECONDARY_HARDWARE_TIMER_CHANNEL = 1; ///< Secondary hardware timer channel.

    /**
     * @brief Converts an angle to a duty cycle and writes it to the LEDC peripheral.
     * @param angleInDegrees Angle in degrees (clamped to 0-180).
     * @runs_on{sampler}
     */
    void writeAngle(int angleInDegrees) const;

protected:
    /**
     * @brief Executes servo movement commands.
     * @param command Command to execute.
     */
    void executeCommand(Command command) override;

public:
    /** @brief Command identifier for locking the servo to the minimum angle (0°). */
    static constexpr int LOCK_TO_MINIMUM_LIMIT_COMMAND_IDENTIFIER = 150;
    /** @brief Command identifier for locking the servo to the maximum angle (180°). */
    static constexpr int LOCK_TO_MAXIMUM_LIMIT_COMMAND_IDENTIFIER = 151;
    /** @brief Command identifier for driving the servo to the buffered target angle. */
    static constexpr int DRIVE_TO_BUFFERED_ANGLE_COMMAND_IDENTIFIER = 152;

    /** @brief Prebuilt command instance for locking to the minimum angle. */
    static constexpr auto LOCK_TO_MINIMUM_LIMIT_COMMAND = Command(LOCK_TO_MINIMUM_LIMIT_COMMAND_IDENTIFIER);
    /** @brief Prebuilt command instance for locking to the maximum angle. */
    static constexpr auto LOCK_TO_MAXIMUM_LIMIT_COMMAND = Command(LOCK_TO_MAXIMUM_LIMIT_COMMAND_IDENTIFIER);
    /** @brief Prebuilt command instance for driving to the buffered target angle. */
    static constexpr auto DRIVE_TO_BUFFERED_ANGLE_COMMAND = Command(DRIVE_TO_BUFFERED_ANGLE_COMMAND_IDENTIFIER);

    /**
     * @brief Constructs a servo actuator.
     * @param signalGpioPin PWM-capable output pin for the servo signal wire.
     * @param initialAngleInDegrees Initial angle applied during setup.
     * @param parentHandler Optional upstream handler for propagated commands.
     */
    explicit ServoMotor(int signalGpioPin, int initialAngleInDegrees = MINIMUM_PHYSICAL_ANGLE_IN_DEGREES, CommandHandler* parentHandler = nullptr);
    ~ServoMotor() override;

    /**
     * @brief Stores a target angle used by `DRIVE_TO_BUFFERED_ANGLE_COMMAND`.
     * @param targetAngleInDegrees Target angle in degrees.
     */
    void setTargetAngleBufferInDegrees(int targetAngleInDegrees);

    /**
     * @brief Returns the currently applied servo angle.
     * @return Active servo angle in degrees.
     */
    [[nodiscard]] int getCurrentActiveAngleInDegrees() const;
};

#endif // SERVO_MOTOR_H