#ifndef TELEMETRY_BUFFER_H
#define TELEMETRY_BUFFER_H

/**
 * @file TelemetryBuffer.h
 * @brief Declares a persistent telemetry buffer using LittleFS for offline replay.
 * @ingroup core_module
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-11-14
 * @version 1.0
 */

#include <Arduino.h>
#include <cstdint>
#include <functional>

/**
 * @brief Maximum number of buffered packets before applying oldest-packet eviction.
 *
 * Each buffered packet is stored as a separate JSON file in LittleFS.
 * Default of 50 packets provides ~25-50 KB of buffer depending on payload size.
 */
constexpr int DEFAULT_MAX_BUFFERED_PACKETS = 50;

/**
 * @brief LittleFS directory path for buffered telemetry packets.
 */
constexpr auto TELEMETRY_BUFFER_DIRECTORY = "/telemetry_buffer";

/**
 * @brief A claimed packet and its stable local spool identity.
 *
 * A claim does not remove a packet. The caller must acknowledge this same claim
 * only after its transport confirms delivery.
 */
struct TelemetryPacketClaim {
    uint32_t packetId = 0;
    String payload;
    bool valid = false;
};

/**
 * @brief Persistent telemetry buffer backed by LittleFS for offline replay.
 * @ingroup core_module
 *
 * When the network is unavailable, telemetry packets that fail to send can be
 * persisted to LittleFS instead of being dropped. When connectivity is restored,
 * buffered packets are replayed in FIFO order.
 *
 * The buffer uses a file-per-packet strategy with monotonic indices
 * (e.g., `/telemetry_buffer/00001.json`, `/telemetry_buffer/00002.json`, etc.)
 * for FIFO ordering. New packets are appended with incrementing indices,
 * and acknowledgment deletes only the file associated with that claim — no
 * file shifting is needed. A failed send leaves the packet in the spool.
 *
 * @par Thread Safety
 * All public methods are designed to be called from the network dispatcher task.
 * The buffer is not thread-safe across multiple tasks; use from a single context.
 *
 * @par Memory Usage
 * Each buffered packet consumes approximately 100-300 bytes in LittleFS depending
 * on payload size. With the default limit of 50 packets, expect ~5-15 KB usage.
 *
 * @par Example
 * @code
 * // Initialize the buffer in setup()
 * TelemetryBuffer::begin();
 *
 * // In processQueuedTelemetryData(), if send fails:
 * if (!gatewayClient.sendTelemetryRecord(*packet)) {
 *     TelemetryBuffer::store(packet->toJsonString());
 * }
 *
 * // When connectivity is restored, claim, send, then acknowledge:
 * TelemetryPacketClaim claim;
 * if (TelemetryBuffer::claimNext(claim) && send(claim.payload)) {
 *     TelemetryBuffer::acknowledge(claim);
 * }
 * @endcode
 */
class TelemetryBuffer {
public:
    /**
     * @brief Callback signature for packet eviction events.
     */
    using EvictionCallback = std::function<void()>;

private:
    static bool initialized; ///< Tracks whether begin() was called successfully.
    static int maxPacketsCount; ///< Maximum number of buffered packets before dropping oldest.
    static int packetCount; ///< Current number of buffered packets in LittleFS.
    static uint32_t nextWriteIndex; ///< Next 0-based monotonic packet filename index.
    static EvictionCallback onEvictionCallback; ///< Callback invoked when a packet is evicted.

    /**
     * @brief Generates the file path for a given monotonic index.
     * @param index Monotonic packet index.
     * @return String path like "/telemetry_buffer/00001.json".
     */
    [[nodiscard]] static String getFilePath(uint32_t index);

    /**
     * @brief Counts the existing buffered packets and finds the oldest index.
     * @return Number of valid packet files, or -1 if the directory cannot be read.
     *
     * On ESP32, this also updates nextWriteIndex to be one past the highest
     * existing index, ensuring new packets are appended without gaps.
     */
    [[nodiscard]] static int countPackets();

    /**
     * @brief Finds the file path of the oldest valid packet (lowest monotonic ID).
     * @return Path string of the oldest file, or empty string if buffer is empty.
     */
    [[nodiscard]] static String findOldestFilePath();

    /**
     * @brief Removes the oldest packet file from LittleFS.
     * @return `true` if a packet was removed, `false` if buffer was empty or removal failed.
     *
     * Uses monotonic indices — no file shifting is needed.
     */
    static bool removeOldestPacket();

public:
    /**
     * @brief Initializes the telemetry buffer in LittleFS.
     * @param maxBufferedPacketsCount Maximum number of packets to buffer (default: 50).
     * @return `true` if LittleFS was mounted without formatting and the buffer directory exists.
     *
     * Creates the buffer directory if it doesn't exist and counts any
     * remaining packets from a previous session (e.g., unexpected reboot).
     *
     * @note Mounting never formats automatically. A mount failure leaves user data untouched.
     *       Must be called before any other buffer operations.
     * Call once during device setup, after LittleFS is available.
     */
    static bool begin(int maxBufferedPacketsCount = DEFAULT_MAX_BUFFERED_PACKETS);

    /**
     * @brief Stores a serialized telemetry packet in the buffer.
     * @param jsonStringTelemetryPacket The JSON string representation of the telemetry packet.
     * @return `true` if the packet was stored, `false` if writing or required eviction fails.
     *
     * At capacity, the new file is fully written before the oldest is evicted;
     * a failed incoming write therefore cannot evict an existing packet first.
     * Evictions invoke the configured callback. A failed cleanup/removal is logged
     * and reported; the packet spool can temporarily exceed its configured cap
     * if the newly written file cannot be removed.
     *
     * @note Should be called when `sendTelemetryRecord()` returns false.
     */
    static bool store(const String& jsonStringTelemetryPacket);

    /**
     * @brief Claims the oldest regular packet file without removing it.
     * @param outClaim Receives the stable packet ID and payload.
     * @return `true` when a nonempty packet was claimed; `false` if empty or unavailable.
     *
     * Empty packet files are treated as unrecoverable and removed with a warning;
     * nonempty payloads are opaque and remain until acknowledged, even if the
     * sender rejects them.
     */
    static bool claimNext(TelemetryPacketClaim& outClaim);

    /**
     * @brief Removes only the exact packet represented by a claim.
     * @param claim The packet claim returned by claimNext().
     * @return `true` if that packet was removed; stale, duplicate, or mismatched
     *         acknowledgments return `false` and cannot remove a later packet.
     */
    static bool acknowledge(const TelemetryPacketClaim& claim);

    /**
     * @brief Retrieves the next buffered packet in FIFO order.
     * @param outJsonBufferedPacket Reference to receive the JSON string content.
     * @return `true` if a packet was retrieved and removed, `false` if empty or removal failed.
     *
     * Legacy destructive retrieval. New replay code should use claimNext() and
     * acknowledge() so failed delivery does not lose a packet.
     */
    static bool retrieveNext(String& outJsonBufferedPacket);

    /**
     * @brief Returns the number of buffered packets.
     * @return Current count of buffered packets in LittleFS.
     */
    [[nodiscard]] static int getBufferedPacketCount();

    /**
     * @brief Returns the maximum number of buffered packets.
     * @return Maximum packet count configured during begin().
     */
    [[nodiscard]] static int getMaxPacketsCount();

    /**
     * @brief Checks whether there are buffered packets available.
     * @return `true` if at least one packet is buffered.
     */
    [[nodiscard]] static bool hasBufferedPackets();

    /**
     * @brief Clears all buffered packets from LittleFS.
     * @return `true` if the directory was cleared successfully.
     *
     * Use this to discard all buffered packets (e.g., after a firmware update
     * where old schema packets should not be replayed).
     */
    static bool clear();

    /**
     * @brief Checks whether the buffer was initialized.
     * @return `true` if begin() was called successfully.
     */
    [[nodiscard]] static bool isInitialized();

    /**
     * @brief Registers a callback invoked when a packet is evicted due to buffer capacity.
     * @param callback The callback function to invoke on eviction.
     *
     * The callback is called after the oldest packet is removed but before the
     * new packet is stored, allowing monitoring of eviction events.
     *
     * @note Call `onEviction(nullptr)` to clear a previously registered callback.
     */
    static void onEviction(EvictionCallback callback);
};

#endif // TELEMETRY_BUFFER_H
