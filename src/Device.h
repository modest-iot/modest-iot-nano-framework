#ifndef DEVICE_H
#define DEVICE_H

#include "EventHandler.h"
#include "CommandHandler.h"
#include "TelemetryPackage.h"
#include <Arduino.h>

struct hw_timer_s;
typedef struct hw_timer_s hw_timer_t;

class Device : public EventHandler, public CommandHandler {
private:
    struct ScheduledTaskSlot {
        EventHandler* sensorInstance;
        int targetEventIdentifier;
    };

    static const uint8_t MAXIMUM_SCHEDULED_SLOTS = 8;
    ScheduledTaskSlot scheduledSequences[MAXIMUM_SCHEDULED_SLOTS];
    uint8_t registeredSlotCount;
    uint8_t activeSequencePointer;

    TaskHandle_t hardwareSamplingTaskHandle;
    TaskHandle_t networkDispatcherTaskHandle;
    QueueHandle_t telemetryQueueHandle;

    hw_timer_t* hardwareTimer;
    const unsigned long pollingIntervalInMilliseconds;
    const uint8_t hardwareTimerIndex;

    static void executeHardwareSamplingContext(void* taskParameters);
    static void executeNetworkDispatchContext(void* taskParameters);
    static void IRAM_ATTR handleHardwareTimerInterrupt(void* deviceInstance);

protected:
    explicit Device(unsigned long samplingIntervalInMilliseconds, uint8_t timerGroupIndex = 0);
    virtual ~Device();

    void initializeAsynchronousEngine(uint8_t maximumQueueLength = 10);
    bool enqueueTelemetryPayload(TelemetryPackage** telemetryPackagePointerAddress);
    bool appendSensorToScheduler(EventHandler* targetSensorInstance, int requestedEventIdentifier);
    virtual void processQueuedTelemetryData(const TelemetryPackage* rawQueueItemPayload) = 0;
};

#endif // DEVICE_H