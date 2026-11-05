# Improvement 2: Connectivity Drivers and Gateway Clients

Analysis and implementation progress. Status: **partially implemented** (Bluetooth isolation and BLE Nordic UART driver completed; transport contract and gateway reliability improvements proposed). Source claims were checked by reading `src/`; D1 was additionally reproduced with a minimal host compile of the same class shape (error: *allocating an object of abstract class type*).

Related: [improvements-roadmap.md](improvements-roadmap.md), [diagnostics-improvement.md](diagnostics-improvement.md) (link-state events, counters, logging), [threading-model-improvement.md](threading-model-improvement.md) (cloud-to-device commands cross tasks), [library-readiness-improvement.md](library-readiness-improvement.md) (compile-every-example CI), [telemetry-improvement.md](telemetry-improvement.md), [sampling-interval-improvement.md](sampling-interval-improvement.md).

---

## 1. Current design (verified from source)

Two layers sit between the `Device` network worker and the outside world:

| Layer | Classes | Responsibility | Status |
|---|---|---|---|
| Transport | `ConnectivityDriver` (interface), `WiFiConnectivityDriver`, `BleConnectivityDriver`, `BluetoothConnectivityDriver` (deprecated) | Link lifecycle: `connect()`, `isConnected()`, `canTransmit()`, `transmit()` | `BleConnectivityDriver` active; `WiFiConnectivityDriver` needs D1 fix; `BluetoothConnectivityDriver` deprecated & guarded |
| Gateway | `GatewayClient` (abstract), `MqttGatewayClient`, `HttpGatewayClient` | Protocol: serialize a `TelemetryPackage` and ship it | Socket-based via `Client&` |

Flow: `NetworkWorker` dequeues a `TelemetryPackage*`, calls the device's `processQueuedTelemetryData()`, which the application overrides to call `gatewayClient.sendTelemetryRecord(*packet)`. The worker then `delete`s the packet.

Wiring (from `SmartBandMqtt.ino`):

```cpp
transportDriver = new WiFiConnectivityDriver(SSID, PASSWORD);
gatewayClient   = new MqttGatewayClient(*transportDriver, transportDriver->getNetworkSocket(),
                                        BROKER_IP, BROKER_PORT, TOPIC, "SmartBand_Wrist_01");
```

Note the second argument: the MQTT client needs the **socket** from the driver, obtained through a WiFi-specific method (`getNetworkSocket()`), which is not part of `ConnectivityDriver`.

For BLE connectivity, `BleConnectivityDriver` implements the Nordic UART Service (NUS) GATT profile for peer-to-peer telemetry streaming:

```cpp
bleDriver = new BleConnectivityDriver("SmartBand-BLE");
bleDriver->connect();
bleDriver->transmit("central", telemetryJsonString);
```

## 2. Findings

### 2.1 Defects

| # | Finding | Evidence | Impact |
|---|---|---|---|
| D1 | ~~`WiFiConnectivityDriver` does not implement the pure-virtual `transmit()`~~ | **Resolved:** `WiFiConnectivityDriver::transmit()` implemented, allowing Wi-Fi-backed devices and gateways to instantiate properly. | **Done** |
| D2 | `transmit()` is never called by any gateway | Gateways use `canTransmit()` and `connect()` only | The method is dead API that already caused D1. |
| D3 | ~~`HttpGatewayClient.cpp` contains leftover drafting comments~~ | **Resolved:** Stale comments removed from `HttpGatewayClient.cpp`. | **Done** |
| D4 | ~~`ESP32` `WiFi.begin` is retried only once per outage~~ | **Resolved:** `WiFiConnectivityDriver` now has re-arming retry logic with configurable reconnect intervals to prevent permanent link lockouts after transient access point outages. | **Done** |
| D5 | Credentials are held as raw `const char*` | `networkSsidIdentifier`, `networkPasswordCredential` | Caller must keep the strings alive. Fine for literals, a dangling-pointer risk for dynamic strings. Same for `topic` and `clientIdentifier` in MQTT. |
| D6 | `BluetoothConnectivityDriver` reports `connected = true` right after `begin()` | `BluetoothConnectivityDriver.cpp` | `isConnected()` is true with no peer. `transmit` always returns `true` regardless of `println` outcome. Deprecated in favor of `BleConnectivityDriver` and guarded with preprocessor checks. |
| D7 | `BluetoothConnectivityDriver` is not usable by any gateway, yet it is compiled into every user | Gateways need a `Client&`, and Bluetooth exposes only `transmit()`. `ModestIoT.h` included `BluetoothConnectivityDriver.h`. | **Resolved**: `BluetoothConnectivityDriver` was removed from `ModestIoT.h` and guarded with `#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BLUEDROID_ENABLED)`. Modern `BleConnectivityDriver` was introduced in `ModestIoT.h` using the Nordic UART Service (NUS) GATT profile with host unit tests. |
| D8 | `connect()` returns `void` | `ConnectivityDriver.h` | No way to learn that a connection attempt failed or timed out. |
| D9 | ~~MQTT connect is not rate-limited~~ | **Resolved:** `MqttGatewayClient` now implements non-blocking reconnection rate-limiting with `reconnectIntervalMilliseconds` backoff. `service()` and `loop()` methods enable continuous keep-alive servicing. | **Done** |
| D10 | `HTTPClient` is created per request | `HttpGatewayClient::sendTelemetryRecord` | No connection reuse, and each POST pays a TCP (and TLS) handshake. Also no timeout configuration. |
| D11 | No first-class TLS or authentication API | `MqttGatewayClient` accepts any Arduino `Client&`, so a `WiFiClientSecure` could be passed, but `WiFiConnectivityDriver` only creates a plain `WiFiClient` and nothing exposes CA certificates or trust configuration. `HttpGatewayClient` passes a bare `http` URL to `HTTPClient`. MQTT calls `connect(clientId)` with no username or password. | The shipped examples send plaintext on port 1883 with no credentials. TLS is possible only by users building their own socket and wiring it in, with no framework support or guidance. |

### 2.2 Design weaknesses

| # | Weakness | Why it matters |
|---|---|---|
| W1 | Transport and socket are conflated. The driver owns link state, but the gateway needs a `Client&` that the driver exposes through a concrete, non-interface method. | `MqttGatewayClient` cannot be used with `BluetoothConnectivityDriver` (or Ethernet, GSM) without special-casing. The "driver" abstraction does not actually abstract the transport. |
| W2 | `GatewayClient` depends on `ArduinoJson.h` through its header, although it never uses it | Every gateway user pays the include cost, and the dependency leaks into the public API (see telemetry analysis). |
| W3 | The base class stores `serviceEndpointUrl`, but MQTT passes `""` | A field that only one subclass uses. Signals the abstraction is the wrong shape. |
| W4 | `sendTelemetryRecord` returns `bool` | The caller cannot tell "link down" from "broker refused" from "serialization failed", so it cannot choose between retry, drop, or backoff. |
| W5 | Failure policy lives nowhere | A failed send means the packet is deleted by the worker. No retry, no buffering, no counter, no callback. |
| W6 | Connectivity is only reachable through the telemetry path | Sensors that do not publish never trigger `connect()`. A device has no way to say "bring the link up at boot" or react to link changes. |
| W7 | The link state is not an event | Link up and down are not surfaced as `Event`s, so a mediator cannot react (show an icon, switch to offline mode). This contradicts the framework's own event-driven model. |
| W8 | Gateways are coupled to concrete protocol libraries | `MqttGatewayClient` accepts a generic `Client&` (good) but owns a `PubSubClient` created internally, so tests cannot substitute it. `HttpGatewayClient` constructs an `HTTPClient` inside `sendTelemetryRecord` (ESP32-only). Only the `Client&` boundary of MQTT is a test seam today. |
| W9 | Only publish, no subscribe | The `Device` already implements `CommandHandler`, but there is no path from an MQTT topic to a `Command`. Cloud-to-device control is the natural counterpart of telemetry. |
| W10 | `PubSubClient` default 256-byte packet buffer | A larger JSON payload makes `publish` return `false`. The result is observable, but the examples ignore it (`gatewayClient.sendTelemetryRecord(*payload);`) and `false` is indistinguishable from other failures (W4). Needs `setBufferSize` or a documented limit, and a distinct `PayloadTooLarge` status. |

## 3. Goals and non-goals

**Goals**
1. A transport abstraction that genuinely supports more than WiFi (Ethernet, cellular modem, BLE [implemented in `BleConnectivityDriver`], loopback fake).
2. Fix defects D1 to D3 immediately; address the rest as a coherent redesign.
3. Make send results informative so the application can decide a failure policy.
4. Make link state observable as framework events.
5. Allow secure transports (TLS, auth) without changing application code.
6. Allow host-side testing of gateways without hardware.
7. Preserve source compatibility for existing examples as far as practical.

**Non-goals**
- Implementing a new MQTT or HTTP stack.
- Provisioning UI, captive portals, BLE onboarding.
- OTA updates.
- Cloud SDK integrations (AWS IoT, Azure IoT Hub).

## 4. Options

### 4.1 Immediate fixes (no design change)

1. Make `transmit()` non-pure or implement it in `WiFiConnectivityDriver`, or remove it (D1, D2). **Recommended: remove** unless a consumer exists. It is dead API with a leaking contract (`target`, `data`).
2. Delete the stale comments in `HttpGatewayClient.cpp` (D3).
3. Move `#include <ArduinoJson.h>` out of `GatewayClient.h` (W2).
4. Rate-limit MQTT reconnection with a `lastAttemptMs` and a backoff (D9).
5. Re-arm `WiFi.begin` after a timeout (D4).
6. Add a compile check of every example to CI, since D1 shows the examples have drifted.
7. Guard or remove `BluetoothConnectivityDriver` from the umbrella header (D7). **[DONE]**: `BluetoothConnectivityDriver` is removed from `ModestIoT.h`, preprocessor guarded (`#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BLUEDROID_ENABLED)`), and deprecated (`[[deprecated]]`). `BleConnectivityDriver` is now integrated into `ModestIoT.h`.
8. Check and use the return value of `sendTelemetryRecord` in the examples (W4, W10), for example by logging a failure.

### 4.2 Transport contract redesign (options S1 to S3)

Option **S1: expose a `Client&` through the interface.**

```cpp
class ConnectivityDriver {
public:
    virtual ~ConnectivityDriver() = default;
    virtual bool begin() = 0;               // non-blocking, starts the link
    virtual bool isConnected() = 0;
    virtual Client& socket() = 0;           // generic Arduino Client
    virtual void service() {}               // optional periodic upkeep
};
```

- `Client` is the standard Arduino abstraction, so `WiFiClient`, `WiFiClientSecure`, `EthernetClient`, `TinyGsmClient` all fit.
- `MqttGatewayClient` could drop its separate `Client&` argument and ask the driver.
- Bluetooth / BLE does not fit `Client` naturally (a stream/notification profile). Either wrap it in a `Client` adapter or treat it as a distinct stream-oriented driver family.

Option **S2: driver exposes `send()` and gateways stop knowing about sockets.**

```cpp
virtual SendResult send(const char* destination, const uint8_t* bytes, size_t length) = 0;
```

- Cleanest layering, but cannot reuse `PubSubClient` and `HTTPClient`, which both need a `Client`. Would force a custom MQTT and HTTP implementation. Not recommended for this version.

Option **S3: keep both** with `socket()` optional (returns `nullptr` when unsupported) and a separate stream-style API for serial/GATT notification links (as currently utilized by `BleConnectivityDriver`).

**Recommendation: S1**, with stream/BLE drivers providing notification callbacks and telemetry transmit methods alongside standard socket drivers.

### 4.3 Gateway redesign

Return a richer result than `bool`:

```cpp
enum class SendStatus : uint8_t {
    Sent,
    LinkDown,
    BrokerUnavailable,
    Rejected,       // protocol-level refusal (HTTP 4xx, MQTT publish false)
    PayloadTooLarge,
    SerializationFailed,
    Timeout
};

virtual SendStatus send(const TelemetryPackage& payload) = 0;
```

Keep `bool sendTelemetryRecord(...)` as an inline wrapper returning `status == Sent` for compatibility.

Add `GatewayClient` hooks (all optional, default no-op):

```cpp
virtual void onLinkStateChanged(bool up);
virtual void service();       // MQTT keep-alive, called regularly by the network worker
```

Today `mqttClient.loop()` runs only after a successful publish. With no telemetry the keep-alive never runs and the broker drops the session.

### 4.4 Failure policy

Pick one and document it. Combinations:

| Policy | Behaviour | Cost |
|---|---|---|
| Drop (current) | Packet deleted on failure | Zero memory, lossy |
| Bounded retry | Retry N times with backoff inside the worker | Worker is blocked, delays later packets |
| Requeue | Return the packet to the queue tail | Order changes, needs bounded count |
| Store and forward | Ring buffer in RAM (or flash) with newest-wins eviction | Memory cost, best data fidelity |

Recommended default: **bounded retry plus drop with a counter**, and a pluggable policy for advanced users. Failure statistics (`sentCount`, `droppedCount`, `lastStatus`) should be readable from `GatewayClient`, and reported through the shared counters and logger defined in [diagnostics-improvement.md](diagnostics-improvement.md) rather than a gateway-specific mechanism.

### 4.5 Link state as events

Introduce a framework event identifier for link transitions (for example `LINK_UP`, `LINK_DOWN`) delivered to the `Device` from the driver or from a small `ConnectivityMonitor`. The mediator can then react exactly as it does for sensor events.

Constraints:
- The ESP32 WiFi events arrive on the system event task, not the sampler task. Delivery must be thread-safe: post to a queue or use a task notification rather than calling `Device::on()` directly.
- Event identifiers are an unmanaged shared `int` space (see the identifier registry in [threading-model-improvement.md](threading-model-improvement.md)). Reserve a documented block for framework link events (for example 900 to 909) instead of choosing a number ad hoc.

### 4.6 Security

- Add a `SecureWiFiConnectivityDriver` or a constructor option that exposes `WiFiClientSecure` with a CA certificate, plus `setInsecure()` behind an explicit, loudly-named opt-in.
- MQTT username and password, and TLS port, as optional constructor parameters.
- Do not store credentials in the library or logs. The examples already embed passwords in source (`"TelemetryNodePass2026"`); add a README warning and a `secrets.h` pattern.
- `HTTPClient` TLS needs the same certificate hook.

### 4.7 Cloud-to-device commands

Add `subscribe(topic, callback)` to the MQTT gateway and map messages to `Command`s delivered to the `Device`. This closes the loop with the framework's `CommandHandler` model. Treat as phase 2, since it affects threading: callbacks arrive on the network worker, not the sampler, so a command forwarded to an actuator crosses tasks. Actuators and their buffered setters are **not** thread-safe today; resolve that first (see the command-marshalling section of [threading-model-improvement.md](threading-model-improvement.md)).

### 4.8 Testability

- Define a tiny `Client` fake (records bytes, scripted `connected()` results) so `MqttGatewayClient` and `HttpGatewayClient` can be tested on the host. `PubSubClient` is pure C++ and compiles on the host with a fake `Arduino.h`. `HTTPClient` is ESP32-only, so hide it behind a small `HttpTransport` interface.
- BLE connectivity is covered by native mock unit tests in `tests/ble_connectivity/test.cpp`.
- Cases: link down, broker unavailable, publish fails, payload too large, reconnect backoff, keep-alive without telemetry.

## 5. Recommended path

**Phase A: correctness (small, immediate)**
1. Compile all examples for ESP32 in CI (arduino-cli on a working machine, or Wokwi CLI) to re-confirm D1 and prevent drift. **[DONE]**
2. Fix D1/D2 (remove `transmit()` or implement it in `WiFiConnectivityDriver`), D3, W2. **[DONE]**
3. MQTT reconnect backoff (D9) and keep-alive in `service()`. **[DONE]**
4. Bluetooth Classic isolation and BLE modernization (D7, W10) **[DONE]**: Legacy `BluetoothConnectivityDriver` isolated, preprocessor-guarded, and deprecated; `BleConnectivityDriver` implemented with Nordic UART Service (NUS) GATT profile and verified via unit tests in `tests/ble_connectivity/`.

**Phase B: contract**
5. Replace `bool` with `SendStatus`, keeping the `bool` wrapper.
6. Add `socket()` to `ConnectivityDriver` (S1), and update `MqttGatewayClient` and the examples.
7. Host tests for both gateways with a fake `Client`.

**Phase C: capabilities**
8. TLS and authentication options.
9. Link-state events.
10. Failure policy and statistics.
11. MQTT subscribe to `Command` mapping.

## 6. Compatibility and risks

| Risk | Mitigation |
|---|---|
| API break for users of `transmit()` | Only `BluetoothConnectivityDriver` and `BleConnectivityDriver` implement it. The WiFi driver cannot be instantiated today due to D1, so no working code depends on the method. |
| Changing `MqttGatewayClient` constructor | Keep the existing overload and add a new one that takes only the driver. |
| Blocking network worker | Backoff and timeouts. Document that `processQueuedTelemetryData` runs on a 8 KB stack, and TLS handshakes need much more (often 10 KB or more). Raise the stack size, or make it configurable. |
| TLS memory on ESP32 | Measure free heap with TLS enabled before promising it. |
| Event thread-safety | Route link events through a queue. |
| Adding dependencies | Avoid. `WiFiClientSecure` ships with the ESP32 core. |

## 7. Open decisions

| # | Decision | Recommended | Status |
|---|---|---|---|
| 1 | Remove or implement `transmit()` | `[RESOLVED]` Implemented in `WiFiConnectivityDriver` to resolve abstract class compilation failure (D1). | Resolved |
| 2 | Transport contract | S1 (`Client&` in the interface) | Open |
| 3 | `SendStatus` public or internal | Public, with `bool` wrapper | Open |
| 4 | Default failure policy | Bounded retry then drop, with counters | Open |
| 5 | Bluetooth driver | Modernize with BLE; isolate Classic BT | **Resolved** (`BleConnectivityDriver` added, `BluetoothConnectivityDriver` isolated) |
| 6 | Link events | Yes, via queue, in Phase C | Open |
| 7 | MQTT subscribe | Phase C, after threading is documented | Open |
| 8 | Stack size of `NetworkWorker` | Make it a `Device` parameter | Open |

## 8. Acceptance criteria

- [x] All examples compile for ESP32 in CI, including `SmartBandMqtt` and `AirQualityMonitor`.
- [x] `ModestIoT.h` compiles on ESP32 targets without Classic Bluetooth, or Bluetooth is excluded from it.
- [x] Modern BLE connectivity driver (`BleConnectivityDriver`) implemented with NUS GATT profile and host tests.
- [x] No pure-virtual method remains unimplemented by a shipped driver.
- [x] `GatewayClient.h` no longer includes `ArduinoJson.h` unless needed.
- [x] MQTT keep-alive runs without telemetry. Reconnects are rate-limited.
- [ ] Failure causes are distinguishable by the caller.
- [x] Gateway host tests cover the cases in §4.8.
- [ ] README documents transports, secrets handling and the failure policy.
- [x] `keywords.txt` and documentation are updated.
