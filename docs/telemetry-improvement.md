# Improvement 3: Telemetry Packages

Analysis for a future development session. Status: **proposed, not implemented.** Memory and performance statements are architectural reasoning, not measurements.

Related: [sampling-interval-improvement.md](sampling-interval-improvement.md), [connectivity-improvement.md](connectivity-improvement.md).

---

## 1. Current design (verified from source)

`TelemetryPackage` (`src/TelemetryPackage.h`) is an abstract class:

```cpp
class TelemetryPackage {
public:
    virtual ~TelemetryPackage() = default;
    virtual void serialize(JsonDocument& destination) const = 0;
    JsonDocument serialize() const;      // convenience, returns by value
    String serializeAsString() const;    // JSON text
};
```

Lifecycle in the examples (`SmartBandMqtt`, `AirQualityMonitor`):

1. The application's mediator builds a subclass on the heap: `new SmartBandTelemetryPackage(...)`.
2. `enqueueTelemetryPayload(&ptr)` copies the **pointer** into a FreeRTOS queue (`xQueueSend`, zero timeout).
3. If the queue is full, the caller deletes the packet.
4. `NetworkWorker` receives the pointer, calls `processQueuedTelemetryData(ptr)`, then `delete`s it.
5. The gateway calls `serializeAsString()`, which builds a `JsonDocument`, serializes it into an Arduino `String`, and hands `c_str()` to MQTT or HTTP.

Ownership is by convention: whoever successfully enqueues transfers ownership, and the worker deletes.

## 2. Findings

### 2.1 Strengths
- Simple, easy to teach: one virtual method per application payload.
- Polymorphism lets gateways stay payload-agnostic.
- Data is a **snapshot** copied at creation, so the sampler never shares mutable state with the network task. This is the right isolation property.

### 2.2 Defects and risks

| # | Finding | Evidence | Impact |
|---|---|---|---|
| T1 | Per-packet heap allocation and free on two different tasks | `new` in the sampler task, `delete` in the network task | Heap fragmentation over long uptimes on a device that is expected to run for months. Also takes the allocator lock from the sampler task, which is the real-time path. |
| T2 | Raw owning pointer through a queue | `enqueueTelemetryPayload(TelemetryPackage**)` | Leak if the caller forgets the `delete` on failure (every example has to remember it). Double delete if the user also deletes after a successful enqueue. Nothing in the type system says who owns it. |
| T3 | Silent loss when the queue is full | Zero-timeout `xQueueSend`, failure only visible by return value | No counter, no log, no policy. Under network outage the **newest** data is the data that is dropped, which is usually the wrong choice for telemetry. |
| T4 | Three-stage JSON construction | `JsonDocument` built, then `String` built, then `c_str()` | Several allocations per send. With ArduinoJson 7 `JsonDocument` is heap-allocated and grows dynamically. `String` heap growth adds fragmentation. |
| T5 | `JsonDocument serialize() const` returns by value | `TelemetryPackage.h` | Works in ArduinoJson 7 (movable), but the library's `depends=` does not pin a version, and v6 has different types (`DynamicJsonDocument`). Public API is tied to a major version. |
| T6 | ArduinoJson leaks into the public interface | `TelemetryPackage.h`, and through `GatewayClient.h` | Users cannot choose another serializer (CBOR, Protobuf, CSV, plain `snprintf`). It also blocks the dependency replacement question analyzed earlier. |
| T7 | Format is hard-wired to JSON | Gateways call `serializeAsString()` | MQTT and HTTP in constrained networks often prefer compact binary. HTTP sets `Content-Type: application/json` unconditionally, so a different format cannot be advertised. |
| T8 | No metadata in the base class | No timestamp, device ID, sequence number, schema version | Each application re-invents these. Without a timestamp, a delayed or retried message loses its true time. The queue adds latency, so receive time is not sample time. |
| T9 | No priority or QoS notion | One queue, FIFO | Alarm events (gas detected) wait behind routine readings during congestion. |
| T10 | No backpressure toward producers | `enqueueTelemetryPayload` returns `bool` only | Producers cannot slow down or coalesce when the network is slow. |
| T11 | Non-trivial copy cost at creation | Each subclass stores fields, then `serialize` copies them again into JSON | Acceptable for small payloads, wasteful for arrays (waveforms, scale arrays). |
| T12 | Payload size is unbounded | Nothing constrains the document size | An oversized message exceeds the 256-byte default `PubSubClient` buffer and the publish fails silently (see connectivity analysis, W10). |
| T13 | No host tests | `tests/` has only `digital_light` and `rgb_led` | Serialization output is the contract with a cloud backend and deserves golden tests. |
| T14 | Naming duplication | `serialize()` is overloaded three ways (`serialize(JsonDocument&)`, `serialize()`, `serializeAsString()`) | Slightly confusing, and the by-value overload hides the virtual one in derived classes that do not use `using`. |

Verify T1 and T4 with measurements before acting on them: capture `ESP.getFreeHeap()` and `ESP.getMaxAllocHeap()` over a long run.

### 2.3 Name hiding detail (T14)
A derived class that overrides `serialize(JsonDocument&)` hides the base's other `serialize()` overloads unless it adds `using TelemetryPackage::serialize;`. Calls through a base reference (as the gateways do) are unaffected, so the bug is latent and only appears when someone calls `packet.serialize()` on a concrete type.

## 3. Goals and non-goals

**Goals**
1. Predictable memory behaviour on long-running devices (no per-sample heap churn, or a bounded one).
2. Explicit ownership at the API level.
3. Decouple the payload from the serialization format and from ArduinoJson.
4. Carry standard metadata (timestamp, device ID, sequence).
5. Make drops observable and policy-driven.
6. Keep the "one class per payload" authoring experience simple.
7. Allow host-side golden tests.

**Non-goals**
- Designing a cloud schema or a vendor-specific format.
- Persistent (flash) buffering. Possible follow-up.
- Compression or encryption.

## 4. Options

### 4.1 Ownership

| Option | Description | Pros | Cons |
|---|---|---|---|
| **O1** | Keep raw pointers, add documentation and a helper that deletes on failure | Zero API change | Does not fix T2 |
| **O2** | `enqueueTelemetryPayload(std::unique_ptr<TelemetryPackage>)` | Ownership explicit, no leak on failure path | Queue still carries raw pointers (move in, release into queue). `unique_ptr` is available on ESP32 (C++11 and later). |
| **O3 (recommended)** | Fixed-size **packet pool** (see 4.2) with an RAII handle | No heap churn, bounded, explicit ownership | More work |

### 4.2 Allocation strategy

| Option | Description | Pros | Cons |
|---|---|---|---|
| **A1** | Status quo (`new` and `delete`) | Simple | T1 |
| **A2** | Static pool of N slots of a fixed maximum size, slot index passed through the queue | No fragmentation, deterministic, drops are explicit when the pool is exhausted | Fixed maximum payload size, need a size constant per device |
| **A3** | Placement-new into a pool, with typed `acquire<T>()` | Keeps polymorphism and type safety with no heap | Pool slot size must cover the largest `T` (static assert on `sizeof(T)`) |
| **A4** | Replace polymorphic packets with a plain POD "reading" struct plus a serializer function | Smallest and fastest | Loses the "one class per payload" authoring style, breaks examples |

**Recommended: A3.** Preserve the polymorphic model but allocate from a pool:

```cpp
template <size_t SlotBytes, uint8_t SlotCount>
class TelemetryPool {
public:
    template <typename T, typename... Args>
    TelemetryHandle<T> acquire(Args&&... args);  // placement-new, or empty handle when exhausted
};
```

A static assertion (`static_assert(sizeof(T) <= SlotBytes)`) turns an oversized payload into a compile error rather than a runtime failure.

### 4.3 Decoupling serialization

Introduce a format-neutral output interface so a package writes to a "sink" rather than to a `JsonDocument`:

```cpp
class TelemetryWriter {            // abstract: JSON, CBOR, key=value text
public:
    virtual void field(const char* name, int32_t v) = 0;
    virtual void field(const char* name, float v) = 0;
    virtual void field(const char* name, bool v) = 0;
    virtual void field(const char* name, const char* v) = 0;
};
class TelemetryPackage {
public:
    virtual void write(TelemetryWriter& out) const = 0;
};
```

Pros: ArduinoJson becomes an implementation detail of one writer (`JsonTelemetryWriter`), so it can be swapped, replaced, or dropped. The `Content-Type` can come from the writer (`application/json`, `application/cbor`). A bounded writer can write straight into a caller-supplied `char[]` and avoid the `JsonDocument` plus `String` double allocation (T4).

Cons: Breaks every existing package subclass (a mechanical migration: the field assignments become `out.field("name", value)`). Nested objects and arrays need `beginObject`, `beginArray` calls.

Compatibility option: keep `serialize(JsonDocument&)` as a deprecated path that an adapter wraps, so old examples continue to build for one release.

### 4.4 Metadata

Add to the envelope (not to each subclass):

| Field | Source | Purpose |
|---|---|---|
| `deviceId` | `Device` or gateway configuration | Identify the source |
| `sequence` | Monotonic counter per device | Detect loss and reordering on the backend (turns silent drops into measurable gaps) |
| `timestamp` | Capture time (`millis()` plus an offset, or RTC/NTP if available) | Preserve sample time through queue delay |
| `schemaVersion` | Constant on the subclass | Backend compatibility |

The envelope should be added at **enqueue time**, not serialization time, because the queue delay is exactly the error being corrected. A `time source` abstraction (`millis()` default, NTP optional) keeps it testable.

### 4.5 Queue policy

| Policy | Behaviour |
|---|---|
| Drop newest (today) | Cheap, loses latest data |
| Drop oldest | Keeps the freshest data, needs a queue that supports overwrite (`xQueueOverwrite` for length 1, or manual ring buffer for N) |
| Priority lanes | Two queues (alarm and routine), the worker drains alarms first |
| Coalesce | Replace a pending packet of the same type with the latest value |

Recommended default: **drop oldest** for routine readings, with a separate **alarm lane that is never dropped before routine data**. Expose `droppedCount` and `highWaterMark` counters, and an optional callback or event when the first drop occurs.

### 4.6 Serialization buffers

For the JSON writer, serialize directly into a fixed buffer with `serializeJson(doc, buffer, size)` or `measureJson` first, and fail with a distinct status (`PayloadTooLarge`) rather than silently truncating. This replaces the intermediate `String`.

## 5. Recommended path

**Phase A: safe, small**
1. Add counters (`enqueued`, `dropped`, high-water mark) to `Device`.
2. Add `using TelemetryPackage::serialize;` guidance and a note in Doxygen (T14).
3. Add golden host tests for each example package's JSON.
4. Compile the examples (shared with the connectivity plan).

**Phase B: ownership and memory**
5. `unique_ptr`-based enqueue overload, keeping the raw overload for compatibility. Measure heap before and after on hardware.
6. Packet pool with placement-new and a `static_assert` on size.
7. Drop-oldest and an alarm lane.

**Phase C: format decoupling**
8. `TelemetryWriter` interface with a JSON implementation, plus an adapter for the legacy `serialize(JsonDocument&)`.
9. Move the `ArduinoJson` include out of public headers.
10. Envelope metadata (`deviceId`, `sequence`, `timestamp`).
11. Optional compact writer (CBOR or key=value) and `Content-Type` negotiation in the gateways.

## 6. Interaction with the other improvements

| Other improvement | Interaction |
|---|---|
| Sampling intervals | Faster per-sensor rates raise the producer rate, so the pool, queue policy and counters matter more. Hold off on high-rate defaults until drops are observable. |
| Connectivity | `SendStatus` and the failure policy decide whether a packet is retried or dropped. The pool must release a slot on every path. `PayloadTooLarge` is shared. Content type comes from the writer. Gateways should stop including `ArduinoJson.h`. |
| Dependency replacement | A `TelemetryWriter` removes the main structural obstacle to replacing ArduinoJson with a small in-house writer. Verify the claim by checking every file that includes `ArduinoJson.h`. |

## 7. Risks

| Risk | Mitigation |
|---|---|
| Breaking every package subclass | Adapter for the old `serialize(JsonDocument&)`, a deprecation window, migration notes in the README |
| Pool slot size too small or wasteful | `static_assert` on `sizeof`, a documented formula, a template parameter |
| Placement-new lifetime mistakes | Handle type with a move-only RAII wrapper, tests with a counting fake type |
| Timestamps wrong without a clock | Default to `millis()` relative time and label it clearly |
| Over-engineering for small devices | Keep Phase A and the legacy path as the default, and make the pool opt-in |
| Cross-task data races | Packets are immutable after enqueue, the pool uses queue indices only. Document it. |

## 8. Open decisions

| # | Decision | Recommended |
|---|---|---|
| 1 | Ownership type | `unique_ptr` first, pool handle later |
| 2 | Pool vs heap | Pool opt-in, with heap as the default until measured |
| 3 | Writer abstraction | Yes, with a JSON writer and a legacy adapter |
| 4 | Metadata placement | Envelope added at enqueue time |
| 5 | Queue drop policy default | Drop oldest plus an alarm lane |
| 6 | Time source | `millis()` default, pluggable |
| 7 | Keep ArduinoJson as a dependency | Yes for now, but behind the writer |
| 8 | Maximum payload size | Compile-time constant per device |

## 9. Verification

- Host tests (no hardware): golden JSON for each packet type, pool exhaustion, handle release on every path, drop-oldest ordering, counters.
- On an ESP32: log `ESP.getFreeHeap()` and `ESP.getMaxAllocHeap()` every minute for 24 hours, comparing the heap strategy against the pool. Measure packet latency and the number of drops under a simulated broker outage.
- Wokwi can run the logic, but not the memory or timing behaviour reliably.

## 9a. Acceptance criteria

- [ ] Drops and high-water mark are observable at runtime.
- [ ] No raw owning pointer is required in application code for new examples.
- [ ] A packet pool exists, and an oversized payload fails at compile time.
- [ ] Public headers no longer require `ArduinoJson.h` (optional, Phase C).
- [ ] Packets carry `sequence` and capture-time `timestamp`.
- [ ] Golden host tests for every example payload pass.
- [ ] README, class diagram, `keywords.txt` and Doxygen are updated.
