# Improvement 8: Third-Party Dependency Strategy

Status: **proposed, not implemented.** Complements the footprint options P1 to P3 in [library-readiness-improvement.md](library-readiness-improvement.md). Cost and risk figures are estimates; none were measured.

Related: [improvements-roadmap.md](improvements-roadmap.md), [telemetry-improvement.md](telemetry-improvement.md) (ArduinoJson), [connectivity-improvement.md](connectivity-improvement.md) (PubSubClient).

---

## 1. Inventory (verified from `src/` and `library.properties`)

| Dependency | Used by | Declared in `depends=` | Notes |
|---|---|---|---|
| `LiquidCrystal_I2C` | `CharacterLcdDisplay` | yes | Many forks with differing APIs |
| `DHT sensor library` | `DhtSensor` | yes (as `DHT_sensor_library`) | Pulls in Adafruit Unified Sensor transitively (verify) |
| `ESP32Servo` | `ServoMotor` | yes | |
| `PubSubClient` | `MqttGatewayClient` | yes | |
| `ArduinoJson` | `TelemetryPackage` (needed); `GatewayClient.h`, `MqttGatewayClient.cpp` (unneeded includes) | yes, version not pinned | Code uses the v7 `JsonDocument` type |
| `HX711` | `LoadCellController` only | yes | **Dead code**: `LoadCellController` is not in `ModestIoT.h` or any example |
| `EspSoftwareSerial` | nothing | yes | Not referenced anywhere in the repository |
| `Adafruit_Unified_Sensor` | nothing directly | yes | Likely transitive via DHT; verify |

Core libraries (no install): `WiFi`, `HTTPClient`, `BluetoothSerial`, `Wire`, `HardwareSerial`.

Note: `LoadCellAmplifier` already implements the HX711 protocol natively (bit-banged clocking, busy-wait for ready). Earlier advice to "replace the HX711 library" is therefore obsolete; the action is deletion.

## 2. Evaluation matrix

| Dependency | Replace? | Removal cost | Risk | Recommendation |
|---|---|---|---|---|
| `HX711` | Delete the only user | Tiny: remove `LoadCellController.*`, the `keywords.txt` entry, and the dependency | Very low | **Do now** |
| `EspSoftwareSerial` | Drop | Tiny | Very low, after a clean-build check | **Do now** |
| `Adafruit_Unified_Sensor` | Drop from `depends=` if DHT declares it | Tiny | Low; verify with a clean install | **Do now, after verifying** |
| `ArduinoJson` unused includes | Remove from `GatewayClient.h`, `MqttGatewayClient.cpp` | Tiny | Very low | **Do now** |
| `ArduinoJson` pin | Pin `>=7` in `depends=` | Tiny | Low | **Do now** |
| `ArduinoJson` replacement | No: keep behind a telemetry writer interface | Medium | Escaping and number formatting bugs | **Keep**; see telemetry document |
| `DHT sensor library` | No | About 150 lines for a timing-critical protocol | Timing bugs, per-model quirks (DHT11 versus DHT22) | **Keep** |
| `PubSubClient` | No | Large (MQTT client state machine, keep-alive, QoS) | High | **Keep**; make it optional (P-options) |
| `ESP32Servo` | Possible | About 60 to 100 lines using `ledcAttach` and `ledcWrite` at 50 Hz with a 500 to 2500 microsecond pulse range | Low to medium; the LEDC API differs between core 2.x and 3.x | **Optional**, behind a thin `ServoDriver` seam |
| `LiquidCrystal_I2C` | Possible | About 100 lines for an HD44780 through a PCF8574 backpack (4-bit mode, init sequence, timing) | Low to medium; backpack pin maps and 3.3 V versus 5 V behaviour vary | **Optional**, behind a thin seam; benefit is avoiding fork fragmentation |
| `Wire`, `WiFi`, `HTTPClient` | Core | n/a | n/a | Keep |
| `BluetoothSerial` | Core, but not on all chips | Make optional | Fails on ESP32-S2, S3 and C3 (verify) | See connectivity D7 |

## 3. Principles

1. **Remove what is unused first.** It costs nothing and cuts install friction.
2. **Replace only simple, well-specified protocols**, where a native implementation is a few dozen lines, is testable on the host, and the third-party library gives little beyond API familiarity.
3. **Never replace protocols with timing risk or large state machines** (DHT, MQTT, TLS, JSON).
4. **Always wrap before replacing.** Introduce a small internal interface first, so users can swap implementations; do the replacement only if it pays off.
5. **Library Manager friction counts.** Each extra `depends=` entry is auto-installed for every user, so a smaller list directly improves first-run success.

## 4. Plan

**Phase A: quick wins (low risk, high value)**
1. Delete `LoadCellController.h/.cpp` and its `keywords.txt` lines; drop `HX711` from `depends=`.
2. Drop `EspSoftwareSerial` after a clean compile of every example.
3. Verify and, if transitive, drop `Adafruit_Unified_Sensor`.
4. Remove unneeded `ArduinoJson.h` includes.
5. Pin ArduinoJson to a v7 range.
6. Verify the exact `depends=` names with `arduino-lint` and a clean-machine install (underscores versus spaces).

**Phase B: seams (only if Phase C is wanted)**
7. Wrap the servo pulse generation and the LCD bus writes behind small internal functions with a host fake. This also strengthens host tests.

**Phase C: optional replacements**
8. Native servo using LEDC, validated on core 3.x. Keep `ESP32Servo` selectable behind a macro for one release.
9. Native PCF8574 LCD driver. Keep the same public `CharacterLcdDisplay` API.
10. Document the trade-off and the macro switches in the README.

## 5. Risks

| Risk | Mitigation |
|---|---|
| A "removed" dependency was transitive | Compile every example on a clean machine (CI, library-readiness document) |
| LEDC API differences across cores | Target core 3.x only, matching the timer API already in use; document it |
| LCD timing or contrast differences | Keep the third-party path selectable for a release |
| Maintenance burden of owned drivers | Limit to the two small drivers; add host tests |

## 6. Open decisions

| # | Decision | Recommended |
|---|---|---|
| 1 | Do Phase A | Yes, immediately |
| 2 | Replace the servo and LCD libraries | Defer; wrap first, decide with data |
| 3 | Keep PubSubClient and DHT | Yes |
| 4 | ArduinoJson | Keep, pinned, behind a writer interface |
| 5 | Optional-dependency approach | P1 from the library-readiness document |

## 7. Acceptance criteria

- [ ] `depends=` lists only libraries that `src/` actually uses, with names validated by `arduino-lint`.
- [ ] `LoadCellController` and its keywords are gone, and `LoadCellAmplifier` examples still compile.
- [ ] No unused third-party `#include` remains in `src/`.
- [ ] A clean-machine install compiles all examples.
- [ ] If Phase C is done: host tests cover the servo pulse math and the LCD byte sequences.
