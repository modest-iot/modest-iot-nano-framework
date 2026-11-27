#include <time.h>
/**
 * @file test.cpp
 * @brief Host unit tests for the Hd44780Driver component.
 *
 * Validates I2C commands, backlight control, and initialization sequence.
 */

#include "Hd44780Driver.h"
#include "FakeI2cBus.h"
#include <cassert>
#include <cstdio>

void testInjectedBusStartupAndFailures() {
    Wire.transmissionCount = 0;
    FakeI2cBus bus;
    Hd44780Driver driver(bus, 0x27, true);

    assert(bus.writeCount == 0);
    assert(Wire.transmissionCount == 0);
    assert(!driver.isInitialized());
    assert(!driver.start());
    assert(driver.getLastI2cResult().status == I2cStatus::NotStarted);
    assert(bus.writeCount == 0);

    const I2cBusConfiguration config{21, 22, 400000};
    assert(bus.start(config) == I2cStatus::Ok);
    assert(driver.start());
    assert(driver.isInitialized());
    assert(bus.writeCount > 0);
    assert(Wire.transmissionCount == 0);

    const int writesBeforeFailure = bus.writeCount;
    bus.nextWriteStatus = I2cStatus::AddressNack;
    driver.sendCommand(0x01);
    assert(bus.writeCount == writesBeforeFailure + 1);
    assert(driver.getLastI2cResult().status == I2cStatus::AddressNack);

    driver.setBacklight(false);
    assert(!driver.isBacklightOn());
}

int main() {
    std::printf("Running Hd44780Driver tests...\n");
    testInjectedBusStartupAndFailures();

    // Test 1: Constructor initializes state
    {
        Hd44780Driver driver(0x27, true);
        assert(driver.isBacklightOn() == true);
        std::printf("  [PASS] Constructor initializes backlight on\n");
    }

    // Test 2: Constructor with backlight off
    {
        Hd44780Driver driver(0x3E, false);
        assert(driver.isBacklightOn() == false);
        std::printf("  [PASS] Constructor initializes backlight off\n");
    }

    // Test 3: setBacklight toggles state
    {
        Hd44780Driver driver(0x27, true);
        assert(driver.isBacklightOn() == true);
        driver.setBacklight(false);
        assert(driver.isBacklightOn() == false);
        driver.setBacklight(true);
        assert(driver.isBacklightOn() == true);
        std::printf("  [PASS] setBacklight toggles state\n");
    }

    // Test 4: initialize sends I2C commands
    {
        Wire.transmissionCount = 0;
        Hd44780Driver driver(0x27, true);
        driver.initialize();
        assert(Wire.transmissionCount > 0); // Initialization sends multiple I2C transmissions
        std::printf("  [PASS] initialize sends I2C commands\n");
    }

    // Test 5: sendCommand sends I2C data
    {
        Wire.transmissionCount = 0;
        Hd44780Driver driver(0x27, true);
        driver.sendCommand(0x01); // Clear display command
        assert(Wire.transmissionCount > 0);
        std::printf("  [PASS] sendCommand sends I2C data\n");
    }

    // Test 6: sendData sends I2C data
    {
        Wire.transmissionCount = 0;
        Hd44780Driver driver(0x27, true);
        driver.sendData('H');
        assert(Wire.transmissionCount > 0);
        std::printf("  [PASS] sendData sends I2C data\n");
    }

    // Test 7: Multiple I2C addresses
    {
        Hd44780Driver driver1(0x27, true);
        Hd44780Driver driver2(0x3E, false);
        assert(driver1.isBacklightOn() == true);
        assert(driver2.isBacklightOn() == false);
        std::printf("  [PASS] Multiple I2C addresses\n");
    }

    std::printf("All Hd44780Driver tests passed successfully!\n");
    return 0;
}
