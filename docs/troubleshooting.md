# Troubleshooting Guide

This guide covers common issues when using the Modest IoT Nano Framework with an ESP32 board. Each problem is organized by symptom, cause, and solution.

---

## Serial Output Issues

### No output in the Serial Monitor

**Symptom**: The Serial Monitor is blank after uploading the sketch.

**Causes & Solutions**:

| Cause | Solution |
|---|---|
| **Baud rate mismatch** | The framework uses **115200 baud** by default. Ensure the Serial Monitor is set to 115200. |
| **Wrong port selected** | Check the correct port: `/dev/ttyUSB0` (Linux), `/dev/cu.usbserial-*` (macOS), `COM3` (Windows). |
| **Board not resetting** | Press the board's RST button manually, then reopen the Serial Monitor. |
| **Garbage characters** | These appear briefly during board reset. Wait 1-2 seconds for clean output to begin. |

### Only warning and error messages appear

**Symptom**: You see `[W]` and `[E]` messages but no `[I]` or `[D]` messages.

**Cause**: The framework's default log level is `MODEST_LOG_LEVEL_WARN`. Info and debug messages are compiled out.

**Solution**: To enable verbose logging, add this before including `ModestIoT.h`:

```cpp
#define MODEST_LOG_LEVEL MODEST_LOG_LEVEL_DEBUG
#include <ModestIoT.h>
```

Log levels from most to least verbose: `VERBOSE` (5), `DEBUG` (4), `INFO` (3), `WARN` (2), `ERROR` (1), `NONE` (0).

---

## Board Detection Issues

### Board not detected by the IDE

**Symptom**: The IDE shows no available ports or the correct port is missing.

**Causes & Solutions**:

| Cause | Solution |
|---|---|
| **Missing USB driver** | Install the driver for your board's USB chip: **CP2102** or **CH340**. Download from the manufacturer's website. |
| **Faulty USB cable** | Use a data-capable USB cable, not a charge-only cable. Try a different cable. |
| **Board in bootloader mode** | Press the BOOT button twice to exit bootloader mode, then reconnect. |
| **Port conflict** | Close other programs that may use the serial port (e.g., PuTTY, another IDE instance). |

### Wrong board selected

**Symptom**: Upload fails with "stubrunner_ulb_upload_bin failed" or similar errors.

**Solution**: Select the correct board model:

| Board | Arduino IDE Board Selection |
|---|---|
| ESP32-DevKitC | ESP32 Dev Module |
| ESP32-S3-DevKitC | ESP32S3 Dev Module |
| ESP32-C3-DevKitC | ESP32C3 Dev Module |
| ESP32-WROVER | ESP32 Dev Module |

---

## Upload Failures

### Upload fails with "Connecting..." timeout

**Symptom**: The build succeeds but the upload hangs at "Connecting..." for 30+ seconds.

**Causes & Solutions**:

| Cause | Solution |
|---|---|
| **Auto-reset not working** | Hold the BOOT button, click Upload, then release BOOT when you see "Connecting...". |
| **DTR/RTS lines not connected** | Some cheap boards have broken auto-reset circuitry. Use manual BOOT button method above. |
| **Wrong baud rate** | In Arduino IDE, go to **File > Preferences** and set "Upload speed" to **921600** (or try 115200). |

### Upload succeeds but sketch doesn't run

**Symptom**: Upload completes successfully, but the Serial Monitor shows no output.

**Solution**: The board may need a manual reset. Press the RST button once. If the sketch still doesn't run, check:

- The `setup()` function is not blocked (e.g., waiting for Wi-Fi without a timeout)
- The correct GPIO pins are configured for your board
- The onboard LED pin matches your board's hardware

---

## Wi-Fi Connectivity Issues

### Wi-Fi connection fails

**Symptom**: Serial output shows repeated Wi-Fi connection attempts or timeout messages.

**Causes & Solutions**:

| Cause | Solution |
|---|---|
| **Wrong SSID or password** | Verify the credentials in your sketch match your router exactly (case-sensitive). |
| **Router on 5 GHz only** | ESP32 only supports **2.4 GHz** Wi-Fi. Ensure your router broadcasts a 2.4 GHz network. |
| **WPA3 encryption** | ESP32 may not support WPA3. Use WPA2 Personal encryption instead. |
| **Too many connection attempts** | The `WiFiConnectivityDriver` has built-in reconnection logic. Wait for it to retry automatically. |

### Wi-Fi disconnects frequently

**Symptom**: The device connects initially but drops the connection after a few minutes.

**Causes & Solutions**:

| Cause | Solution |
|---|---|
| **Weak signal** | Move the device closer to the router or add a Wi-Fi extender. |
| **Power saving mode** | The ESP32 may enter light sleep. Check that the framework's network task is calling `gatewayClient.service()` periodically. |
| **Router client limit** | Some routers limit the number of connected devices. Check your router settings. |

---

## BLE Issues

### BLE device not discoverable

**Symptom**: The BLE gateway example runs but no phone or computer can find the device.

**Causes & Solutions**:

| Cause | Solution |
|---|---|
| **Advertising not started** | Ensure `BleConnectivityDriver::startAdvertising()` is called in `setup()`. |
| **Wrong BLE name** | The device name in your sketch must match what you're searching for on the phone. |
| **Phone BLE cache** | Clear the BLE cache on your phone and try again. |

### BLE connection drops

**Symptom**: The device connects via BLE but the connection drops after a few seconds.

**Causes & Solutions**:

| Cause | Solution |
|---|---|
| **Notification not enabled** | Ensure the phone app enables notifications on the correct characteristic. |
| **MTU too small** | The default MTU may be too small for large payloads. Request a larger MTU from the phone side. |
| **Interrupt conflicts** | If using interrupts on the same GPIO as BLE, move the interrupt to a different pin. |

### BLE API compile errors

**Symptom**: A BLE example fails to compile with missing BLE headers or API/signature errors.

**Causes & Solutions**:

| Cause | Solution |
|---|---|
| **Non-ESP32 board selected** | Select a supported ESP32 board; the BLE driver and `BleHal` target ESP32 BLE APIs. |
| **Unsupported or missing ESP32 core headers** | Install an ESP32 Arduino core from the supported 3.x or 4.x series and rebuild. The HAL uses the unified BLE API on core 4.x and the separate BLE headers on core 3.x. |
| **BLE stack unavailable in the board configuration** | Verify that the selected board package enables its BLE stack and that the build is using that package's headers. |

---

## MQTT Connectivity Issues

### Cannot connect to MQTT broker

**Symptom**: Serial output shows MQTT connection errors or the device never publishes telemetry.

**Causes & Solutions**:

| Cause | Solution |
|---|---|
| **Wrong broker address** | Verify the IP address and port (default 1883) in your sketch. |
| **Firewall blocking** | Ensure the broker's port is accessible from the ESP32's network. |
| **Wi-Fi not connected** | MQTT requires Wi-Fi. Ensure Wi-Fi connects successfully before MQTT initialization. |
| **Client ID conflict** | Each device must have a unique client ID. Change the client ID in `MqttGatewayClient` constructor. |

### MQTT messages not received

**Symptom**: The device publishes telemetry but the broker or subscriber doesn't receive messages.

**Causes & Solutions**:

| Cause | Solution |
|---|---|
| **Wrong topic** | Verify the publish topic in your sketch matches the subscribe topic on the broker. |
| **QoS mismatch** | The framework uses QoS 0 by default. Ensure the broker accepts QoS 0 messages. |
| **Payload too large** | Check that the JSON payload fits within the MQTT message size limit (typically 256 KB). |

---

## Memory Issues

### Stack overflow

**Symptom**: The device crashes with a Guru Meditation Error or Watchdog Timer reset.

**Causes & Solutions**:

| Cause | Solution |
|---|---|
| **Large local arrays** | Move large buffers from stack to static or global scope. |
| **Deep recursion** | Avoid recursive functions; use iterative approaches instead. |
| **Task stack too small** | The framework uses default FreeRTOS stack sizes. Increase them if needed via `initializeAsynchronousEngine()`. |

**Diagnosis**: Check stack usage with high-water marks:

```cpp
Serial.printf("Sampler stack high water: %lu\n", uxTaskGetStackHighWaterMark(NULL));
```

A value below 50 indicates the stack is too small.

### Heap exhaustion

**Symptom**: The device works initially but crashes after running for hours.

**Causes & Solutions**:

| Cause | Solution |
|---|---|
| **Memory leaks** | Avoid `new` without matching `delete`. The framework uses `std::unique_ptr` for telemetry packages. |
| **String concatenation** | Use `F()` macro for string literals to keep them in Flash: `Serial.println(F("message"));` |
| **Too many telemetry packages** | If the telemetry queue fills, packets are dropped. Increase the queue depth in `initializeAsynchronousEngine()`. |

**Diagnosis**: Monitor free heap:

```cpp
Serial.printf("Free heap: %lu bytes\n", ESP.getFreeHeap());
```

---

## Sensor Issues

### Sensor returns zero or stale values

**Symptom**: The sensor reads 0, the same value repeatedly, or returns invalid data.

**Causes & Solutions**:

| Cause | Solution |
|---|---|
| **Wrong GPIO pin** | Verify the pin number matches your wiring. Check the pin is not reserved for other functions. |
| **I2C bus conflict** | If multiple I2C devices share the bus, ensure each has a unique address. |
| **Sensor not initialized** | The framework initializes sensors in the `Device` constructor. Ensure the sensor is registered with `appendSensorToScheduler()`. |
| **Power issues** | Some sensors require 3.3V, not 5V. Check the sensor's voltage requirements. |

### Sensor fault events

**Symptom**: The framework emits sensor fault events or the sensor stops reporting.

**Causes & Solutions**:

| Cause | Solution |
|---|---|
| **I2C timeout** | The sensor didn't respond within the timeout period. Check wiring and pull-up resistors. |
| **One-wire protocol error** | DHT sensors are timing-sensitive. Ensure no other code delays the single-wire bus. |
| **Analog pin saturation** | The reading is at 0 or 4095 (full scale). Check the sensor's output voltage range. |

---

## Scheduler Issues

### "Scheduler capacity full" error

**Symptom**: Serial output shows `[ERROR] ... registration failed: Scheduler capacity full!`

**Cause**: The sampling scheduler has a fixed maximum number of slots. All slots are already occupied.

**Solution**: Check how many sensors are registered. The default scheduler size is limited; reduce the number of sensors or increase the scheduler capacity in the framework configuration.

### Sensor period clamped

**Symptom**: Serial output shows a warning about the sensor period being clamped.

**Cause**: The requested sampling interval is below the minimum period the scheduler can support.

**Solution**: Increase the sampling interval or reduce the number of sensors to allow longer minimum periods.

---

## Telemetry Queue Issues

### "First packet dropped: telemetry queue full" warning

**Symptom**: Serial output shows `[W] [Device] First packet dropped: telemetry queue full`.

**Cause**: The telemetry queue is full because the network task cannot send data fast enough.

**Solutions**:

| Solution | When to Use |
|---|---|
| **Increase queue depth** | Pass a larger value to `initializeAsynchronousEngine()`. |
| **Reduce sampling rate** | Increase the interval between sensor readings. |
| **Check network connectivity** | Ensure Wi-Fi or BLE is connected and the gateway is functioning. |

---

## Debugging Tips

### Enable verbose logging

To see all framework internal messages, add this before `#include <ModestIoT.h>`:

```cpp
#define MODEST_LOG_LEVEL MODEST_LOG_LEVEL_VERBOSE
#include <ModestIoT.h>
```

### Check FreeRTOS task health

Add this to your `loop()` or a periodic event handler to monitor task health:

```cpp
Serial.printf("Free heap: %lu | Sampler stack: %lu\n",
    ESP.getFreeHeap(),
    uxTaskGetStackHighWaterMark(NULL));
```

### Verify event flow

The framework's event flow is:

1. **Hardware timer fires** → event placed in queue
2. **Sampler task processes** → calls `on(Event)` handler
3. **Device routes** → sensors measure, actuators command
4. **Network task** → sends telemetry via MQTT or BLE

If any step is missing, check the serial output for the corresponding log messages.

---

## Getting More Help

If your issue is not covered here:

1. Check the [examples](../examples/) directory for working code patterns.
2. Review the [Architecture Decision Records](./adrs.md) for design rationale.
3. Search the [GitHub Issues](https://github.com/angelvelasquez/Modest-IoT-Nano-Framework/issues) for similar problems.
4. File a new issue with: board model, framework version, serial output, and a minimal reproducing sketch.
