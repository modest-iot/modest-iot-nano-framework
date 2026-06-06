#ifndef MODEST_IOT_H
#define MODEST_IOT_H

/**
 * @file ModestIoT.h
 * @brief Umbrella header that exposes the public framework API.
 *
 * Include this header from sketches to import all core primitives,
 * connectivity abstractions, and built-in sensor/actuator implementations.
 */

// --- 1. CORE ASYNCHRONOUS ENGINE & SIGNALING PRIMITIVES ---
#include "Event.h"
#include "Command.h"
#include "EventHandler.h"
#include "CommandHandler.h"
#include "TelemetryPackage.h"
#include "Device.h"

// --- 2. LAYER-ISOLATED NETWORK COMPREHENSION ---
#include "ConnectivityDriver.h"
#include "WiFiConnectivityDriver.h"
#include "HttpEdgeGatewayClient.h"
#include "MqttGatewayClient.h"

// --- 3. HARDWARE PERIPHERAL DRIVER SUBSYSTEMS ---
#include "Button.h"
#include "DhtSensor.h"
#include "UltrasonicSensor.h"
#include "PulseSensor.h"
#include "Led.h"
#include "CharacterLcdDisplay.h"
#include "ServoMotor.h"

#endif // MODEST_IOT_H