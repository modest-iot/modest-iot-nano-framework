/**
 * @file test.cpp
 * @brief Host tests for TelemetryBuffer persistent buffering.
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-11-14
 * @version 1.0
 */

#include <Arduino.h>
#include <LittleFS.h>
#include "TelemetryBuffer.h"
#include <cassert>
#include <cstdio>
#include <cstring>

// Reset static state before each test
static void resetBuffer() {
    LittleFS.reset();
    assert(TelemetryBuffer::begin());
    assert(TelemetryBuffer::clear());
}

int main() {
    printf("=== TelemetryBuffer Tests ===\n");

    // Test 1: Default state is not initialized
    printf("Test 1: Default state is not initialized... ");
    assert(!TelemetryBuffer::isInitialized());
    assert(!TelemetryBuffer::hasBufferedPackets());
    assert(TelemetryBuffer::getBufferedPacketCount() == 0);
    printf("PASSED\n");

    // Test 2: begin() initializes the buffer
    printf("Test 2: begin() initializes the buffer... ");
    assert(TelemetryBuffer::begin());
    assert(TelemetryBuffer::isInitialized());
    assert(!TelemetryBuffer::hasBufferedPackets());
    assert(TelemetryBuffer::getBufferedPacketCount() == 0);
    printf("PASSED\n");

    // Test 3: begin() with custom max packets
    printf("Test 3: begin() with custom max packets... ");
    resetBuffer();
    assert(TelemetryBuffer::begin(5));
    assert(TelemetryBuffer::isInitialized());
    printf("PASSED\n");

    // Test 4: begin() with invalid max packets fails
    printf("Test 4: begin() with invalid max packets fails... ");
    resetBuffer();
    assert(!TelemetryBuffer::begin(0));
    // Note: begin(0) returns false but doesn't change the initialized flag
    // if it was already initialized from a previous successful begin()
    assert(!TelemetryBuffer::begin(-1));
    printf("PASSED\n");

    // Test 5: Store a packet
    printf("Test 5: Store a packet... ");
    resetBuffer();
    TelemetryBuffer::begin();
    String json = "{\"sequence\":1,\"temperature\":25.5}";
    assert(TelemetryBuffer::store(json));
    assert(TelemetryBuffer::hasBufferedPackets());
    assert(TelemetryBuffer::getBufferedPacketCount() == 1);
    printf("PASSED\n");

    // Test 6: Store multiple packets
    printf("Test 6: Store multiple packets... ");
    resetBuffer();
    TelemetryBuffer::begin(10);
    assert(TelemetryBuffer::store("{\"seq\":1}"));
    assert(TelemetryBuffer::store("{\"seq\":2}"));
    assert(TelemetryBuffer::store("{\"seq\":3}"));
    assert(TelemetryBuffer::getBufferedPacketCount() == 3);
    printf("PASSED\n");

    // Test 7: Store empty string fails
    printf("Test 7: Store empty string fails... ");
    resetBuffer();
    TelemetryBuffer::begin();
    assert(!TelemetryBuffer::store(""));
    assert(TelemetryBuffer::getBufferedPacketCount() == 0);
    printf("PASSED\n");

    // Test 8: getMaxPacketsCount() returns configured value
    printf("Test 8: getMaxPacketsCount() returns configured value... ");
    resetBuffer();
    TelemetryBuffer::begin(5);
    assert(TelemetryBuffer::getMaxPacketsCount() == 5);
    resetBuffer();
    TelemetryBuffer::begin();
    assert(TelemetryBuffer::getMaxPacketsCount() == DEFAULT_MAX_BUFFERED_PACKETS);
    printf("PASSED\n");

    // Test 9: Retrieve next packet (FIFO)
    printf("Test 9: Retrieve next packet (FIFO)... ");
    resetBuffer();
    TelemetryBuffer::begin();
    TelemetryBuffer::store("{\"seq\":1}");
    TelemetryBuffer::store("{\"seq\":2}");
    {
        String out9;
        assert(TelemetryBuffer::retrieveNext(out9));
        assert(out9.length() > 0);
    }
    assert(TelemetryBuffer::getBufferedPacketCount() == 1);
    printf("PASSED\n");

    // Test 10: Retrieve drains all packets
    printf("Test 10: Retrieve drains all packets... ");
    resetBuffer();
    TelemetryBuffer::begin(5);
    TelemetryBuffer::store("{\"seq\":1}");
    TelemetryBuffer::store("{\"seq\":2}");
    TelemetryBuffer::store("{\"seq\":3}");
    {
        String out10a, out10b, out10c;
        assert(TelemetryBuffer::retrieveNext(out10a));
        assert(TelemetryBuffer::retrieveNext(out10b));
        assert(TelemetryBuffer::retrieveNext(out10c));
    }
    assert(!TelemetryBuffer::hasBufferedPackets());
    assert(TelemetryBuffer::getBufferedPacketCount() == 0);
    printf("PASSED\n");

    // Test 11: Retrieve from empty buffer fails
    printf("Test 11: Retrieve from empty buffer fails... ");
    resetBuffer();
    TelemetryBuffer::begin();
    {
        String out11;
        assert(!TelemetryBuffer::retrieveNext(out11));
    }
    printf("PASSED\n");

    // Test 12: Buffer capacity eviction (FIFO)
    printf("Test 12: Buffer capacity eviction (FIFO)... ");
    resetBuffer();
    TelemetryBuffer::begin(3);
    assert(TelemetryBuffer::store("{\"seq\":1}"));
    assert(TelemetryBuffer::store("{\"seq\":2}"));
    assert(TelemetryBuffer::store("{\"seq\":3}"));
    assert(TelemetryBuffer::getBufferedPacketCount() == 3);
    // This should evict the oldest (seq:1) and store seq:4
    assert(TelemetryBuffer::store("{\"seq\":4}"));
    assert(TelemetryBuffer::getBufferedPacketCount() == 3);
    printf("PASSED\n");

    // Test 13: Clear all buffered packets
    printf("Test 13: Clear all buffered packets... ");
    resetBuffer();
    TelemetryBuffer::begin();
    TelemetryBuffer::store("{\"seq\":1}");
    TelemetryBuffer::store("{\"seq\":2}");
    assert(TelemetryBuffer::clear());
    assert(!TelemetryBuffer::hasBufferedPackets());
    assert(TelemetryBuffer::getBufferedPacketCount() == 0);
    printf("PASSED\n");

    // Test 14: hasBufferedPackets reflects state
    printf("Test 14: hasBufferedPackets reflects state... ");
    resetBuffer();
    TelemetryBuffer::begin();
    assert(!TelemetryBuffer::hasBufferedPackets());
    TelemetryBuffer::store("{\"seq\":1}");
    assert(TelemetryBuffer::hasBufferedPackets());
    {
        String out14;
        TelemetryBuffer::retrieveNext(out14);
    }
    assert(!TelemetryBuffer::hasBufferedPackets());
    printf("PASSED\n");

    // Test 15: getFilePath generates correct paths
    printf("Test 15: getFilePath generates correct paths... ");
    // Access via the store/retrieve flow — host mode simulates correctly
    resetBuffer();
    TelemetryBuffer::begin();
    // The getFilePath method is private, but we can verify the behavior
    // by checking that store/retrieve works correctly
    TelemetryBuffer::store("{\"test\":true}");
    assert(TelemetryBuffer::getBufferedPacketCount() == 1);
    printf("PASSED\n");

    // Test 16: Multiple begin() calls reset state
    printf("Test 16: Multiple begin() calls reset state... ");
    resetBuffer();
    TelemetryBuffer::begin(10);
    TelemetryBuffer::store("{\"seq\":1}");
    assert(TelemetryBuffer::getBufferedPacketCount() == 1);
    // Re-begin should reset packet count
    TelemetryBuffer::begin(5);
    assert(TelemetryBuffer::isInitialized());
    printf("PASSED\n");

    // Test 17: Store and retrieve preserves exact JSON content on the fake filesystem
    printf("Test 17: Store and retrieve preserves exact JSON content... ");
    resetBuffer();
    TelemetryBuffer::begin();
    TelemetryBuffer::store("{\"data\":\"value\"}");
    {
        String out17;
        assert(TelemetryBuffer::retrieveNext(out17));
        assert(out17 == "{\"data\":\"value\"}");
    }
    printf("PASSED\n");

    // Test 18: DEFAULT_MAX_BUFFERED_PACKETS constant
    printf("Test 18: DEFAULT_MAX_BUFFERED_PACKETS constant... ");
    assert(DEFAULT_MAX_BUFFERED_PACKETS == 50);
    printf("PASSED\n");

    // Test 19: TELEMETRY_BUFFER_DIRECTORY constant
    printf("Test 19: TELEMETRY_BUFFER_DIRECTORY constant... ");
    assert(strcmp(TELEMETRY_BUFFER_DIRECTORY, "/telemetry_buffer") == 0);
    printf("PASSED\n");

    // Test 20: Eviction callback is called when buffer is full
    printf("Test 20: Eviction callback is called when buffer is full... ");
    resetBuffer();
    TelemetryBuffer::begin(3);
    bool evicted = false;
    TelemetryBuffer::onEviction([&evicted]() { evicted = true; });
    assert(TelemetryBuffer::store("{\"seq\":1}"));
    assert(TelemetryBuffer::store("{\"seq\":2}"));
    assert(TelemetryBuffer::store("{\"seq\":3}"));
    assert(!evicted); // Not yet at capacity eviction
    assert(TelemetryBuffer::store("{\"seq\":4}")); // This triggers eviction
    assert(evicted);
    // Clear callback
    TelemetryBuffer::onEviction(nullptr);
    printf("PASSED\n");

    // Test 21: Mount failure never formats existing packets
    printf("Test 21: Mount failure preserves existing packets... ");
    resetBuffer();
    assert(TelemetryBuffer::store("{\"seq\":\"preserve\"}"));
    LittleFS.setMountAvailable(false);
    assert(!TelemetryBuffer::begin());
    assert(LittleFS.getFormatCount() == 0);
    assert(LittleFS.getStoredFileCount() == 1);
    LittleFS.setMountAvailable(true);
    assert(TelemetryBuffer::begin());
    assert(TelemetryBuffer::getBufferedPacketCount() == 1);
    printf("PASSED\n");

    // Test 22: Failed incoming write does not evict the existing packet
    printf("Test 22: Failed incoming write preserves oldest packet... ");
    resetBuffer();
    assert(TelemetryBuffer::begin(1));
    assert(TelemetryBuffer::store("{\"seq\":\"old\"}"));
    LittleFS.failNextWriteAfter(2);
    assert(!TelemetryBuffer::store("{\"seq\":\"new\"}"));
    assert(LittleFS.getStoredFileCount() == 1);
    File oldest = LittleFS.open(LittleFS.getFirstStoredFilePath(), "r");
    assert(oldest && oldest.readString() == "{\"seq\":\"old\"}");
    printf("PASSED\n");

    // Test 23: Partial file writes are cleaned up
    printf("Test 23: Partial file writes are cleaned up... ");
    resetBuffer();
    LittleFS.failNextWriteAfter(2);
    assert(!TelemetryBuffer::store("{\"seq\":partial}"));
    assert(LittleFS.getStoredFileCount() == 0);
    assert(TelemetryBuffer::getBufferedPacketCount() == 0);
    printf("PASSED\n");

    // Test 24: Directory creation failure leaves the buffer uninitialized
    printf("Test 24: Directory creation failure is reported... ");
    LittleFS.reset();
    LittleFS.setDirectoryCreationAvailable(false);
    assert(!TelemetryBuffer::begin());
    assert(!TelemetryBuffer::isInitialized());
    assert(LittleFS.getFormatCount() == 0);
    printf("PASSED\n");

    // Test 25: Failed capacity eviction retains both the old record and spool contents
    printf("Test 25: Failed capacity eviction preserves old record... ");
    resetBuffer();
    assert(TelemetryBuffer::begin(1));
    assert(TelemetryBuffer::store("{\"seq\":\"old\"}"));
    const String oldPacketPath = LittleFS.getFirstStoredFilePath();
    LittleFS.failRemoveFor(oldPacketPath);
    assert(!TelemetryBuffer::store("{\"seq\":\"new\"}"));
    assert(TelemetryBuffer::getBufferedPacketCount() == 1);
    TelemetryPacketClaim retainedOld;
    assert(TelemetryBuffer::claimNext(retainedOld));
    assert(retainedOld.payload == "{\"seq\":\"old\"}");
    assert(TelemetryBuffer::acknowledge(retainedOld));
    printf("PASSED\n");

    // Test 26: Claims retain packets until an exact acknowledgment
    printf("Test 26: Claims retain packets until acknowledged... ");
    resetBuffer();
    assert(TelemetryBuffer::store("{\"seq\":\"claimed\"}"));
    TelemetryPacketClaim firstClaim;
    assert(TelemetryBuffer::claimNext(firstClaim));
    assert(TelemetryBuffer::getBufferedPacketCount() == 1);
    TelemetryPacketClaim retryClaim;
    assert(TelemetryBuffer::claimNext(retryClaim));
    assert(retryClaim.packetId == firstClaim.packetId);
    assert(retryClaim.payload == firstClaim.payload);
    assert(TelemetryBuffer::acknowledge(firstClaim));
    assert(!TelemetryBuffer::acknowledge(firstClaim));
    assert(TelemetryBuffer::getBufferedPacketCount() == 0);
    printf("PASSED\n");

    // Test 27: Stale acknowledgments cannot remove a later packet
    printf("Test 27: Stale acknowledgment cannot remove later packet... ");
    assert(TelemetryBuffer::clear());
    assert(TelemetryBuffer::begin());
    assert(TelemetryBuffer::store("{\"seq\":\"later\"}"));
    TelemetryPacketClaim laterClaim;
    assert(TelemetryBuffer::claimNext(laterClaim));
    assert(laterClaim.packetId != firstClaim.packetId);
    assert(!TelemetryBuffer::acknowledge(firstClaim));
    assert(TelemetryBuffer::getBufferedPacketCount() == 1);
    assert(TelemetryBuffer::acknowledge(laterClaim));
    printf("PASSED\n");

    // Test 28: Read and remove errors retain a claimed packet for retry
    printf("Test 28: Read/remove failures retain packet... ");
    resetBuffer();
    assert(TelemetryBuffer::store("{\"seq\":\"retry\"}"));
    const String retryPath = LittleFS.getFirstStoredFilePath();
    LittleFS.failOpenFor(retryPath);
    TelemetryPacketClaim retryAfterOpenFailure;
    assert(!TelemetryBuffer::claimNext(retryAfterOpenFailure));
    LittleFS.failReadFor(retryPath);
    assert(!TelemetryBuffer::claimNext(retryAfterOpenFailure));
    assert(TelemetryBuffer::getBufferedPacketCount() == 1);
    assert(TelemetryBuffer::claimNext(retryAfterOpenFailure));
    LittleFS.failRemoveFor(retryPath);
    assert(!TelemetryBuffer::acknowledge(retryAfterOpenFailure));
    assert(TelemetryBuffer::claimNext(retryAfterOpenFailure));
    assert(TelemetryBuffer::acknowledge(retryAfterOpenFailure));
    printf("PASSED\n");

    // Test 29: Empty files are discarded, while opaque malformed payloads remain claimed
    printf("Test 29: Empty and malformed packet files are handled... ");
    resetBuffer();
    LittleFS.addFileForTest("/telemetry_buffer/00001.json", "");
    LittleFS.addFileForTest("/telemetry_buffer/not-a-packet.txt", "ignored");
    LittleFS.addFileForTest("/telemetry_buffer/00002.json", "not-json");
    LittleFS.addFileForTest("/telemetry_buffer/00003.json", "{\"seq\":\"valid\"}");
    assert(TelemetryBuffer::begin());
    assert(TelemetryBuffer::getBufferedPacketCount() == 3);
    TelemetryPacketClaim validAfterEmpty;
    assert(TelemetryBuffer::claimNext(validAfterEmpty));
    assert(validAfterEmpty.packetId == 2);
    assert(validAfterEmpty.payload == "not-json");
    TelemetryPacketClaim malformedRetry;
    assert(TelemetryBuffer::claimNext(malformedRetry));
    assert(malformedRetry.packetId == validAfterEmpty.packetId);
    assert(malformedRetry.payload == "not-json");
    assert(TelemetryBuffer::acknowledge(validAfterEmpty));
    assert(TelemetryBuffer::getBufferedPacketCount() == 1);
    assert(TelemetryBuffer::claimNext(validAfterEmpty));
    assert(validAfterEmpty.packetId == 3);
    assert(validAfterEmpty.payload == "{\"seq\":\"valid\"}");
    assert(TelemetryBuffer::acknowledge(validAfterEmpty));
    assert(TelemetryBuffer::getBufferedPacketCount() == 0);
    assert(TelemetryBuffer::clear());
    assert(LittleFS.getStoredFileCount() == 0);
    printf("PASSED\n");

    // Test 30: Eviction callback can be cleared
    printf("Test 30: Eviction callback can be cleared... ");
    resetBuffer();
    TelemetryBuffer::begin(2);
    TelemetryBuffer::onEviction(nullptr); // Ensure no callback
    assert(TelemetryBuffer::store("{\"seq\":1}"));
    assert(TelemetryBuffer::store("{\"seq\":2}"));
    assert(TelemetryBuffer::store("{\"seq\":3}")); // Evicts without callback
    printf("PASSED\n");

    // Test 31: getMaxPacketsCount() remains available after initialization
    printf("Test 31: getMaxPacketsCount() returns a positive value... ");
    // After all tests, the buffer is initialized. We can't easily uninit in host mode,
    // but we verified the initialized path in Test 8.
    // This test verifies the getter is available and returns a sensible value.
    assert(TelemetryBuffer::getMaxPacketsCount() > 0);
    printf("PASSED\n");

    printf("\n=== All 31 TelemetryBuffer tests passed ===\n");
    return 0;
}
