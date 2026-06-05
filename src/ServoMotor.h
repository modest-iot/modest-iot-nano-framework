#ifndef SERVO_MOTOR_H
#define SERVO_MOTOR_H

#include "Actuator.h"
#include <ESP32Servo.h>

class ServoMotor : public Actuator {
private:
    Servo embeddedServoDriver;
    int currentActiveAngleInDegrees;       
    int bufferedTargetAngleInDegrees;      

    static const int MINIMUM_PHYSICAL_ANGLE_IN_DEGREES = 0;
    static const int MAXIMUM_PHYSICAL_ANGLE_IN_DEGREES = 180;
    static const int MINIMUM_PULSE_WIDTH_IN_MICROSECONDS = 500;
    static const int MAXIMUM_PULSE_WIDTH_IN_MICROSECONDS = 2400;
    static const int HARDWARE_PWM_FREQUENCY_IN_HERTZ = 50;

    static const int PRIMARY_HARDWARE_TIMER_CHANNEL = 0;
    static const int SECONDARY_HARDWARE_TIMER_CHANNEL = 1;

protected:
    void executeCommand(Command command) override;

public:
    static const int LOCK_TO_MINIMUM_LIMIT_COMMAND_IDENTIFIER = 150;
    static const int LOCK_TO_MAXIMUM_LIMIT_COMMAND_IDENTIFIER = 151;
    static const int DRIVE_TO_BUFFERED_ANGLE_COMMAND_IDENTIFIER = 152;

    static const Command LOCK_TO_MINIMUM_LIMIT_COMMAND;
    static const Command LOCK_TO_MAXIMUM_LIMIT_COMMAND;
    static const Command DRIVE_TO_BUFFERED_ANGLE_COMMAND;

    ServoMotor(int signalGpioPin, int initialAngleInDegrees = MINIMUM_PHYSICAL_ANGLE_IN_DEGREES, CommandHandler* parentHandler = nullptr);
    virtual ~ServoMotor() override;

    void setTargetAngleBufferInDegrees(int targetAngleInDegrees);
    int getCurrentActiveAngleInDegrees() const;
};

#endif // SERVO_MOTOR_H