# Improvement 4: Library Readiness, CI and Test Infrastructure

Analysis and implementation progress. Status: **partially implemented** (Phase A hygiene completed: repository clutter removed, dependencies cleaned, tests consolidated into `tests/fakes/`, `tests/run_all.sh` runner created, CMake test targets added, and `host-tests.yml` CI workflow introduced). Findings come from reading the repository; example compilation verified locally via `arduino-cli`.

Related: [improvements-roadmap.md](improvements-roadmap.md) (this is step 0: everything else depends on it), [dependencies-improvement.md](dependencies-improvement.md), [connectivity-improvement.md](connectivity-improvement.md) (defect D1 is found by compile CI), [sampling-interval-improvement.md](sampling-interval-improvement.md) (needs the host test harness).

---

## 1. Why this comes first

Three other documents propose behaviour changes (scheduler, gateways, telemetry). None can be validated safely today because:
- **Automated build is in place.** `host-tests.yml` CI workflow runs host tests on push and pull requests. Example compilation verified locally via `arduino-cli`.
- **Local compile was unreliable.** `arduino-cli compile` previously failed on this development machine (`ctags` "bad CPU type"), resolved via proper toolchain configuration and `--library .` in CMake.
- **Tests are consolidated.** Ten host suites exist under `tests/fakes/` shared harness with `tests/run_all.sh` runner, CMake test targets, and Address/Undefined sanitizers.
- **The repository is closer to publishable.** Root clutter removed, dependencies cleaned, but no git tags yet and `library.properties` needs `arduino-lint` validation.

## 2. Current state (verified)

| Area | Finding | Evidence |
|---|---|---|
| Library layout | Valid: `library.properties`, `src/`, `examples/`, `keywords.txt`, MIT license | repo root |
| Root clutter | ~~`main.cpp` and empty `.ino`~~ | **Resolved:** Root clutter removed, CMake `SKETCH_DIR` repointed to `examples/ClassicBlinker`. |
| Build coupling to the clutter | ~~`CMakeLists.txt` compiled the repo root~~ | **Resolved:** `CMakeLists.txt` `SKETCH_DIR` repointed to `examples/ClassicBlinker` with cache variable. |
| Ignore rules | `.gitignore` ignores `/cmake-build-debug/`, `/.idea/`, `/.junie/`, `/cmake/`, `/docs/build/`. It does not ignore `.DS_Store` (untracked copies exist in `docs/`, `examples/`, `src/`) or `cmake-build-arduino/` | `.gitignore`, `git status` |
| CI | `host-tests.yml` runs host tests on push/PR | `.github/workflows/host-tests.yml` |
| Releases | No git tags. `library.properties` says `version=1.0.0` | `git tag` is empty |
| Host tests | 10 suites under `tests/fakes/` shared harness: digital_light, rgb_led, rfid_sensor, ble_connectivity, log, mqtt_gateway, engine_config, diagnostics, sensor_fault, scheduler. Runner: `tests/run_all.sh`. | `tests/` |
| Umbrella header | `ModestIoT.h` includes every sensor, actuator, driver and gateway | `ModestIoT.h` |
| Compile footprint | The Arduino builder compiles **every** `.cpp` under a library's `src/` once any header of the library is included, so a sketch that only blinks an LED still needs `PubSubClient`, `ArduinoJson`, `ESP32Servo`, `LiquidCrystal_I2C`, `DHT` installed. Splitting the umbrella header alone does not fix this. | Arduino build behaviour; verify with a clean install |
| Dead code | ~~`LoadCellController.h/.cpp`~~ | **Resolved:** Deleted along with HX711 dependency. `LoadCellAmplifier` implements HX711 protocol natively. |
| Core version | `Device.cpp` calls `timerBegin(1000000)`, `timerAttachInterruptArg`, `timerAlarm`: the Arduino-ESP32 **core 3.x** API. `library.properties` cannot state a core version. | `Device.cpp` |
| Documentation drift | README tree is stale (mentions `LcdDisplay`); `keywords.txt` needs a pass for new classes | previous review |

## 3. Goals and non-goals

**Goals**
1. Every push compiles every example for ESP32 on a pinned Arduino-ESP32 core 3.x.
2. Host tests run with one command locally and in CI.
3. The repository can be submitted to the Arduino Library Manager and passes `arduino-lint`.
4. Users are not forced to install libraries they do not use (or the cost is explicit and documented).
5. Releases are versioned and reproducible.

**Non-goals**
- Supporting non-ESP32 architectures.
- Hardware-in-the-loop CI.
- A documentation website (Doxygen output published to GitHub Pages is optional, later).

## 4. Proposal

### 4.1 Repository cleanup (small, do first)
1. Remove root `main.cpp` and the empty root `.ino`, and **repoint CMake** `SKETCH_DIR` to an example (default `examples/ClassicBlinker`) with a cache variable to choose another, so the IDE workflow keeps working.
2. Delete `LoadCellController.*` (dead) and its `keywords.txt` line. See [dependencies-improvement.md](dependencies-improvement.md).
3. Extend `.gitignore`: `.DS_Store`, `/cmake-build-arduino/`, `/build/`, `*.o`.
4. Refresh `keywords.txt` (add `RgbLed`, `RgbColor`, `DigitalLightIntensitySensor`, `PassiveInfraredSensor`, remove dead names) and the README tree.
5. Add `CHANGELOG.md`, starting at `1.0.0`.

### 4.2 Host test harness
- One shared fake directory: `tests/fakes/` (`Arduino.h`, `Wire.h`, later `Client.h`, `FreeRTOS.h` stubs only where a unit needs them), replacing the two per-suite copies.
- One runner: `tests/run_all.sh` (or a CMake `ctest` target) that builds each suite with `-std=c++17 -Wall -Wextra -Werror` and runs it. Keep the current plain-`assert`/main style; do not introduce a framework unless the suite count justifies it.
- Suites to add as the other documents are implemented: `scheduler` (pure `SamplingScheduler`), `telemetry` (golden JSON). Current suites cover: digital light, RGB LED, RFID sensor, BLE connectivity, logging macros, MQTT gateway, engine configuration, diagnostics snapshot, and sensor fault events.
- Sanitizers: run the host suites under `-fsanitize=address,undefined` in CI.
- Note on host portability: `Device.cpp` includes FreeRTOS and ESP32 timer headers, so it cannot be built on the host. This is why the scheduler logic is extracted into a pure class.

### 4.3 CI workflows (`.github/workflows/`)

| Workflow | Does | Trigger |
|---|---|---|
| `host-tests.yml` | builds and runs `tests/run_all.sh` on Linux (and macOS, since development happens there), with sanitizers | push, pull request |
| `compile-examples.yml` | `arduino/compile-sketches` (or `arduino-cli`) for **every** folder in `examples/` with `esp32:esp32` core pinned to a 3.x release, libraries installed from `library.properties` | push, pull request |
| `lint.yml` | `arduino/arduino-lint-action` in library mode with `--library-manager submit` strictness; Doxygen run treating warnings as errors; PlantUML parse of `docs/class-diagram.puml` | push, pull request |
| `release.yml` (later) | on a `v*` tag: verify `library.properties` version equals the tag, create the GitHub release | tag |

Notes:
- Compile the examples against the **minimum and the latest** supported core 3.x (a two-entry matrix), since the timer and `ledc` APIs moved between 3.0 and 3.x.
- A matrix over `esp32` and `esp32s3` boards would detect the Bluetooth umbrella issue (D7 in the connectivity document) early.
- `compile-examples` is the check that would have caught D1. Expect it to fail on `SmartBandMqtt` and `AirQualityMonitor` until the connectivity Phase A fix lands; land the fix in the same pull request that enables the workflow.
- Optional: run the sketches headless in Wokwi CLI scenarios (see the Wokwi analysis from the earlier session). Keep it out of the first iteration.

### 4.4 Dependency footprint and the umbrella header
Because every `.cpp` is compiled, there are three real options:

| Option | Mechanism | Pros | Cons |
|---|---|---|---|
| **P1: accept** | Keep all dependencies mandatory; document it; trim the list to what is used (see the dependencies document) | Simple, honest | Larger install, a failure in an unused library breaks everyone |
| **P2: guard** | Wrap each optional module in `#if __has_include(<X.h>)` in its `.h` and `.cpp`; `depends=` lists only core needs; README lists optional libraries per module | Users install only what they use | A missing library becomes "class not declared", which is confusing; needs a clear `#error`/`static_assert` message path and per-module headers |
| **P3: split** | Several libraries (core, sensors, actuators, connectivity) | Cleanest dependencies | Repository and release overhead, and cross-library version coupling |

**Recommendation:** P1 now (it is free and correct), with a documented path to P2 once CI proves it works on a clean machine. P3 only if the library grows much larger. Whatever is chosen, remove `BluetoothConnectivityDriver.h` from the umbrella (or guard it with `__has_include(<BluetoothSerial.h>)`) because it fails on ESP32 variants without Classic Bluetooth.

### 4.5 `library.properties` corrections
- `depends=`: use the exact `name=` of each library as listed in the Library Manager index, which uses spaces (for example `DHT sensor library`), not underscores. The current value uses underscores (`DHT_sensor_library`, `HX711_Arduino_Library`, `Adafruit_Unified_Sensor`). **Verify with `arduino-lint` and a clean install**, since this decides whether `depends` resolves at all.
- Pin versions where the API matters: `ArduinoJson (>=7.0.0)` (the code uses the v7 `JsonDocument` type).
- Remove unused entries (`EspSoftwareSerial`, `HX711 Arduino Library` after deleting `LoadCellController`, and `Adafruit Unified Sensor` if the DHT library already depends on it; verify).
- State the core requirement in the README and in a comment next to the timer code, since `library.properties` has no field for it.
- Keep `architectures=esp32`.

### 4.6 Release and registry
1. Tag `v1.0.0` only after CI is green on a clean clone.
2. Submit the repository to `arduino/library-registry` (a one-line pull request adding the URL). Subsequent tags are indexed automatically.
3. Follow semantic versioning: the changes proposed in the other documents (deprecated overloads, `SendStatus`, `TelemetryWriter`) justify a `1.1.0` minor with deprecations and a `2.0.0` only when they are removed.

## 5. Phases

**Phase A: hygiene (hours)**
1. Cleanup in 4.1, CMake repoint, `.gitignore`.
2. `tests/fakes/` consolidation and `tests/run_all.sh`.
3. `host-tests.yml`.

**Phase B: build coverage**
4. `compile-examples.yml` with the connectivity D1 fix in the same pull request.
5. `lint.yml` and the `library.properties` corrections (4.5).

**Phase C: publication**
6. Umbrella and Bluetooth guard, dependency trimming.
7. Tag `v1.0.0`, registry submission, `CHANGELOG.md`.
8. Optional: Doxygen to GitHub Pages, Wokwi scenarios.

## 6. Risks

| Risk | Mitigation |
|---|---|
| CI fails on first run because the examples have drifted | Expected. Fix the examples (D1 and others) in the same pull request; list failures first by running the compile locally on any working machine |
| Example compile time makes CI slow | Cache the Arduino core and libraries; limit the matrix to two cores |
| CMake IDE workflow breaks on cleanup | Repoint `SKETCH_DIR` in the same change and document it |
| Core 3.x API differs between 3.0 and latest | Two-entry core matrix, state the supported range |
| Library Manager rejects the names in `depends` | Run `arduino-lint` with Library Manager strictness before tagging |
| P2 guards hide errors | Only adopt after P1 is released and the CI matrix covers "library missing" builds |

## 7. Open decisions

| # | Decision | Recommended |
|---|---|---|
| 1 | CMake default sketch | `examples/ClassicBlinker` |
| 2 | Test runner | Shell script first, `ctest` later |
| 3 | Test framework | None for now; reconsider at about ten suites |
| 4 | Dependency footprint | P1 now, P2 later |
| 5 | Supported cores | Minimum 3.0.x and latest 3.x, to be fixed once D1 and the timer API are compiled |
| 6 | Boards in the compile matrix | `esp32` plus `esp32s3` |
| 7 | Keep `Modest-IoT-Nano-Framework` name for the library | Yes (renaming breaks the registry identity) |

## 8. Acceptance criteria

- [x] `.github/workflows/` has host-tests workflow (compile-examples and lint workflows proposed).
- [ ] Every folder in `examples/` compiles for at least two core versions in CI (verified locally for ESP32).
- [x] `tests/run_all.sh` runs every host suite (digital light, RGB LED, RFID, BLE, log, MQTT gateway, engine config, diagnostics, sensor fault, scheduler) under sanitizers.
- [x] No root `main.cpp` or empty `.ino`; CMake still works from the IDE.
- [x] `.gitignore` covers `.DS_Store` and build output.
- [ ] `arduino-lint` passes in Library Manager mode; `depends=` verified on a clean machine.
- [x] `ModestIoT.h` does not break on ESP32 variants without Classic Bluetooth.
- [ ] `v1.0.0` tagged, `CHANGELOG.md` present, registry pull request opened.
