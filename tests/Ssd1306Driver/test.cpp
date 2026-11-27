#include <time.h>
/**
 * @file test.cpp
 * @brief Host unit tests for the Ssd1306Driver component.
 *
 * Validates I2C commands, display control, and frame buffer operations.
 */

#include "Ssd1306Driver.h"
#include "FakeI2cBus.h"
#include <cassert>
#include <cstdio>
#include <cstring>

void testInjectedBusStartupAndFailures() {
    Wire.transmissionCount = 0;
    FakeI2cBus bus;
    Ssd1306Driver driver(bus);

    assert(bus.writeCount == 0);
    assert(Wire.transmissionCount == 0);
    assert(!driver.isInitialized());
    assert(!driver.start());
    assert(driver.getLastI2cResult().status == I2cStatus::NotStarted);
    assert(bus.writeCount == 0);

    const I2cBusConfiguration config{21, 22, 400000};
    assert(bus.start(config) == I2cStatus::Ok);
    bus.nextWriteStatus = I2cStatus::AddressNack;
    assert(!driver.start());
    assert(bus.writeCount == 1);
    assert(!driver.isInitialized());
    assert(!driver.isDisplayOn());
    assert(driver.getLastI2cResult().status == I2cStatus::AddressNack);

    assert(driver.start());
    assert(driver.isInitialized());
    assert(driver.isDisplayOn());
    assert(Wire.transmissionCount == 0);

    const uint8_t page[128]{};
    const int writesBeforePage = bus.writeCount;
    driver.sendData(page, sizeof(page));
    assert(bus.writeCount == writesBeforePage + 2);
    assert(bus.lastWrite.size() == 2);
    assert(bus.lastWrite[0] == 0x40);

    const int writesBeforePageFailure = bus.writeCount;
    bus.nextWriteStatus = I2cStatus::AddressNack;
    driver.sendData(page, sizeof(page));
    assert(bus.writeCount == writesBeforePageFailure + 1);
    assert(driver.getLastI2cResult().status == I2cStatus::AddressNack);

    const int writesBeforeFailure = bus.writeCount;
    bus.nextWriteStatus = I2cStatus::BusError;
    driver.setContrast(0x55);
    assert(bus.writeCount == writesBeforeFailure + 1);
    assert(driver.getLastI2cResult().status == I2cStatus::BusError);
}

int main() {
    std::printf("Running Ssd1306Driver tests...\n");
    testInjectedBusStartupAndFailures();

    // Test 1: Constructor initializes state
    {
        Ssd1306Driver driver;
        assert(driver.getWidth() == 128);
        assert(driver.getHeight() == 64);
        assert(driver.isDisplayOn() == false);
        std::printf("  [PASS] Constructor initializes default state\n");
    }

    // Test 2: Constructor with custom dimensions
    {
        Ssd1306Driver driver(0x3C, 128, 32);
        assert(driver.getWidth() == 128);
        assert(driver.getHeight() == 32);
        std::printf("  [PASS] Constructor with custom dimensions\n");
    }

    // Test 3: Constructor with alternate I2C address
    {
        Ssd1306Driver driver(0x3D, 128, 64);
        assert(driver.getWidth() == 128);
        assert(driver.getHeight() == 64);
        std::printf("  [PASS] Constructor with alternate I2C address\n");
    }

    // Test 4: initialize sends I2C commands and turns display on
    {
        Wire.transmissionCount = 0;
        Ssd1306Driver driver;
        driver.initialize();
        assert(Wire.transmissionCount > 0);
        assert(driver.isDisplayOn() == true);
        std::printf("  [PASS] initialize sends I2C commands and turns display on\n");
    }

    // Test 5: setDisplayOn controls display state
    {
        Ssd1306Driver driver;
        driver.setDisplayOn(true);
        assert(driver.isDisplayOn() == true);
        driver.setDisplayOn(false);
        assert(driver.isDisplayOn() == false);
        std::printf("  [PASS] setDisplayOn controls display state\n");
    }

    // Test 6: setContrast sends I2C commands
    {
        Wire.transmissionCount = 0;
        Ssd1306Driver driver;
        driver.setContrast(0xFF);
        assert(Wire.transmissionCount > 0);
        std::printf("  [PASS] setContrast sends I2C commands\n");
    }

    // Test 7: setInvert sends I2C commands
    {
        Wire.transmissionCount = 0;
        Ssd1306Driver driver;
        driver.setInvert(true);
        assert(Wire.transmissionCount > 0);
        driver.setInvert(false);
        assert(Wire.transmissionCount > 0);
        std::printf("  [PASS] setInvert sends I2C commands\n");
    }

    // Test 8: sendData transmits data
    {
        Wire.transmissionCount = 0;
        Ssd1306Driver driver;
        uint8_t data[] = {0xFF, 0xFF, 0xFF};
        driver.sendData(data, 3);
        assert(Wire.transmissionCount > 0);
        std::printf("  [PASS] sendData transmits data\n");
    }

    // Test 9: sendFrameBuffer transmits page by page
    {
        Wire.transmissionCount = 0;
        Ssd1306Driver driver(0x3C, 128, 64);
        uint8_t frameBuffer[128 * 8] = {0};
        driver.sendFrameBuffer(frameBuffer, 128);
        assert(Wire.transmissionCount > 0);
        std::printf("  [PASS] sendFrameBuffer transmits page by page\n");
    }

    std::printf("All Ssd1306Driver tests passed successfully!\n");
    return 0;
}
