#include <cassert>
#include <Arduino.h>
#include "ServoMotor.h"

namespace {
struct Recorder : CommandHandler {
    int count = 0;
    void handle(const Command) override { count++; }
};
} // namespace

[[nodiscard]] int main() {
    // Test 1: Constructor configures LEDC pin with correct parameters
    Recorder parent;
    ServoMotor servo(13, 0, &parent);
    assert(fakeLedcFreq[13] == 50);
    assert(fakeLedcPin[13] == 13);
    assert(servo.getCurrentActiveAngleInDegrees() == 0);

    // Test 2: LOCK_TO_MINIMUM_LIMIT_COMMAND sets angle to 0°
    servo.handle(ServoMotor::LOCK_TO_MINIMUM_LIMIT_COMMAND);
    assert(servo.getCurrentActiveAngleInDegrees() == 0);
    assert(fakeLedcDuty[13] == 0);

    // Test 3: LOCK_TO_MAXIMUM_LIMIT_COMMAND sets angle to 180°
    servo.handle(ServoMotor::LOCK_TO_MAXIMUM_LIMIT_COMMAND);
    assert(servo.getCurrentActiveAngleInDegrees() == 180);
    assert(fakeLedcDuty[13] == 8191);

    // Test 4: DRIVE_TO_BUFFERED_ANGLE_COMMAND sets angle to buffered target
    servo.setTargetAngleBufferInDegrees(90);
    servo.handle(ServoMotor::DRIVE_TO_BUFFERED_ANGLE_COMMAND);
    assert(servo.getCurrentActiveAngleInDegrees() == 90);
    assert(fakeLedcDuty[13] == 4095);

    // Test 5: Angle clamping — negative values clamp to 0°
    servo.setTargetAngleBufferInDegrees(-10);
    servo.handle(ServoMotor::DRIVE_TO_BUFFERED_ANGLE_COMMAND);
    assert(servo.getCurrentActiveAngleInDegrees() == 0);
    assert(fakeLedcDuty[13] == 0);

    // Test 6: Angle clamping — values above 180° clamp to 180°
    servo.setTargetAngleBufferInDegrees(200);
    servo.handle(ServoMotor::DRIVE_TO_BUFFERED_ANGLE_COMMAND);
    assert(servo.getCurrentActiveAngleInDegrees() == 180);
    assert(fakeLedcDuty[13] == 8191);

    // Test 7: Unknown command does not change state but still propagates
    servo.handle(Command{999});
    assert(servo.getCurrentActiveAngleInDegrees() == 180);

    // Test 8: Parent handler receives all propagated commands (including unknown)
    assert(parent.count == 6);

    // Test 9: Constructor with non-zero initial angle
    ServoMotor servo2(14, 45);
    assert(servo2.getCurrentActiveAngleInDegrees() == 45);

    return 0;
}
