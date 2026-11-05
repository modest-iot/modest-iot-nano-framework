# Improvement 7: Threading Model, Command Marshalling and Identifiers

Status: **partially implemented** (Phase B: EngineConfig and stopAsynchronousEngine completed; Phase A documentation and Phase C command marshalling proposed). The "current model" below is read from the source; behaviours of the Arduino-ESP32 core marked "verify" were not tested.

Related: [improvements-roadmap.md](improvements-roadmap.md), [sampling-interval-improvement.md](sampling-interval-improvement.md) (handler budgets), [connectivity-improvement.md](connectivity-improvement.md) (cloud-to-device commands, link events), [telemetry-improvement.md](telemetry-improvement.md), [diagnostics-improvement.md](diagnostics-improvement.md), [interrupt-power-improvement.md](interrupt-power-improvement.md).

---

## 1. Current model (verified from source)

| Context | What runs there |
|---|---|
| Timer ISR | `Device::handleHardwareTimerInterrupt`: one `vTaskNotifyGiveFromISR` |
| `HardwareWorker` (priority 3, 4096 bytes, core 1) | Scheduler dispatch, then `Sensor::on` and `processEvent`, then the **mediator's `Device::on`** (events propagate synchronously up the parent chain), and from there every actuator `handle(Command)` the mediator issues, plus `new TelemetryPackage` and `enqueueTelemetryPayload` |
| `NetworkWorker` (priority 1, 8192 bytes, core 1) | `processQueuedTelemetryData`, the gateway `sendTelemetryRecord`, `serializeAsString`, then `delete` of the packet |
| Arduino `loop()` task | `setup()` (all constructors, `pinMode`, `Wire.begin`, `Device` construction and `initializeAsynchronousEngine`), then user `loop()` (examples just `vTaskDelay`) |
| Wi-Fi/Bluetooth system tasks | ESP-IDF event callbacks, not currently used by the framework |

Important consequences of the synchronous propagation:
- **Actuator work runs on the sampler task.** In `AirConditioningDevice` a button event leads to `statusDisplay.handle(...)` (I²C LCD update) and a `ServoMotor` command inside the sampler's context, so slow actuators consume the sensor tick budget.
- **Everything is effectively single-threaded on the sampler**, which is why there are no locks anywhere in `src/` (verified: no mutex, semaphore, or critical-section code outside `Device`). That is a strength, provided it stays true and is documented.
- The two workers share no mutable framework state except the queue, and packets are snapshots, so the network side is safe today.

The model breaks the moment anything else touches the same objects from another task. Three planned features do exactly that: cloud-to-device commands (network worker), link-state events (system event task), and interrupt triggers (ISR, which only sets a flag and so stays safe).

## 2. Findings

| # | Finding | Evidence | Impact |
|---|---|---|---|
| H1 | The threading rules are not documented anywhere | No such section in README, Doxygen or the class diagram | Users cannot know that actuators must be driven from the mediator context, or that `loop()` must not call them |
| H2 | Buffered-setter-then-command actuator pattern is not atomic | `Buzzer::setTone` then `handle(PLAY_TONE)`; `RgbLed::setColor` then `handle(SET_COLOR)`; servo buffered angle then `DRIVE_TO_BUFFERED_ANGLE` | Safe on one task. With two tasks, an interleaving applies one task's parameters with another's command |
| H3 | `Command` carries only an identifier | `Command.h`: `const int identifier` | Commands cannot carry an argument, which is why the buffered pattern exists, and why a remote command (MQTT payload) has no way to say "set colour to X" |
| H4 | Commands have no target | One `Led::TURN_ON_COMMAND` (100) means "the LED" for every `Led` instance; the mediator picks the instance | A command arriving from the network cannot address a specific actuator without extra routing logic |
| H5 | ~~Stack sizes and priorities are hard-coded~~ | **Resolved:** `EngineConfig` struct provides tunable stack sizes, priorities, and core pinning with original values as defaults. | **Done** |
| H6 | ~~Both workers and the Arduino `loop()` task are pinned to core 1~~ | **Resolved:** `EngineConfig` makes core pinning configurable via `samplerCore` and `networkCore`. | **Done** |
| H7 | ~~No engine shutdown path; destructor ordering is unsafe~~ | **Resolved:** `stopAsynchronousEngine()` provides cooperative task termination, queue drain, and timer cleanup. `~Device()` calls it as a safety net. | **Done** |
| H8 | Event and command identifier spaces are unmanaged integers | All values are constants in the owning class (see 5) | Collisions inside one space are easy for user code to introduce; there is no reserved range for the framework |
| H9 | Shared bus access | The BH1750 (`Wire`) and the I²C LCD (`LiquidCrystal_I2C`) share the I²C bus; today both run on the sampler | Safe as long as only the sampler uses `Wire`. A user sketch calling `Wire` from `loop()` races. Whether `Wire` is thread-safe depends on the core version: **verify** for core 3.x |
| H10 | `registeredSlotCount` is written by the setup task and read by the sampler | `appendSensorToScheduler` after the engine starts is unguarded | Covered by the `engineStarted` guard in the sampling document |

## 3. Proposal

### 3.1 Document the threading model (no code; do first)
Add a "Threading model" section to the README and the Doxygen main page, and a diagram to `docs/class-diagram.puml` (or a separate sequence diagram), containing:
1. The table in section 1.
2. Rules:
   - Sensor, mediator and actuator code runs on the sampler task and **must not block for long** (see the handler budget in the sampling document).
   - Do not call actuators, sensors or `Wire` from `loop()` or other tasks; use the marshalling mechanism in 3.2.
   - Telemetry packets are immutable snapshots passed by ownership transfer through the queue.
   - Interrupts only set flags (interrupt document).
3. A per-method "Runs on:" Doxygen tag for the public hooks (`on`, `handle`, `processEvent`, `processQueuedTelemetryData`, gateway `send...`).

### 3.2 Command marshalling
Give other tasks a safe way to act on the sampler's objects.

| Option | Description | Pros | Cons |
|---|---|---|---|
| **M1: document only** | State the single-context rule | Free | Does not enable cloud-to-device commands |
| **M2: `Device::post(CommandHandler*, Command)`** | A command queue consumed by the sampler task; the sampler wakes through the same notification used for interrupt triggers (bit flag), drains the queue, and calls `handle` on its own context | Keeps every actuator single-threaded; no locks in actuators; small | Needs the notification redesign (N1 in the interrupt document); queue bounds and drop policy to define |
| **M3: per-actuator mutex** | Lock inside each actuator `handle` | Local | Locks in 8 classes, priority inversion risk between sampler (3) and network (1), still not atomic across set-then-handle |
| **M4: argument-carrying `Command`** | Extend `Command` with an optional value: `Command(id, int32_t argument = 0)` | Removes the buffered-setter race (H2), enables remote parameters | Changes a core type; `Command` currently has a `const` member, so it is copy-constructible but not assignable (it would need care if stored in a queue by value, which the queue itself handles via copy) |

**Recommendation:** M1 now; M2 as the mechanism for any cross-task command (it is the prerequisite for MQTT subscribe); consider M4 afterward to remove the buffered-setter pattern. Do not pursue M3.

M4 sketch (source compatible, since existing call sites pass one argument):

```cpp
class Command {
public:
    const int identifier;
    const int32_t argument;    // 0 when unused
    constexpr explicit Command(int id, int32_t arg = 0) : identifier(id), argument(arg) {}
};
```

For multi-parameter commands (`RgbLed::setColor` needs three channels, `Buzzer` needs frequency and duration), either pack into one 32-bit value for small cases or keep the buffered pattern but guarantee it only runs on the sampler through M2.

### 3.3 Addressing
Remote commands need a target (H4). Two ways, in increasing weight:
1. **Mediator routing (recommended first):** the application's `Device::handle(Command)` (already part of the framework: `Device` is a `CommandHandler`) maps a command identifier or a topic/subtopic to the actuator it owns. No framework change; document the pattern and provide an example.
2. **Targeted command:** add `targetId` to `Command` and give actuators an optional logical id. More flexible, but larger API surface; defer until a real use case demands it.

### 3.4 Engine configuration
Replace the hard-coded numbers with a configuration struct, with today's values as defaults:

```cpp
struct EngineConfig {
    uint32_t samplerStackBytes   = 4096;
    uint32_t networkStackBytes   = 8192;
    uint8_t  samplerPriority     = 3;
    uint8_t  networkPriority     = 1;
    BaseType_t samplerCore       = 1;
    BaseType_t networkCore       = 1;
    uint8_t  queueLength         = 10;
};
void initializeAsynchronousEngine(const EngineConfig& config = EngineConfig{});
```

Keep the existing `initializeAsynchronousEngine(uint8_t maximumQueueLength)` overload for source compatibility. Use the stack high-water marks from the diagnostics document to pick data-driven defaults. This also resolves open decision 8 in the connectivity document (network stack size).

### 3.5 Lifetime
Add `stopAsynchronousEngine()` that stops the timer, signals the tasks to exit and waits for them (a termination notification, not `vTaskDelete` of a running task), frees the queue and drains and deletes pending packets. Call it from the `Device` destructor **and document that derived classes must call it first in their own destructor**, because derived members are gone by the time `~Device` runs. Test with a host fake of the task API, or at least with a hardware soak test that creates and destroys a device.

### 3.6 I²C and shared peripherals
Document that a bus is owned by the sampler context. If users need to share it, provide a lock helper around `Wire` use inside the framework, after verifying whether the core already serializes access.

## 4. Phases

**Phase A: documentation (small, no risk)**
1. Threading section in README and Doxygen, diagram, "Runs on" tags.
2. Identifier registry document (section 5).

**Phase B: configuration and lifetime [DONE]**
3. `[DONE]` `EngineConfig` with unchanged defaults — implemented in `src/Device.h`.
4. `[DONE]` `stopAsynchronousEngine()` and destructor handling — implemented in `src/Device.cpp` with cooperative termination via notification bits (bit 1 = stop).

**Phase C: cross-task commands (prerequisite: notification redesign from the interrupt document, and the pure scheduler)**
5. `Device::post` and the command queue.
6. Optional argument-carrying `Command`.
7. MQTT subscribe mapping (connectivity document) built on `post`.

## 5. Identifier registry

Current identifiers (verified from source), all plain integers in class scope:

| Space | Range in use | Owner |
|---|---|---|
| Events | 10 | `Button::BUTTON_PRESSED` |
| Events | 20, 21 | `Sensor::DATA_READ`, `MEASURE_DATA_REQUESTED` |
| Events | 30, 31 | `PassiveInfraredSensor` motion detected, stopped |
| Events | 40 | `GasSensor` digital gas detected |
| Events | 50, 51 | `DigitalLightIntensitySensor` light detected, absent |
| Commands | 100 to 102 | `Led` |
| Commands | 150 to 152 | `ServoMotor` |
| Commands | 200 to 202 | `CharacterLcdDisplay` |
| Commands | 250 to 252 | `Buzzer` |
| Commands | 300 to 303 | `RgbLed` |

**Correction to an earlier remark:** the `ClassicBlinker` example uses `TIMER_EVENT_IDENTIFIER = 300`, the same number as `RgbLed::TURN_ON_COMMAND_IDENTIFIER`. That is **not** a collision: `Event` and `Command` are different types with separate handlers (`EventHandler::on` versus `CommandHandler::handle`). Cross-space equality is harmless. Collisions matter only inside one space, or if a future feature routes both through one switch.

Real risks:
- **User event identifiers can collide with future framework ones.** Framework events use 10 to 51 so far, with 22 and 23 proposed for sensor health ([diagnostics-improvement.md](diagnostics-improvement.md)) and a block proposed for link events ([connectivity-improvement.md](connectivity-improvement.md)).
- **Commands are not unique per instance (H4)**, so a remote command id selects a class of actuator, not a device.
- **`sourceId` is overloaded**: sensors fill it with a pin number, DHT leaves the default `-1`, and the light sensor uses `signalGpioPin` only as an id. There is no consistent meaning.
- **Declaration style differs**: most identifiers are `static constexpr int`, several are `static const int` (`Button`, LCD, `Buzzer`); harmless but inconsistent.

Proposal:
1. Publish a registry table (this one, maintained in the README) with reserved ranges: framework events 0 to 999, framework commands 0 to 999 (existing values stay), **user identifiers from 1000**.
2. Reserve blocks now: sensor-health 22 to 23, connectivity link events 900 to 909, power events 910 to 919.
3. Add a host test that includes every public header with the fakes and `static_assert`s uniqueness within each space (feasible only for headers that compile on the host; grow it with the harness).
4. Update the `ClassicBlinker` example to use an identifier at or above 1000 to model the rule.
5. Define and document `sourceId` as "the logical id of the source", defaulting to the pin number; give sensors an optional id constructor parameter only if a use case needs it.

## 6. Risks

| Risk | Mitigation |
|---|---|
| Adding a command queue adds a second wake path to the sampler | Reuse the single notification with bit flags; test ordering and loss on the host |
| `Command` size change affects queued copies | Keep it two `int32_t`; copy semantics already exist |
| Engine shutdown races | Cooperative termination notification, never `vTaskDelete` on a running task |
| Raising stack sizes costs RAM | Defaults unchanged; guidance from measured high-water marks |
| Over-documenting unverified core behaviour | Mark "verify" items and confirm on the target core before publishing |

## 7. Open decisions

| # | Decision | Recommended |
|---|---|---|
| 1 | Marshalling mechanism | M1 now, M2 for cross-task commands, M4 later |
| 2 | Addressing | Mediator routing first, targeted commands only on demand |
| 3 | Configuration | `[RESOLVED]` `EngineConfig` with current values as defaults |
| 4 | Lifetime | `[RESOLVED]` `stopAsynchronousEngine()` plus documented destructor rule |
| 5 | User identifier range | 1000 and above |
| 6 | `sourceId` meaning | Logical source id, defaulting to the pin |
| 7 | Core pinning default | Keep both on core 1; make it configurable |

## 8. Acceptance criteria

- [ ] README and Doxygen contain the threading model, the rules, and "Runs on" tags for the public hooks.
- [ ] The identifier registry is published, user range 1000+ is reserved, and `ClassicBlinker` follows it.
- [x] `EngineConfig` exists and the old overload still works.
- [x] `stopAsynchronousEngine()` exists with a documented destructor rule and a host test (`tests/engine_config/test.cpp`).
- [ ] `Device::post` marshals a command onto the sampler task; a host test proves ordering and bounded queue behaviour.
- [ ] Remote commands (if implemented) reach actuators only through the sampler context.
- [ ] Class diagram updated.
