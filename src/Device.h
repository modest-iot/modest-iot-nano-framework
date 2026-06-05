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

    // FreeRTOS OS Kernel Handles
    TaskHandle_t hardwareSamplingTaskHandle;
    TaskHandle_t networkDispatcherTaskHandle;
    QueueHandle_t telemetryQueueHandle;

    // Encapsulated Processor Clock Attributes
    hw_timer_t* hardwareTimer;
    const unsigned long pollingIntervalInMilliseconds;

    // Dynamic asset token prevents multi-instance silicon register crashes
    const uint8_t hardwareTimerIndex;

    static void executeHardwareSamplingContext(void* taskParameters);
    static void executeNetworkDispatchContext(void* taskParameters);

    /**
     * @brief Static Interrupt Service Routine bridge mapping physical silicon signals
     * back into the active object instance state.
     * ALIGNED: Parameter name context redundancy cleaned (deviceInstanceInstance -> deviceInstance)
     */
    static void IRAM_ATTR handleHardwareTimerInterrupt(void* deviceInstance);

protected:
    /**
     * @param samplingIntervalInMilliseconds The targeted scheduling sequence evaluation rate.
     * @param timerGroupIndex The hardware timer ID allocated to this device (0 to 3 on ESP32).
     */
    explicit Device(unsigned long samplingIntervalInMilliseconds, uint8_t timerGroupIndex = 0);
    virtual ~Device();

    void initializeAsynchronousEngine(uint8_t maximumQueueLength = 10);
    bool enqueueTelemetryPayload(TelemetryPackage** telemetryPackagePointerAddress);
    bool appendSensorToScheduler(EventHandler* targetSensorInstance, int requestedEventIdentifier);
    virtual void processQueuedTelemetryData(const TelemetryPackage* rawQueueItemPayload) = 0;
};

#endif // DEVICE_H