/**
 * @file TelemetryBuffer.cpp
 * @brief Implements persistent telemetry buffering using LittleFS for offline replay.
 * @ingroup core_module
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-11-14
 * @version 1.0
 */

#include "TelemetryBuffer.h"

#include <cstring>
#include <limits>

#ifdef ARDUINO_ARCH_ESP32
#include <LittleFS.h>
#endif

#include "ModestLog.h"

static constexpr auto TELEMETRY_BUFFER_COMPONENT = "TelemetryBuffer";

bool TelemetryBuffer::initialized = false;
int TelemetryBuffer::maxPacketsCount = DEFAULT_MAX_BUFFERED_PACKETS;
int TelemetryBuffer::packetCount = 0;
uint32_t TelemetryBuffer::nextWriteIndex = 0;
TelemetryBuffer::EvictionCallback TelemetryBuffer::onEvictionCallback = nullptr;

namespace {

bool parsePacketId(const char* fileName, uint32_t& packetId) {
    if (fileName == nullptr) {
        return false;
    }

    const char* baseName = std::strrchr(fileName, '/');
    baseName = baseName == nullptr ? fileName : baseName + 1;
    const size_t nameLength = std::strlen(baseName);
    if (nameLength <= 5 || std::strcmp(baseName + nameLength - 5, ".json") != 0) {
        return false;
    }

    const size_t digitsLength = nameLength - 5;
    uint64_t parsedId = 0;
    for (size_t index = 0; index < digitsLength; ++index) {
        const char digit = baseName[index];
        if (digit < '0' || digit > '9') {
            return false;
        }
        parsedId = parsedId * 10U + static_cast<uint64_t>(digit - '0');
        if (parsedId > std::numeric_limits<uint32_t>::max()) {
            return false;
        }
    }
    if (parsedId == 0) {
        return false;
    }

    packetId = static_cast<uint32_t>(parsedId);
    return true;
}

String getFullPath(const char* fileName) {
    if (fileName == nullptr) {
        return {};
    }
    if (std::strchr(fileName, '/') != nullptr) {
        return String(fileName);
    }
    String path(TELEMETRY_BUFFER_DIRECTORY);
    path += '/';
    path += fileName;
    return path;
}

} // namespace

String TelemetryBuffer::getFilePath(const uint32_t index) {
    String path(TELEMETRY_BUFFER_DIRECTORY);
    path += '/';
    // Zero-padded 5-digit minimum-width monotonic packet ID.
    char num[11];
    snprintf(num, sizeof(num), "%05lu", static_cast<unsigned long>(index + 1U));
    path += num;
    path += ".json";
    return path;
}

int TelemetryBuffer::countPackets() {
    int count = 0;
#ifdef ARDUINO_ARCH_ESP32
    uint32_t maxPacketId = 0;
    File dir = LittleFS.open(TELEMETRY_BUFFER_DIRECTORY, "r");
    if (!dir || !dir.isDirectory()) {
        return -1;
    }
    while (File file = dir.openNextFile()) {
        uint32_t packetId = 0;
        if (!file.isDirectory() && parsePacketId(file.name(), packetId)) {
            ++count;
            if (packetId > maxPacketId) {
                maxPacketId = packetId;
            }
        }
    }
    dir.close();
    if (maxPacketId > nextWriteIndex) {
        nextWriteIndex = maxPacketId;
    }
#endif
    return count;
}

String TelemetryBuffer::findOldestFilePath() {
    String oldestPath;
#ifdef ARDUINO_ARCH_ESP32
    uint32_t oldestPacketId = std::numeric_limits<uint32_t>::max();
    File dir = LittleFS.open(TELEMETRY_BUFFER_DIRECTORY, "r");
    if (!dir || !dir.isDirectory()) {
        return oldestPath;
    }
    while (File file = dir.openNextFile()) {
        uint32_t packetId = 0;
        if (!file.isDirectory() && parsePacketId(file.name(), packetId) &&
            packetId < oldestPacketId) {
            oldestPacketId = packetId;
            oldestPath = getFilePath(packetId - 1U);
        }
    }
    dir.close();
#endif
    return oldestPath;
}

bool TelemetryBuffer::removeOldestPacket() {
#ifdef ARDUINO_ARCH_ESP32
    const String path = findOldestFilePath();
    if (path.isEmpty()) {
        return false;
    }
    // Simply delete the oldest file — no shifting needed with monotonic indices
    if (LittleFS.remove(path)) {
        packetCount--;
        return true;
    }
    return false;
#else
    // Host testing: simulate removal
    if (packetCount > 0) {
        packetCount--;
        return true;
    }
    return false;
#endif
}

bool TelemetryBuffer::begin(const int maxBufferedPacketsCount) {
    if (maxBufferedPacketsCount <= 0) {
        MODEST_LOG_E(TELEMETRY_BUFFER_COMPONENT, "Invalid max buffered packets: %d", maxBufferedPacketsCount);
        return false;
    }

#ifdef ARDUINO_ARCH_ESP32
    initialized = false;
    if (!LittleFS.begin(false)) {
        MODEST_LOG_E(TELEMETRY_BUFFER_COMPONENT, "Failed to mount LittleFS");
        return false;
    }

    // Create buffer directory if it doesn't exist
    if (!LittleFS.exists(TELEMETRY_BUFFER_DIRECTORY)) {
        if (!LittleFS.mkdir(TELEMETRY_BUFFER_DIRECTORY)) {
            MODEST_LOG_E(TELEMETRY_BUFFER_COMPONENT, "Failed to create buffer directory");
            return false;
        }
    }

    const int existingPacketCount = countPackets();
    if (existingPacketCount < 0) {
        MODEST_LOG_E(TELEMETRY_BUFFER_COMPONENT, "Failed to enumerate telemetry buffer directory");
        return false;
    }

    TelemetryBuffer::maxPacketsCount = maxBufferedPacketsCount;
    packetCount = existingPacketCount;
    initialized = true;

    if (packetCount > maxPacketsCount) {
        MODEST_LOG_W(TELEMETRY_BUFFER_COMPONENT,
                     "Existing spool exceeds capacity (%d > %d); new stores are blocked until packets are drained",
                     packetCount, maxPacketsCount);
    }
    if (packetCount > 0) {
        MODEST_LOG_W(TELEMETRY_BUFFER_COMPONENT, "Found %d buffered packets from previous session", packetCount);
    }

    MODEST_LOG_I(TELEMETRY_BUFFER_COMPONENT, "Telemetry buffer initialized (max %d packets)", maxBufferedPacketsCount);
    return true;
#else
    // Host testing: simulate initialization
    TelemetryBuffer::maxPacketsCount = maxBufferedPacketsCount;
    packetCount = 0;
    nextWriteIndex = 0;
    initialized = true;
    return true;
#endif
}

bool TelemetryBuffer::store(const String& jsonStringTelemetryPacket) {
    if (!initialized || jsonStringTelemetryPacket.length() == 0) {
        return false;
    }

    if (packetCount > maxPacketsCount) {
        MODEST_LOG_E(TELEMETRY_BUFFER_COMPONENT, "Buffer exceeds configured capacity; refusing additional packet");
        return false;
    }
    if (nextWriteIndex == std::numeric_limits<uint32_t>::max()) {
        MODEST_LOG_E(TELEMETRY_BUFFER_COMPONENT, "Packet identity space exhausted");
        return false;
    }

#ifdef ARDUINO_ARCH_ESP32
    // Write first, so an incoming write failure cannot evict an older packet.
    const String path = getFilePath(nextWriteIndex);
    File file = LittleFS.open(path, "w");
    if (!file) {
        MODEST_LOG_E(TELEMETRY_BUFFER_COMPONENT, "Failed to create buffer file: %s", path.c_str());
        return false;
    }

    const size_t written = file.print(jsonStringTelemetryPacket);
    file.close();

    if (written != jsonStringTelemetryPacket.length()) {
        if (!LittleFS.remove(path)) {
            const int refreshedCount = countPackets();
            if (refreshedCount >= 0) {
                packetCount = refreshedCount;
            }
            MODEST_LOG_E(TELEMETRY_BUFFER_COMPONENT, "Incomplete write cleanup failed; partial file retained");
        } else {
            MODEST_LOG_E(TELEMETRY_BUFFER_COMPONENT, "Incomplete write to buffer file; partial file removed");
        }
        return false;
    }

    bool evictedOldestPacket = false;
    if (packetCount == maxPacketsCount) {
        if (!removeOldestPacket()) {
            if (LittleFS.remove(path)) {
                MODEST_LOG_W(TELEMETRY_BUFFER_COMPONENT, "Failed to evict oldest packet; new packet removed");
                return false;
            }
            const int refreshedCount = countPackets();
            if (refreshedCount >= 0) {
                packetCount = refreshedCount;
            }
            MODEST_LOG_E(TELEMETRY_BUFFER_COMPONENT,
                         "Stored packet but could not enforce capacity; spool is over capacity");
            return true;
        }
        evictedOldestPacket = true;
    }
    if (evictedOldestPacket) {
        if (onEvictionCallback) {
            onEvictionCallback();
        }
    }
    ++nextWriteIndex;
    ++packetCount;
    return true;
#else
    // Host testing: simulate storage
    packetCount++;
    return true;
#endif
}

bool TelemetryBuffer::retrieveNext(String& outJsonBufferedPacket) {
    TelemetryPacketClaim claim;
    if (!claimNext(claim)) {
        return false;
    }
    outJsonBufferedPacket = claim.payload;
    return acknowledge(claim);
}

bool TelemetryBuffer::claimNext(TelemetryPacketClaim& outClaim) {
    outClaim = {};
    if (!initialized) {
        return false;
    }

#ifdef ARDUINO_ARCH_ESP32
    while (packetCount > 0) {
        const String path = findOldestFilePath();
        if (path.length() == 0) {
            const int refreshedCount = countPackets();
            if (refreshedCount >= 0) {
                packetCount = refreshedCount;
            }
            MODEST_LOG_E(TELEMETRY_BUFFER_COMPONENT, "No readable packet file found");
            return false;
        }

        uint32_t packetId = 0;
        if (!parsePacketId(path.c_str(), packetId)) {
            MODEST_LOG_E(TELEMETRY_BUFFER_COMPONENT, "Invalid packet identity in spool path");
            return false;
        }

        File file = LittleFS.open(path, "r");
        if (!file || file.isDirectory()) {
            MODEST_LOG_E(TELEMETRY_BUFFER_COMPONENT, "Failed to open regular packet file: %s", path.c_str());
            return false;
        }

        const size_t expectedSize = file.size();
        const String payload = file.readString();
        file.close();
        if (expectedSize == 0) {
            if (!LittleFS.remove(path)) {
                MODEST_LOG_E(TELEMETRY_BUFFER_COMPONENT, "Failed to discard empty unrecoverable packet");
                return false;
            }
            --packetCount;
            MODEST_LOG_W(TELEMETRY_BUFFER_COMPONENT, "Discarded empty unrecoverable packet");
            continue;
        }
        if (payload.length() != expectedSize) {
            MODEST_LOG_E(TELEMETRY_BUFFER_COMPONENT, "Incomplete packet read; packet retained for retry");
            return false;
        }

        outClaim.packetId = packetId;
        outClaim.payload = payload;
        outClaim.valid = true;
        return true;
    }
#endif
    return false;
}

bool TelemetryBuffer::acknowledge(const TelemetryPacketClaim& claim) {
    if (!initialized || !claim.valid || claim.packetId == 0) {
        return false;
    }

#ifdef ARDUINO_ARCH_ESP32
    const String path = getFilePath(claim.packetId - 1U);
    File file = LittleFS.open(path, "r");
    if (!file || file.isDirectory()) {
        return false;
    }
    if (file.size() != claim.payload.length()) {
        file.close();
        return false;
    }
    const String currentPayload = file.readString();
    file.close();
    if (currentPayload != claim.payload || !LittleFS.remove(path)) {
        MODEST_LOG_W(TELEMETRY_BUFFER_COMPONENT, "Packet acknowledgment did not match or remove the claimed file");
        return false;
    }
    if (packetCount > 0) {
        --packetCount;
    } else {
        const int refreshedCount = countPackets();
        if (refreshedCount >= 0) {
            packetCount = refreshedCount;
        }
    }
    return true;
#else
    return false;
#endif
}

int TelemetryBuffer::getBufferedPacketCount() {
    if (!initialized) {
        return 0;
    }
#ifdef ARDUINO_ARCH_ESP32
    // Refresh count from filesystem for accuracy
    const int refreshedCount = countPackets();
    if (refreshedCount >= 0) {
        packetCount = refreshedCount;
    }
#endif
    return packetCount;
}

int TelemetryBuffer::getMaxPacketsCount() {
    if (!initialized) {
        return DEFAULT_MAX_BUFFERED_PACKETS;
    }
    return maxPacketsCount;
}

bool TelemetryBuffer::hasBufferedPackets() {
    return initialized && packetCount > 0;
}

bool TelemetryBuffer::clear() {
    if (!initialized) {
        return false;
    }

#ifdef ARDUINO_ARCH_ESP32
    // Iterate over all files in the directory and remove them
    // (monotonic indices mean files are not at sequential positions)
    File dir = LittleFS.open(TELEMETRY_BUFFER_DIRECTORY, "r");
    if (!dir || !dir.isDirectory()) {
        return false;
    }
    bool removalSucceeded = true;
    while (File file = dir.openNextFile()) {
        if (!file.isDirectory()) {
            const String path = getFullPath(file.name());
            if (!LittleFS.remove(path)) {
                removalSucceeded = false;
            }
        }
    }
    dir.close();
    const int refreshedCount = countPackets();
    if (refreshedCount < 0) {
        return false;
    }
    packetCount = refreshedCount;
    MODEST_LOG_I(TELEMETRY_BUFFER_COMPONENT, "Buffer cleared");
    return removalSucceeded && packetCount == 0;
#else
    // Host testing: simulate clear
    packetCount = 0;
    nextWriteIndex = 0;
    return true;
#endif
}

bool TelemetryBuffer::isInitialized() {
    return initialized;
}

void TelemetryBuffer::onEviction(EvictionCallback callback) {
    onEvictionCallback = callback;
}
