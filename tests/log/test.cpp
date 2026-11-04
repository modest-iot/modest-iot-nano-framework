#include "ModestLog.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

struct LogEntry {
    int level;
    std::string tag;
    std::string message;
};

std::vector<LogEntry> capturedLogs;

void testLogSink(int level, const char* tag, const char* fmt, va_list args) {
    char buffer[256];
    std::vsnprintf(buffer, sizeof(buffer), fmt, args);
    capturedLogs.push_back({level, tag != nullptr ? tag : "", buffer});
}

void resetCapture() {
    capturedLogs.clear();
}

} // anonymous namespace

int main() {
    std::printf("Running ModestLog and MIOT_LOG tests...\n");

    modest_iot::setLogSink(testLogSink);

    // 1. Verify MODEST_LOG_* macros with default log level (WARN = 2)
    resetCapture();
    MODEST_LOG_E("Sensor", "Error code %d", 404);
    MODEST_LOG_W("Sensor", "Warning value %d", 42);
    // Info, Debug, and Verbose should be compiled out / filtered at WARN level
    MODEST_LOG_I("Sensor", "Info message %s", "ignored");
    MODEST_LOG_D("Sensor", "Debug message %s", "ignored");
    MODEST_LOG_V("Sensor", "Verbose message %s", "ignored");

    assert(capturedLogs.size() == 2);
    assert(capturedLogs[0].level == MODEST_LOG_LEVEL_ERROR);
    assert(capturedLogs[0].tag == "Sensor");
    assert(capturedLogs[0].message == "Error code 404");

    assert(capturedLogs[1].level == MODEST_LOG_LEVEL_WARN);
    assert(capturedLogs[1].tag == "Sensor");
    assert(capturedLogs[1].message == "Warning value 42");

    // 2. Verify MIOT_LOG_* aliases behavior
    resetCapture();
    MIOT_LOG_E("Network", "Connection lost (code: %d)", 101);
    MIOT_LOG_W("Network", "Retrying in %d ms", 5000);
    MIOT_LOG_I("Network", "Connected to %s", "broker");
    MIOT_LOG_D("Network", "Packet payload %s", "raw");
    MIOT_LOG_V("Network", "Trace byte %02X", 0xFF);

    assert(capturedLogs.size() == 2);
    assert(capturedLogs[0].level == MODEST_LOG_LEVEL_ERROR);
    assert(capturedLogs[0].tag == "Network");
    assert(capturedLogs[0].message == "Connection lost (code: 101)");

    assert(capturedLogs[1].level == MODEST_LOG_LEVEL_WARN);
    assert(capturedLogs[1].tag == "Network");
    assert(capturedLogs[1].message == "Retrying in 5000 ms");

    // 3. Verify format specifiers and multiple arguments
    resetCapture();
    MIOT_LOG_E("Actuator", "PWM failed on pin %d, duty %d%%, status %s", 18, 75, "OVERLOAD");
    assert(capturedLogs.size() == 1);
    assert(capturedLogs[0].tag == "Actuator");
    assert(capturedLogs[0].message == "PWM failed on pin 18, duty 75%, status OVERLOAD");

    // 4. Verify sink unregistration fallback
    modest_iot::setLogSink(nullptr);
    // Should safely format to stdout without throwing or crashing
    MODEST_LOG_E("StdoutTest", "Testing fallback to stdout: %d", 1);
    MIOT_LOG_W("StdoutTest", "Testing fallback to stdout: %d", 2);

    std::printf("All ModestLog and MIOT_LOG tests passed successfully!\n");
    return 0;
}
