#ifndef MODEST_IOT_H
#define MODEST_IOT_H

/**
 * @file ModestIoT.h
 * @brief Umbrella header that exposes the public framework API.
 * @ingroup core_module
 *
 * Include this header from sketches to import all core primitives,
 * connectivity abstractions, and built-in sensor/actuator implementations.
 * @date 2026-06-06
 * @version 1.0
 */

/// --- 1. CORE ASYNCHRONOUS ENGINE & SIGNALING PRIMITIVES ---
#include "ModestLog.h"
#include "Event.h"
#include "Command.h"
#include "EventHandler.h"
#include "CommandHandler.h"
#include "TelemetryPackage.h"
#include "Device.h"
#include "LocationData.h"

/// --- 2. LAYER-ISOLATED NETWORK COMPREHENSION ---
#include "ConnectivityDriver.h"
#include "WiFiConnectivityDriver.h"
#include "BleConnectivityDriver.h"
#include "HttpGatewayClient.h"
#include "MqttGatewayClient.h"

/// --- 3. HARDWARE PERIPHERAL DRIVER SUBSYSTEMS ---
#include "Button.h"
#include "DhtSensor.h"
#include "UltrasonicSensor.h"
#include "PulseSensor.h"
#include "Led.h"
#include "RgbLed.h"
#include "Hd44780Driver.h"
#include "CharacterLcdDisplay.h"
#include "ServoMotor.h"
#include "GasSensor.h"
#include "Buzzer.h"
#include "GeolocationSensor.h"
#include "LoadCellAmplifier.h"
#include "PassiveInfraredSensor.h"
#include "DigitalLightIntensitySensor.h"
#include "RfidSensor.h"
#include "RelayActuator.h"
#include "SoilMoistureSensor.h"
#include "WaterFlowSensor.h"
#include "BmeEnvironmentalSensor.h"
#include "PthSensor.h"
#include "WaterLeakSensor.h"
#include "DoorWindowSensor.h"
#include "OneWireTemperatureSensor.h"
#include "EnvironmentalGasSensor.h"
#include "Ssd1306Driver.h"
#include "OledDisplay.h"

#endif // MODEST_IOT_H