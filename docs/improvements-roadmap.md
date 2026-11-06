# Modest IoT Improvements Roadmap

Index, implementation status, and recommended execution order for the `docs/*-improvement.md` briefs.

## 1. Documents

| # | Document | Topic | Status |
|---|---|---|---|
| 1 | [library-readiness-improvement.md](library-readiness-improvement.md) | Cleanup, host tests, CI, release, footprint | Phase A / Host CI Complete |
| 2 | [dependencies-improvement.md](dependencies-improvement.md) | Unused and replaceable third-party libraries | Phase A Complete |
| 3 | [connectivity-improvement.md](connectivity-improvement.md) | Drivers and gateway clients; includes D1 and BLE | Phase A & BLE Driver Complete |
| 4 | [diagnostics-improvement.md](diagnostics-improvement.md) | Logging, sensor health, counters | Phases A & B Complete |
| 5 | [threading-model-improvement.md](threading-model-improvement.md) | Threading docs, engine config, command marshalling, ID registry | Proposed (Tier 2/4) |
| 6 | [sampling-interval-improvement.md](sampling-interval-improvement.md) | Per-slot sampling intervals and pure scheduler | Phases A & B Complete |
| 7 | [telemetry-improvement.md](telemetry-improvement.md) | Package ownership, pool, writer, queue policy | Partially Implemented (Tier 3/5) |
| 8 | [interrupt-power-improvement.md](interrupt-power-improvement.md) | Interrupt-driven sensing, tickless, sleep | Proposed (Tier 4/5) |

## 2. Prioritization method and status

Each work item is scored 0 to 5 on three benefits, and sized by cost. Scores reflect architectural value; revisit them after diagnostics data is captured.

- **Q** quality: correctness, safety, testability, release credibility.
- **P** performance: latency, CPU, memory, energy.
- **D** developer experience: first-run success, API clarity, debuggability.
- **Cost**: XS under half a day, S about a day, M several days, L a week or more.
- **Score** = 2Q + P + 2D, divided by cost weight (XS 1, S 1.5, M 2.5, L 4), rounded. Quality and DX are double-weighted because release credibility and developer onboarding come first.

| ID | Work item | Doc | Q | P | D | Cost | Score | Status |
|---|---|---|---|---|---|---|---|---|
| W1 | Fix abstract `WiFiConnectivityDriver` (D1); examples must compile | connectivity | 5 | 0 | 5 | XS | 20 | **Completed** |
| W2 | Delete dead `LoadCellController`, unused deps, unused includes, pin ArduinoJson | dependencies | 4 | 1 | 4 | XS | 17 | **Completed** |
| W3 | Repo cleanup (`main.cpp`, empty `.ino`, `.gitignore`), `library.properties` fixes | library-readiness | 4 | 0 | 4 | XS | 16 | **Completed** |
| W4 | Threading rules, "Runs on" tags, ID registry (docs only) | threading | 3 | 0 | 4 | XS | 14 | **Completed** (README updated, `@runs_on` tags added) |
| W5 | Host test harness plus CI (host tests, compile-examples, lint) | library-readiness | 5 | 0 | 4 | M | 7 | **Completed** |
| W6 | Logging macros, stack high-water, sensor fault events, sensor conversion | diagnostics | 4 | 1 | 5 | S | 13 | **Completed** |
| W7 | Connectivity reconnect policy, `mqttClient.loop()` every cycle, link events | connectivity | 5 | 2 | 4 | M | 8 | **Completed** |
| W8 | Per-slot sampling intervals, pure scheduler, elapsed-tick accounting, example migration | sampling | 4 | 5 | 5 | M | 9 | **Completed** |
| W9 | `EngineConfig` and `stopAsynchronousEngine()` | threading | 4 | 2 | 3 | S | 11 | **Completed** |
| W10 | Optional Bluetooth, BLE driver addition, optional MQTT footprint (P1) | connectivity, library-readiness | 3 | 2 | 4 | S | 11 | **Partially Complete** (BLE NUS added, BT Classic isolated) |
| W11 | Telemetry ownership and queue policy (Phases A and B) | telemetry | 4 | 3 | 4 | M | 8 | **Partially Complete** (Phase A done, `unique_ptr` overload done, drop-oldest policy done, bounded serialization buffer done, envelope metadata done, T13 golden tests done, T14 name hiding resolved) |
| W12 | `Device::post` command marshalling | threading | 3 | 0 | 4 | M | 6 | Pending (Tier 4) |
| W13 | Interrupt triggers for Button and PIR | interrupt-power | 3 | 4 | 3 | M | 6 | Pending (Tier 4) |
| W14 | HX711 data-ready and ultrasonic echo capture (remove blocking) | interrupt-power | 2 | 4 | 2 | M | 5 | Pending (Tier 4) |
| W15 | Cloud-to-device commands (MQTT subscribe) | connectivity | 2 | 0 | 4 | M | 5 | Pending (Tier 4) |
| W16 | Tickless scheduling and light sleep | interrupt-power | 2 | 4 | 2 | L | 3 | Pending (Tier 5) |
| W17 | Deep-sleep cycle and battery example | interrupt-power | 1 | 4 | 2 | L | 3 | Pending (Tier 5) |
| W18 | Telemetry pool and writer interface (Phase C) | telemetry | 2 | 3 | 1 | L | 2 | Pending (Tier 5) |
| W19 | Native servo and LCD drivers | dependencies | 2 | 0 | 1 | M | 2 | Pending (Tier 5) |

## 3. Recommended order and progression tiers

Progression history and tier execution:
- **Tier 0 (Correctness & Hygiene)** is **Complete**: `WiFiConnectivityDriver` abstract class issue resolved, dead `LoadCellController` deleted, `library.properties` standardized, examples compile cleanly.
- **Tier 1 (Safety Net & Test Consolidation)** is **Complete**: Host test infrastructure consolidated under `tests/fakes/` (4 suites: Light, LED, RFID, BLE), `tests/run_all.sh` runner, and GitHub Actions CI workflow active.
- **Tier 2 (Reliability & Footprint)** is **Complete**: BLE driver implemented, MQTT keep-alive and reconnect policy completed, `EngineConfig` and `stopAsynchronousEngine()` completed, `MODEST_LOG_*` zero-cost logging facility completed, `DiagnosticsSnapshot` with shared counters completed, sensor fault and recovery events completed with sensor conversion (DhtSensor, LoadCellAmplifier, UltrasonicSensor, DigitalLightIntensitySensor).
- **Tier 3 (Core Scheduling & Telemetry)** is **Partially Complete**: Pure `SamplingScheduler` with per-slot periods completed, Hybrid approach with virtual sampling interval getters and `Sensor*` overload completed, all 10 examples migrated to use per-sensor intervals, host tests passing. `TelemetryPackage` ownership remains.

| Tier | Items | Goal | Exit criterion | Status |
|---|---|---|---|---|
| 0: Make it correct and shippable | W1, W2, W3, W4 | Everything compiles, repo is clean, rules are written down | All examples compile cleanly; clean repo layout | **Complete** |
| 1: Safety net and visibility | W5 | Regressions caught automatically via host tests & CI | CI green on PRs/pushes; all 4 host test suites pass | **Complete** |
| 2: Reliability, diagnostics & footprint | W6 (logging/high-water/sensor conversion), W7, W9, W10 | Reconnection robust, tasks tunable, BLE modern, zero-cost logs, sensor fault events with conversion | Soak test reconnects; logs available behind macro; BLE tests pass; diagnostics snapshot works; sensors report faults | **Complete** |
| 3: Core performance and DX | W8, then W11 | Per-slot intervals, bounded memory use for telemetry | Scheduler host tests pass; all examples use per-sensor intervals; sampler wake budget measured | In Progress |
| 4: Capabilities | W12, W13, W14, W15, rest of W6 | Edge-driven sensing, remote commands, fault events | Latency measured for polling versus interrupt | Queued |
| 5: Optimization and power | W16, W17, W18, W19 | Battery operation and further tuning | Decide per item from measured data | Queued |

Parallelism: Tier 0 and Tier 1 have landed. Tier 2 items (logging macros, reconnect state machine, `EngineConfig`) can be executed independently.

## 4. Dependencies between items

- Interrupt triggers and `Device::post` share one notification redesign (bit flags plus an atomic tick counter) and require the pure scheduler.
- Tickless sleep requires `ticksUntilNextDue()` from the scheduler.
- Cloud-to-device commands (connectivity) require `Device::post` (threading).
- Stack-size defaults should come from diagnostics measurements.
- Telemetry writer and pool work should follow the host test harness.

## 5. Deliberately out of scope

- Replacing DHT, PubSubClient, ArduinoJson or TLS.
- Targeted (per-instance) commands until a use case appears.
- Switchable sensor power rails and ADC/DMA sampling.
- Non-ESP32 targets.

## 6. Claims verified and remaining verification gaps

- **Verified**: Local `arduino-cli compile` cleanly builds all 10 examples across ESP32 targets with `--library .` configured.
- **Verified**: `library.properties` `depends=` syntax formatted for Arduino Library Manager compliance.
- **Verified**: Bluetooth Classic fails on modern ESP32 variants (S2, S3, C3, C6, H2); resolved via target guards and `BleConnectivityDriver`.
- **Verified**: Host test harness executes 11 unit test suites (`tests/digital_light`, `tests/rgb_led`, `tests/rfid_sensor`, `tests/ble_connectivity`, `tests/log`, `tests/mqtt_gateway`, `tests/engine_config`, `tests/diagnostics`, `tests/sensor_fault`, `tests/scheduler`, `tests/telemetry`) under Address and Undefined sanitizers.
- **Remaining**: Performance, latency, memory and power figures (reasoned, to be measured on hardware).
- **Remaining**: `Wire` thread-safety on core 3.x; `loop()` core affinity.
- **Remaining**: Task notification array availability; `CONFIG_PM_ENABLE` in the precompiled core.
- **Remaining**: Datasheet values behind the per-sensor interval constants.

## 7. Comparison with traditional development (end state and per tier)

Compares three points: a traditional Arduino `loop()` app, Modest IoT today, and Modest IoT once Tiers 0 to 4 are done (Tier 5 is optional and measured separately). **All entries are reasoned from the source and the briefs, not measured.** Section 8 defines how to turn them into numbers.

### 7.1 Developer experience

| Aspect | Traditional `loop()` | Modest IoT today | Modest IoT after Tier 4 |
|---|---|---|---|
| First build | Works with the board package | Some examples do not compile (`WiFiConnectivityDriver` abstract); extra libraries to install | All examples compile in CI; only used libraries are declared; Bluetooth and MQTT optional |
| Structure | One growing `loop()` with `millis()` flags and global state | Sensors emit events, a `Device` mediates, actuators take commands | Same, plus documented threading rules and an ID registry |
| Adding a sensor | New globals, new timing variable, new branch in `loop()` | Subclass or reuse, register in the scheduler | One line with a per-sensor default period (`appendSensorToScheduler<T>`) |
| Sampling rates | Hand-written per-sensor timers | One global tick for all slots (fixed rate) | Per-slot periods, minimums enforced, staggering handled |
| Timing mistakes | Easy: a blocking call stalls everything | Same risk inside the single sampler task | Handler budgets documented; blocking HX711/ultrasonic reads removed from the sampler |
| Debugging | `Serial.println` scattered | No logging, no counters | `MODEST_LOG_*`, stack high-water marks, sensor fault events, health snapshot |
| Networking | Blocking connect/publish in `loop()` | Separate network task, but weak reconnect and a keep-alive defect | Reconnect policy, keep-alive each cycle, link-state events, send status |
| Cloud-to-device | Hand-rolled callback | Not supported | Mediator-routed commands through `Device::post` |
| Testing | Rarely possible off-device | Two host test suites | Host harness and CI on every change |
| Learning curve | Lowest to start, steepest as the sketch grows | Moderate (events, commands, mediator) | Same concepts; fewer pitfalls, more to read |

Honest cost: Modest IoT asks for more concepts up front than a 30-line blink sketch. The benefit appears once a sketch has several sensors, actuators and a network path.

### 7.2 Performance

| Aspect | Traditional `loop()` | Modest IoT today | Modest IoT after Tier 4 |
|---|---|---|---|
| Timing model | Cooperative polling; jitter grows with the slowest block in `loop()` | Timer ISR plus a sampler task at priority 3; one slot per tick, round-robin | Same engine; each slot runs at its own period, missed ticks are accounted for |
| Sample period accuracy | Depends on `loop()` duration | Fixed tick shared by all slots (an N-slot sensor gets 1/N of the tick rate) | Per-slot periods independent of slot count |
| Worst-case stall | Any blocking call, including network | HX711 up to 100 ms, ultrasonic up to 30 ms, DHT about 20 to 25 ms inside the sampler | Interrupt-driven HX711/ultrasonic and documented budgets; DHT stays as is |
| Edge latency (button, PIR) | Up to one `loop()` pass | Up to one polling period | Interrupt mode: task wake-up latency instead of a period (to be measured) |
| Network isolation | A slow publish blocks sensing | Isolated in the network task | Isolated, with bounded queue policy and no silent packet loss |
| Heap behaviour | Usually static | One `new`/`delete` per packet | Ownership made explicit; pool only if measurement justifies it (Tier 5) |
| CPU when idle | Spins in `loop()` at 100 % unless delays are added | Sleeps between ticks, but wakes at a fixed tick forever | Fewer idle wake-ups from per-slot periods and interrupt sensors; true sleep only in Tier 5 |
| Energy | High unless hand-tuned | Same as traditional (no sleep path) | Modestly better; large savings only with Tier 5 sleep modes |

Expected direction: Modest IoT will not beat a hand-tuned single-purpose `loop()` on raw cycles, because it adds an RTOS notification, a queue and virtual dispatch. The claim to test is narrower: **more predictable timing and better isolation from network stalls as the application grows**, at a small fixed overhead.

### 7.3 Resource consumption

| Resource | Traditional `loop()` | Modest IoT today | Modest IoT after Tier 4 |
|---|---|---|---|
| Task stacks | `loop()` task only | 4096 B sampler plus 8192 B network, fixed | Same defaults, configurable through `EngineConfig`; tuned from measured high-water marks |
| Queue and packets | None | Queue of pointers plus one heap packet per reading | Same order of magnitude; bounded by queue policy |
| Flash | Smallest | Larger: framework, Wi-Fi, Bluetooth driver pulled in by the umbrella header, plus unused dependencies | Smaller than today if Bluetooth/MQTT are optional and dead dependencies are gone; still larger than a bare sketch |
| Static RAM | Small | Slot table, per-object state | Slot table gains a few words per slot (period, countdown, minimum); small |
| Heap fragmentation risk | Low | Moderate over long runs (`new`/`delete` per packet, `String` in places) | Lower with explicit ownership; pool in Tier 5 if measured |
| Third-party dependencies installed | As needed | 8 declared, 2 unused, 1 dead-code only | Only those used (DHT, LiquidCrystal_I2C, ESP32Servo, PubSubClient, ArduinoJson) |
| Logging overhead | User-controlled | None | Zero when `MODEST_LOG_*` is compiled out; UART cost when on |

### 7.4 Progression at the end of each tier

Cumulative state after each tier, against a traditional `loop()` app. Same caveat: reasoned, not measured. "Trad." means a typical multi-sensor `loop()` sketch.

#### Developer experience

| After | State versus traditional |
|---|---|
| Today | Better structure, worse first run: some examples fail to compile, no logging, rules unwritten. Trad. wins on "it just builds" |
| Tier 0 | Parity on first build (examples compile, clean repo, fewer installs). Structure advantage is now usable; threading and ID rules are documented, so fewer surprises than a growing `loop()` |
| Tier 1 | Ahead on trust: CI and host tests catch regressions, which trad. sketches rarely have. Logging macros and stack high-water marks give debugging tools trad. users build by hand |
| Tier 2 | Ahead on connected devices: reconnect, keep-alive and link events work out of the box where trad. users hand-write a state machine. Tunable stacks; builds on ESP32-S2/S3/C3 |
| Tier 3 | Clearly ahead for multi-sensor apps: one line per sensor with a sensible default period, instead of a `millis()` flag per sensor. Explicit packet ownership removes leak and double-free guesswork |
| Tier 4 | Ahead on features: interrupt sensors without writing ISRs, remote commands, sensor fault events. Cost: more concepts to learn than trad. |
| Tier 5 | Battery operation through a policy setting instead of hand-built sleep code; optional, measured |

#### Performance (versus a `loop()` app)

| After | Timing and latency | Isolation | Idle CPU and energy |
|---|---|---|---|
| Today | Fixed shared tick; per-sensor rate falls as slots are added. Jitter from blocking handlers (HX711 up to 100 ms) | Network work is isolated; keep-alive defect can drop the MQTT session | Sleeps between ticks but ticks forever; about equal to trad. in energy |
| Tier 0 | Unchanged | Unchanged | Unchanged |
| Tier 1 | Unchanged (now observable) | Unchanged | Unchanged; log overhead only if enabled |
| Tier 2 | Unchanged | Better: network faults recover and do not stall sensing; stack overflows avoided by tuning | Unchanged |
| Tier 3 | Per-slot periods independent of slot count; missed ticks accounted for; worst-case handler budget documented | Bounded queue policy, no silent loss | Fewer needless sensor reads for slow sensors |
| Tier 4 | Edge latency drops from one polling period to task wake-up time; HX711/ultrasonic no longer stall the sampler | Commands marshalled onto the sampler, no cross-task races | Idle button/PIR slots leave the periodic table |
| Tier 5 | Same as Tier 4 | Same | Tickless and sleep modes can cut average current substantially; amount unknown until measured |

Expected shape: trad. is faster per cycle but its jitter and stall exposure grow with application size. Modest IoT starts roughly level, falls behind slightly in overhead, then pulls ahead on predictability from Tier 3 and on latency from Tier 4.

#### Resources (versus a `loop()` app)

| After | Flash | RAM and stacks | Heap | Dependencies |
|---|---|---|---|---|
| Today | Larger than trad. (Bluetooth pulled in by the umbrella header, dead code) | 4 KB + 8 KB task stacks, fixed | `new`/`delete` per packet | 8 declared, 2 unused, 1 dead-code only |
| Tier 0 | Slightly smaller (dead `LoadCellController` removed) | Unchanged | Unchanged | Unused ones removed, ArduinoJson pinned |
| Tier 1 | Slightly larger with logs enabled; zero when compiled out | Unchanged; test code is host-only | Unchanged | Unchanged |
| Tier 2 | Smaller when Bluetooth/MQTT are optional | Configurable; likely small rise for TLS-capable network stack | Unchanged | Optional libraries not installed unless used |
| Tier 3 | Small rise for the scheduler | A few words per slot | Lower fragmentation risk from explicit ownership | Unchanged |
| Tier 4 | Small rise (interrupt, command queue) | One extra small queue; ISR code in IRAM | Unchanged | Unchanged |
| Tier 5 | Rise if sleep and pool code is included (opt-in) | Pool preallocates RAM (trade for no fragmentation) | Lowest | Possibly fewer if native drivers replace servo/LCD libraries |

Expected shape: footprint is above trad. today, falls back toward it in Tiers 0 and 2, and then grows modestly with features. The framework should stay clearly larger than a bare sketch; the target is "fits comfortably on a 4 MB ESP32 with headroom", checked against measured size reports.

## 8. How to validate the comparison

Run the same three reference applications on the same ESP32 board, once per variant (plain `loop()`, current Modest IoT, post-Tier-4 Modest IoT):

1. **Multi-sensor monitor:** DHT, BH1750 and a button, with a display and Wi-Fi/MQTT telemetry (similar to `AirConditioningDevice`).
2. **Blocking-sensor stress:** the monitor plus an HX711 and an ultrasonic sensor.
3. **Network fault:** the monitor with the broker unreachable for 60 seconds, then restored.

| Metric | Method |
|---|---|
| Sample period jitter | GPIO toggle per sample on a logic analyzer, or `esp_timer_get_time` deltas logged to RAM |
| Button and PIR edge-to-action latency | Logic analyzer: input edge to actuator output edge |
| Sensing continuity during network fault | Count missed samples while the broker is down |
| Reconnect time | Time from broker restore to first successful publish |
| Stack use | `uxTaskGetStackHighWaterMark` for each task |
| Heap | `ESP.getFreeHeap()` and `getMinFreeHeap()` after 24 hours; track fragmentation (`getMaxAllocHeap`) |
| Flash and static RAM | Size report from the build for each variant |
| Current draw | Inline current meter (Wokwi cannot measure it) |
| Lines of application code | Count in the reference sketches, as a rough developer-experience proxy |

Publish the table in the README with the board, core version and commit hashes. Capture measurements after each tier, not only at the end, and update the entries in 7.2 to 7.4 to measured values, and re-score the work items in section 2 from the results, particularly W16 to W19.
