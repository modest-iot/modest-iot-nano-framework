#ifndef SERVO_MOTOR_H
#define SERVO_MOTOR_H

/**
 * @file ServoMotor.h
 * @brief Declares a servo actuator with bounded-angle control commands.
 * @ingroup actuators_module
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-06-06
 * @version 1.0
 */

#include "Actuator.h"

/**
 * @brief Actuator adapter for hobby servo motors driven through ESP32 PWM.
 * @ingroup actuators_module
 *
 * A successful LEDC result confirms that the driver accepted a duty update; it does not
 * measure or guarantee physical servo movement.
 */
class ServoMotor : public Actuator {
private:
    int currentActiveAngleInDegrees; ///< Last angle accepted by the PWM driver, or -1 if unknown.
    int bufferedTargetAngleInDegrees; ///< Buffered target angle in degrees.
    int requestedTargetAngleInDegrees; ///< Most recently requested angle, independent of driver acceptance.
    bool pwmAttached; ///< Whether LEDC attachment succeeded.
    CommandExecutionResult initializationResult; ///< Result of PWM attachment and initial output setup.

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
     * @return `true` when the LEDC driver accepts the duty update.
     * @runs_on{sampler}
     */
    [[nodiscard]] bool writeAngle(int angleInDegrees) const;

protected:
    /**
     * @brief Executes servo movement commands.
     * @param command Command to execute.
     */
    void executeCommand(Command command) override;
    CommandExecutionResult executeCommandChecked(Command command) override;

public:
    /** @brief Command identifier for locking the servo to the minimum angle (0°). */
    static constexpr int LOCK_TO_MINIMUM_LIMIT_COMMAND_IDENTIFIER = 150;
    /** @brief Command identifier for locking the servo to the maximum angle (180°). */
    static constexpr int LOCK_TO_MAXIMUM_LIMIT_COMMAND_IDENTIFIER = 151;
    /** @brief Command identifier for driving the servo to the buffered target angle. */
    static constexpr int DRIVE_TO_BUFFERED_ANGLE_COMMAND_IDENTIFIER = 152;
    /** @brief Command identifier for driving the servo to the payload angle directly. */
    static constexpr int DRIVE_TO_ANGLE_COMMAND_IDENTIFIER = 153;

    /** @brief Prebuilt command instance for locking to the minimum angle. */
    static constexpr auto LOCK_TO_MINIMUM_LIMIT_COMMAND = Command(LOCK_TO_MINIMUM_LIMIT_COMMAND_IDENTIFIER);
    /** @brief Prebuilt command instance for locking to the maximum angle. */
    static constexpr auto LOCK_TO_MAXIMUM_LIMIT_COMMAND = Command(LOCK_TO_MAXIMUM_LIMIT_COMMAND_IDENTIFIER);
    /** @brief Prebuilt command instance for driving to the buffered target angle. */
    static constexpr auto DRIVE_TO_BUFFERED_ANGLE_COMMAND = Command(DRIVE_TO_BUFFERED_ANGLE_COMMAND_IDENTIFIER);

    /**
     * @brief Constructs a servo actuator.
     * @param signalGpioPin PWM-capable output pin for the servo signal wire.
     * @param initialAngleInDegrees Initial PWM target requested during setup.
     * @param parentHandler Optional upstream handler for propagated commands.
     */
    explicit ServoMotor(int signalGpioPin, int initialAngleInDegrees = MINIMUM_PHYSICAL_ANGLE_IN_DEGREES, CommandHandler* parentHandler = nullptr);
    ~ServoMotor() override;

    /**
     * @brief Reports whether LEDC attachment and the initial duty write succeeded.
     * @return `true` only when both driver setup operations were accepted.
     */
    [[nodiscard]] bool isInitialized() const;

    /**
     * @brief Returns the outcome of PWM attachment and initial output setup.
     * @return Driver-level setup result; does not confirm physical servo movement.
     */
    [[nodiscard]] CommandExecutionResult getInitializationResult() const;

    /**
     * @brief Clamps a physical angle to the valid range (0–180°).
     * @param physicalAngleInDegrees Angle in degrees to validate.
     * @return Clamped angle within [MINIMUM_PHYSICAL_ANGLE_IN_DEGREES, MAXIMUM_PHYSICAL_ANGLE_IN_DEGREES].
     */
    [[nodiscard]] static int validateOrAdjustPhysicalAngle(int physicalAngleInDegrees);

    /**
     * @brief Stores a target angle used by `DRIVE_TO_BUFFERED_ANGLE_COMMAND`.
     * @param targetAngleInDegrees Target angle in degrees.
     */
    void setTargetAngleBufferInDegrees(int targetAngleInDegrees);

    /**
     * @brief Returns the last angle whose PWM duty update was accepted by LEDC.
     * @return Accepted target angle in degrees, or -1 if no output has been accepted.
     */
    [[nodiscard]] int getCurrentActiveAngleInDegrees() const;

    /**
     * @brief Returns the most recently requested target angle.
     * @return Clamped requested target angle in degrees; this is not measured position.
     */
    [[nodiscard]] int getRequestedTargetAngleInDegrees() const;
};

#endif // SERVO_MOTOR_H