#ifndef BUTTON_H
#define BUTTON_H

#include "Sensor.h"

class Button : public Sensor {
private:
    unsigned long lastStableTimestamp;
    const unsigned long debounceDelayInMilliseconds;
    bool previousState;

protected:
    void processEvent(Event& event) override;

public:
    static const int BUTTON_PRESSED_EVENT_IDENTIFIER = 10;
    static const Event BUTTON_PRESSED_EVENT;

    Button(int signalGpioPin, unsigned long debounceDelayInMilliseconds = 50, EventHandler* parentHandler = nullptr);
    virtual ~Button() override = default;
};

#endif // BUTTON_H