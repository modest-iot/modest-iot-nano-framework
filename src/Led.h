#ifndef LED_H
#define LED_H

#include "Actuator.h"

class Led : public Actuator {
private:
    bool state;

protected:
    void executeCommand(Command command) override;

public:
    static const int TURN_ON_COMMAND_IDENTIFIER = 100;
    static const int TURN_OFF_COMMAND_IDENTIFIER = 101;
    static const int TOGGLE_LED_COMMAND_IDENTIFIER = 102;

    static const Command TURN_ON_COMMAND;
    static const Command TURN_OFF_COMMAND;
    static const Command TOGGLE_LED_COMMAND;

    Led(int signalGpioPin, bool initialState = false, CommandHandler* parentHandler = nullptr);
    bool isActive() const;
};

#endif // LED_H