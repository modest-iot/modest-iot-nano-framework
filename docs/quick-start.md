# Quick Start Guide

Get your first Modest IoT Nano Framework device running in under 10 minutes. This guide walks you through the `StatusIndicator` example — the simplest entry point with no sensors or network required.

Choose the route that fits your situation:

- **Route A: Simulation** — Try the framework in your browser with Wokwi, no hardware needed.
- **Route B: Hardware** — Run the framework on a physical ESP32 board.

---

## Route A: Simulation (Wokwi)

Try the framework without any hardware. Wokwi simulates an ESP32 board directly in your browser.

### Prerequisites

- **Web browser** (Chrome, Firefox, Safari, or Edge)
- **Free Wokwi account** at [wokwi.com](https://wokwi.com)

### Get the Example

1. Open [Wokwi](https://wokwi.com) and sign in.
2. Click **New Project**, select **ESP32** as the board.
   - **What you'll see**: A blank project with a simulated ESP32 board, an empty code editor on the left, and a visual circuit canvas on the right.
3. Add the required libraries:
   - Click the **Libraries** button on the left sidebar.
   - Search for **`Modest-IoT-Nano-Framework`** and click **Add**.
   - Search for **`MQTT`** (by 256dpi) and click **Add**.
   - **What you'll see**: Each library appears in the project dependencies list with a checkmark. Wokwi downloads the library files in the background — this takes a few seconds.
4. Replace the default code with the `StatusIndicator` example from the framework's `examples/StatusIndicator/StatusIndicator.ino` file.
   - **What you'll see**: The code editor now contains the full `StatusIndicator` sketch (~180 lines). The default empty `setup()` and `loop()` functions are replaced with the framework's event-driven implementation.

### Configure the LED Pin

The example uses **GPIO 2** by default, which maps to the simulated onboard LED in Wokwi. No changes needed for the default simulation.

### Run and Verify

1. Click **Simulate** to start the simulation.
   - **What you'll see**: Wokwi compiles the sketch in the cloud. A progress indicator shows "Compiling..." for a few seconds, then the simulation starts. The serial output panel appears at the bottom of the screen.
2. In the serial output panel, you should see:

```
[StatusIndicator] Loop-less, event-driven command routing active.
[StatusIndicator] Timer event caught -> LED toggle dispatched.
[StatusIndicator] Timer event caught -> LED toggle dispatched.
[StatusIndicator] Timer event caught -> LED toggle dispatched.
...
```

   - **What this means**: The first line confirms the framework booted successfully — the asynchronous engine is running with background FreeRTOS tasks. Each "Timer event caught" line means the hardware timer fired, the event was dispatched through the framework's event queue, and the LED toggle command was routed to the actuator. These messages repeat every 500ms.
3. Observe the **LED toggling every 500ms** in the visual circuit view.
   - **What you'll see**: The simulated LED on the ESP32 board visually blinks — turning on and off twice per second. This confirms the actuator command routing is working: timer event → event handler → LED toggle command → GPIO state change.

The simulation is now running continuously. Click **Stop** to halt the simulation, or modify the code and click **Simulate** again to test changes.

### Simulation Limitations

- **BLE is not supported** — BLE gateway examples cannot be simulated.
- **MQTT to a local broker** requires a paid Wokwi subscription (Private IoT Gateway).
- **FreeRTOS task scheduling** may differ slightly from real hardware.

For full framework features, continue to Route B with a physical ESP32 board.

---

## Route B: Hardware (ESP32 Board)

Run the framework on a physical ESP32 board with real sensors, actuators, and network connectivity.

### Prerequisites

- **ESP32 board** (any variant: ESP32-WROOM, ESP32-S3, ESP32-C3, etc.)
- **USB cable** (USB-A to Micro-USB or USB-C, depending on your board)
- **ESP32 Arduino core 3.x or later** (the BLE hardware abstraction layer supports core 3.x and 4.x APIs)
- **One of the following toolchains**:
  - Arduino IDE 2.x with ESP32 board package installed, **or**
  - Visual Studio Code with PlatformIO extension installed, **or**
  - CLion with ESP BSP (Board Support Package) plugin installed

### Installation

#### Option 1: Arduino Library Manager

1. Open **Arduino IDE**.
2. Go to **Tools > Manage Libraries...** (or press `Ctrl+Shift+I` / `Cmd+Shift+I`).
   - **What you'll see**: The Library Manager window opens, showing a searchable list of available libraries.
3. Search for **`Modest-IoT-Nano-Framework`**.
    - **What you'll see**: The framework appears in the results with its name, version (1.3.0 or later), and a brief description.
4. Click **Install** to install the framework and its dependency (MQTT).
    - **What you'll see**: A progress bar shows the download and installation. The framework and its dependency are downloaded in order: first MQTT, then Modest-IoT-Nano-Framework. Once complete, the Install button changes to a version number, confirming the library is installed.
   - **Verification**: Go to **File > Examples** and scroll to find **Modest-IoT-Nano-Framework** in the list. The `StatusIndicator` example should be available.

#### Option 2: PlatformIO (VS Code or CLion)

PlatformIO is the recommended path for professional IoT development. It works identically in both Visual Studio Code and CLion.

**Visual Studio Code:**

1. Open **Visual Studio Code** with the PlatformIO extension.
2. Create a new project or open an existing one.
3. In your `platformio.ini`, add the framework to `lib_deps`:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps = Modest-IoT-Nano-Framework >=1.3.0
```

4. PlatformIO will automatically resolve and download the framework and its dependencies.
    - **What you'll see**: The PlatformIO Output panel at the bottom shows the dependency resolution: "Resolving dependencies...", then downloads each package with progress bars. You'll see messages like "Downloading Modest-IoT-Nano-Framework @ >=1.3.0" followed by MQTT.
   - **Verification**: Check the `.pio/libdeps/` directory in your project — it contains the downloaded framework and dependency source code.

**CLion:**

1. Open **CLion** and install the **ESP BSP** plugin from **Settings > Plugins** (search for "ESP BSP").
   - **What you'll see**: After installation, CLion may prompt for a restart. After restart, the ESP BSP plugin adds ESP32 project templates and PlatformIO integration.
2. Create a new project: **File > New Project > ESP Project**.
3. Select your board (e.g., ESP32-DevKitC) and framework (Arduino).
   - **What you'll see**: CLion creates a project with `platformio.ini`, a `src/` directory containing `main.cpp`, and a CMakeLists.txt file.
4. In your `platformio.ini`, add the framework to `lib_deps`:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps = Modest-IoT-Nano-Framework >=1.3.0
```

5. CLion will automatically resolve and download the framework and its dependencies via PlatformIO.
   - **What you'll see**: The Build tool window shows the dependency resolution progress. CLion indexes the framework source code, enabling code completion, navigation (Ctrl/Cmd+Click), and inline documentation for all framework classes.

### First Project: Status Indicator

The `StatusIndicator` example toggles an LED at a fixed interval using the framework's event-driven model. No sensors, no network — just a proof that the framework is working.

#### Open the Example

**Arduino IDE:**
1. Go to **File > Examples > Modest-IoT-Nano-Framework > StatusIndicator**.

**PlatformIO (VS Code):**
1. Copy the example from the installed library:
   ```
   <platformio_libs>/Modest-IoT-Nano-Framework/examples/StatusIndicator/StatusIndicator.ino
   ```
2. Place it in your project's `src/` directory as `main.cpp` (or `.ino`).
   - **What you'll see**: The file contains ~180 lines of code with the `StatusIndicator` class, `setup()`, and an empty `loop()`.

**CLion:**
1. Copy the example from the installed library:
   ```
   <platformio_libs>/Modest-IoT-Nano-Framework/examples/StatusIndicator/StatusIndicator.ino
   ```
2. Place it in your project's `main.cpp` (CLion creates this file by default).
3. CLion will index the framework automatically, providing code completion, navigation, and inline documentation.
   - **What you'll see**: CLion highlights the framework classes (`Device`, `Led`, `Event`) in syntax colors. Ctrl/Cmd+Click on any framework class name opens the corresponding header file with full Doxygen documentation.

#### Configure the LED Pin

The example uses **GPIO 2** by default, which is the onboard LED on most ESP32 development boards. If your board uses a different pin, modify this line:

```cpp
static const int SYSTEM_DIAGNOSTIC_LED_PIN = 2;  // Change to your board's onboard LED pin
```

Common onboard LED pins:
| Board | Onboard LED Pin |
|-------|-----------------|
| ESP32-DevKitC | GPIO 2 |
| ESP32-S3-DevKitC | GPIO 8 |
| ESP32-C3-DevKitC | GPIO 8 |
| ESP32-WROVER | GPIO 2 |

### Wiring

The `StatusIndicator` example uses only the **onboard LED** — no external components are needed.

```
ESP32 Board
├── Onboard LED (GPIO 2) — no wiring required
└── USB cable — connect to your computer
```

For an **external LED**, wire it as follows:

```
ESP32 GPIO 2 ──[220Ω resistor]── LED(+) ── LED(-) ── GND
```

### Upload and Verify

#### Upload the Sketch

1. Connect your ESP32 board via USB.
   - **What you'll see**: The board's onboard LED may light up briefly. Your operating system detects the USB serial port (e.g., `/dev/ttyUSB0` on Linux, `/dev/cu.usbserial-*` on macOS, `COM3` on Windows).
2. Select the correct **Board** and **Port**:
   - **Arduino IDE**: Tools > Board > ESP32 Arduino, then Tools > Port > (your board's port)
   - **PlatformIO (VS Code)**: Board is configured in `platformio.ini`; ensure the USB port is detected.
   - **CLion**: Board is configured in `platformio.ini`; CLion auto-detects the USB port.
3. Upload:
   - **Arduino IDE**: Click **Upload**
   - **PlatformIO (VS Code)**: Run `pio run -t upload` or click the Upload button
   - **CLion**: Right-click `platformio.ini` > **Upload 'esp32dev'**
   - **What you'll see**: The build output shows compilation progress — each `.cpp` file is compiled, then linked into a single firmware binary. After compilation, the upload begins: the board resets automatically (onboard LED flashes), enters bootloader mode, and receives the firmware. You'll see progress messages like "Writing at 0x00010000..." followed by "Leaving..." and "Hard resetting via DTR/RST". The entire process takes 10-30 seconds depending on the board.
   - **After upload**: The board resets and starts running the sketch immediately. The onboard LED begins toggling at the configured interval.

#### Verify the Output

1. Open the **Serial Monitor** at **115200 baud**:
   - **Arduino IDE**: Tools > Serial Monitor
   - **PlatformIO (VS Code)**: `pio device monitor`
   - **CLion**: Right-click `platformio.ini` > **Monitor 'esp32dev'**
   - **What you'll see**: The Serial Monitor connects and may show a brief burst of garbage characters as the board resets. Then the clean output begins.
2. You should see:

```
[StatusIndicator] Loop-less, event-driven command routing active.
[StatusIndicator] Timer event caught -> LED toggle dispatched.
[StatusIndicator] Timer event caught -> LED toggle dispatched.
[StatusIndicator] Timer event caught -> LED toggle dispatched.
...
```

   - **What this means**: The first line confirms the framework booted — `setup()` completed, the asynchronous engine is running, and background FreeRTOS tasks are active. Each "Timer event caught" line means: the hardware timer fired (every 500ms), the event was placed in the queue, the background task processed it, and the LED toggle command was routed to the actuator. These messages repeat continuously, proving the event-driven pipeline is working.
3. Observe the **onboard LED toggling every 500ms** (twice per second).
   - **What you'll see**: The physical onboard LED blinks steadily — on for 500ms, off for 500ms. This confirms the full command chain: timer event → event handler → actuator command → GPIO state change → physical LED.

The device is now running continuously. The `loop()` function is empty — all work is handled by FreeRTOS background tasks. This is the core benefit of the framework: your main loop stays clean while the framework manages scheduling, sensing, and networking.

---

## Next Steps

Now that your first device is running, explore the following resources:

- **Full README** — Overview, architecture, and feature list: [README.md](../README.md)
- **Examples** — 28 examples covering sensors, actuators, MQTT, BLE, on-device ML, and automation: [examples/](../examples/)
- **Documentation** — ADRs, user stories, evolution history, and API reference: [docs/](../docs/)
- **Contributing Guide** — How to contribute to the framework: [CONTRIBUTING.md](../CONTRIBUTING.md)
- **Improvements Roadmap** — Planned features and comparison tables: [docs/improvements.md](../docs/improvements.md)

### Recommended Examples to Try Next

1. **HomeSecurityDevice** — Demonstrates the high-priority telemetry lane: routine sensor state uses `enqueueTelemetryPayload()` while critical alerts (water leak, door opened) bypass the standard queue via `enqueueHighPriorityPayload()`. The network task processes high-priority packets first, ensuring alerts reach the MQTT broker before routine telemetry.
2. **CropMonitoringStation** — Add real sensors (DHT, BH1750, soil moisture) with the framework's sampling scheduler and MQTT telemetry.
3. **MqttTelemetryDevice** — Connect to an MQTT broker and publish environmental telemetry data with configurable topics.
4. **IntrusionDetector** — Composite event pattern matching using `EventAggregator`: motion and door opened within a time window triggers an intrusion alert.

---

> **Tip**: The framework is designed to be loop-less. Your `loop()` function stays empty while FreeRTOS background tasks handle all scheduling, sensing, and networking. This is the core benefit of the Modest IoT Nano Framework.
