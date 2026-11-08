#include "Device.h"
#include <cassert>
#include <cstdio>

namespace {

// Verify EngineConfig default values match the original hard-coded parameters
void testEngineConfigDefaults() {
    EngineConfiguration config;

    // Default sampler stack: 4096 bytes
    assert(config.samplerStackBytes == 4096);

    // Default network stack: 8192 bytes
    assert(config.networkStackBytes == 8192);

    // Default sampler priority: 3
    assert(config.samplerPriority == 3);

    // Default network priority: 1
    assert(config.networkPriority == 1);

    // Default core pinning: both on core 1
    assert(config.samplerCore == 1);
    assert(config.networkCore == 1);

    // Default queue length: 10
    assert(config.queueLength == 10);
}

// Verify EngineConfig can be customized for high-memory scenarios
void testEngineConfigCustomValues() {
    EngineConfiguration config;
    config.samplerStackBytes = 8192;
    config.networkStackBytes = 16384;
    config.samplerPriority = 4;
    config.networkPriority = 2;
    config.samplerCore = 0;
    config.networkCore = 1;
    config.queueLength = 20;

    assert(config.samplerStackBytes == 8192);
    assert(config.networkStackBytes == 16384);
    assert(config.samplerPriority == 4);
    assert(config.networkPriority == 2);
    assert(config.samplerCore == 0);
    assert(config.networkCore == 1);
    assert(config.queueLength == 20);
}

// Verify EngineConfig can be used with minimal queue for low-memory devices
void testEngineConfigMinimalValues() {
    EngineConfiguration config;
    config.samplerStackBytes = 2048;
    config.networkStackBytes = 4096;
    config.queueLength = 4;

    assert(config.samplerStackBytes == 2048);
    assert(config.networkStackBytes == 4096);
    assert(config.queueLength == 4);
    // Priorities and cores should remain at defaults
    assert(config.samplerPriority == 3);
    assert(config.networkPriority == 1);
    assert(config.samplerCore == 1);
    assert(config.networkCore == 1);
}

// Verify EngineConfig aggregate initialization
void testEngineConfigAggregateInit() {
    EngineConfiguration config{4096, 8192, 3, 1, 1, 1, 10};
    assert(config.samplerStackBytes == 4096);
    assert(config.networkStackBytes == 8192);
    assert(config.samplerPriority == 3);
    assert(config.networkPriority == 1);
    assert(config.samplerCore == 1);
    assert(config.networkCore == 1);
    assert(config.queueLength == 10);
}

} // anonymous namespace

[[nodiscard]] int main() {
    std::printf("Running EngineConfig Tests...\n");

    testEngineConfigDefaults();
    std::printf("  [PASS] EngineConfig default values match hard-coded parameters\n");

    testEngineConfigCustomValues();
    std::printf("  [PASS] EngineConfig custom values for high-memory scenarios\n");

    testEngineConfigMinimalValues();
    std::printf("  [PASS] EngineConfig minimal values for low-memory devices\n");

    testEngineConfigAggregateInit();
    std::printf("  [PASS] EngineConfig aggregate initialization\n");

    std::printf("All EngineConfig tests passed successfully!\n");
    return 0;
}
