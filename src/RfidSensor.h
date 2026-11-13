#ifndef RFID_SENSOR_H
#define RFID_SENSOR_H

/**
 * @file RfidSensor.h
 * @brief Declares the RfidSensor RFID/NFC reader sensor adapter.
 * @ingroup sensors_module
 * @copyright (c) 2025-2026 Angel Velasquez
 * @license Apache-2.0
 * @date 2026-11-03
 * @version 1.0
 */

#include "Sensor.h"
#include <Arduino.h>
#include <SPI.h>

/**
 * @brief RFID reader sensor adapter supporting MFRC522/ISO14443A contactless smart cards and tags.
 * @ingroup sensors_module
 *
 * This class implements a lightweight, direct SPI register driver for the NXP MFRC522
 * transceiver IC, reading ISO/IEC 14443 Type A tags (Mifare Classic, Ultralight, NTAG).
 * It detects card presence transitions and extracts Unique Identifiers (UIDs) across
 * Cascade Levels 1, 2, and 3 (4-byte, 7-byte, and 10-byte UIDs).
 */
class RfidSensor : public Sensor {
public:
    /// Maximum supported UID length in bytes (Single=4, Double=7, Triple=10).
    static constexpr uint8_t MAX_UID_BYTES = 10;
    /// Maximum formatted UID string length (including colons and null terminator).
    static constexpr uint8_t MAX_UID_STRING_LENGTH = 32;

    /// Event identifier emitted when a new RFID/NFC tag enters the RF proximity field.
    static constexpr int TAG_DETECTED_EVENT_IDENTIFIER = 60;
    /// Event identifier emitted when an RFID/NFC tag leaves the RF proximity field.
    static constexpr int TAG_REMOVED_EVENT_IDENTIFIER = 61;

    /// Prebuilt tag-detected event instance.
    static const Event TAG_DETECTED_EVENT;
    /// Prebuilt tag-removed event instance.
    static const Event TAG_REMOVED_EVENT;

    /// Special identifier used internally to suppress event propagation.
    static constexpr int PROPAGATION_SUPPRESSION_IDENTIFIER = -1;

    /// Default sampling interval in milliseconds.
    /// @note MFRC522 tag polling at 100 ms provides responsive detection without excessive SPI traffic.
    static constexpr uint32_t DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS = 100;
    /// Minimum sampling interval in milliseconds.
    /// @note Below 50 ms the RF field may not stabilize between polling cycles.
    static constexpr uint32_t MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS = 50;

    /// MFRC522 operation status codes.
    enum StatusCode : uint8_t {
        STATUS_OK = 0,
        STATUS_ERROR = 1,
        STATUS_COLLISION = 2,
        STATUS_TIMEOUT = 3,
        STATUS_NO_ROOM = 4,
        STATUS_INTERNAL_ERROR = 5,
        STATUS_INVALID = 6,
        STATUS_CRC_WRONG = 7,
        STATUS_MIFARE_NACK = 8
    };

    // MFRC522 Register Addresses
    static constexpr uint8_t CommandReg         = 0x01;
    static constexpr uint8_t ComIEnReg          = 0x02;
    static constexpr uint8_t DivIEnReg          = 0x03;
    static constexpr uint8_t ComIrqReg          = 0x04;
    static constexpr uint8_t DivIrqReg          = 0x05;
    static constexpr uint8_t ErrorReg           = 0x06;
    static constexpr uint8_t Status1Reg         = 0x07;
    static constexpr uint8_t Status2Reg         = 0x08;
    static constexpr uint8_t FIFODataReg        = 0x09;
    static constexpr uint8_t FIFOLevelReg       = 0x0A;
    static constexpr uint8_t WaterLevelReg      = 0x0B;
    static constexpr uint8_t ControlReg         = 0x0C;
    static constexpr uint8_t BitFramingReg      = 0x0D;
    static constexpr uint8_t CollReg            = 0x0E;

    static constexpr uint8_t ModeReg            = 0x11;
    static constexpr uint8_t TxModeReg          = 0x12;
    static constexpr uint8_t RxModeReg          = 0x13;
    static constexpr uint8_t TxControlReg       = 0x14;
    static constexpr uint8_t TxASKReg           = 0x15;
    static constexpr uint8_t TxSelReg           = 0x16;
    static constexpr uint8_t RxSelReg           = 0x17;
    static constexpr uint8_t RxThresholdReg     = 0x18;
    static constexpr uint8_t DemodReg           = 0x19;
    static constexpr uint8_t MfTxReg            = 0x1C;
    static constexpr uint8_t MfRxReg            = 0x1D;
    static constexpr uint8_t SerialSpeedReg     = 0x1F;

    static constexpr uint8_t CRCResultRegH      = 0x21;
    static constexpr uint8_t CRCResultRegL      = 0x22;
    static constexpr uint8_t ModWidthReg        = 0x24;
    static constexpr uint8_t RFCfgReg           = 0x26;
    static constexpr uint8_t GsNReg             = 0x27;
    static constexpr uint8_t CWGsPReg           = 0x28;
    static constexpr uint8_t ModGsPReg          = 0x29;
    static constexpr uint8_t TModeReg           = 0x2A;
    static constexpr uint8_t TPrescalerReg      = 0x2B;
    static constexpr uint8_t TReloadRegH        = 0x2C;
    static constexpr uint8_t TReloadRegL        = 0x2D;
    static constexpr uint8_t TCounterValueRegH  = 0x2E;
    static constexpr uint8_t TCounterValueRegL  = 0x2F;

    static constexpr uint8_t TestSel1Reg        = 0x31;
    static constexpr uint8_t TestSel2Reg        = 0x32;
    static constexpr uint8_t TestPinEnReg       = 0x33;
    static constexpr uint8_t TestPinValueReg    = 0x34;
    static constexpr uint8_t TestBusReg         = 0x35;
    static constexpr uint8_t AutoTestReg        = 0x36;
    static constexpr uint8_t VersionReg         = 0x37;
    static constexpr uint8_t AnalogTestReg      = 0x38;
    static constexpr uint8_t TestDAC1Reg        = 0x39;
    static constexpr uint8_t TestDAC2Reg        = 0x3A;
    static constexpr uint8_t TestADCReg         = 0x3B;

    // MFRC522 PCD Commands
    static constexpr uint8_t PCD_Idle               = 0x00;
    static constexpr uint8_t PCD_Mem                = 0x01;
    static constexpr uint8_t PCD_GenerateRandomID   = 0x02;
    static constexpr uint8_t PCD_CalcCRC            = 0x03;
    static constexpr uint8_t PCD_Transmit           = 0x04;
    static constexpr uint8_t PCD_NoCmdChange        = 0x07;
    static constexpr uint8_t PCD_Receive            = 0x08;
    static constexpr uint8_t PCD_Transceive         = 0x0C;
    static constexpr uint8_t PCD_MFAuthent          = 0x0E;
    static constexpr uint8_t PCD_SoftReset          = 0x0F;

    // ISO14443A PICC Commands
    static constexpr uint8_t PICC_CMD_REQA          = 0x26;
    static constexpr uint8_t PICC_CMD_WUPA          = 0x52;
    static constexpr uint8_t PICC_CMD_CT            = 0x88;
    static constexpr uint8_t PICC_CMD_SEL_CL1       = 0x93;
    static constexpr uint8_t PICC_CMD_SEL_CL2       = 0x95;
    static constexpr uint8_t PICC_CMD_SEL_CL3       = 0x97;
    static constexpr uint8_t PICC_CMD_HLTA          = 0x50;

private:
    int resetGpioPin;
    SPIClass& spi;

    char cachedUidString[MAX_UID_STRING_LENGTH];
    char lastNotifiedUidString[MAX_UID_STRING_LENGTH];
    uint8_t cachedUidBytes[MAX_UID_BYTES];
    uint8_t cachedUidLength;

    bool initialized;
    bool tagCurrentlyPresent;
    bool lastNotifiedPresence;

    void updateUidCache(const uint8_t* rawBytes, uint8_t length);
    void clearUidCache();

    StatusCode requestAOrWakeUpA(uint8_t command, uint8_t* bufferAtqa, uint8_t* bufferSize);
    StatusCode readCardSerial(uint8_t* outUidBytes, uint8_t& outLength);

protected:
    /**
     * @brief Polled by the Device sampling engine to check for card presence and transitions.
     * @param event Mutable event context.
     */
    void processEvent(Event& event) override;

    [[nodiscard]] uint32_t getDefaultSamplingIntervalInMilliseconds() const override;
    [[nodiscard]] uint32_t getMinimalSamplingIntervalInMilliseconds() const override;

public:
    /**
     * @brief Constructs an RFID sensor instance.
     * @param chipSelectGpioPin SPI Chip Select / Slave Select (SS/SDA) GPIO pin.
     * @param resetGpioPin Hardware reset (RST) GPIO pin (default: -1 if unassigned).
     * @param parentHandler Optional upstream handler for event propagation.
     * @param spiInstance Reference to the SPIClass bus instance (default: SPI).
     */
    explicit RfidSensor(int chipSelectGpioPin, int resetGpioPin = -1, EventHandler* parentHandler = nullptr, SPIClass& spiInstance = SPI);
    ~RfidSensor() override = default;

    /**
     * @brief Initializes hardware registers and SPI transceiver.
     * @return True if communication with MFRC522 is verified, false otherwise.
     */
    bool initialize();

    /**
     * @brief Returns whether the transceiver is currently initialized.
     */
    [[nodiscard]] bool isInitialized() const;

    /**
     * @brief Writes a byte to a register.
     */
    void writeRegister(uint8_t reg, uint8_t value);

    /**
     * @brief Writes multiple bytes to a register.
     */
    void writeRegister(uint8_t reg, uint8_t count, const uint8_t* values);

    /**
     * @brief Reads a byte from a register.
     */
    [[nodiscard]] uint8_t readRegister(uint8_t reg) const;

    /**
     * @brief Reads multiple bytes from a register.
     */
    void readRegister(uint8_t reg, uint8_t count, uint8_t* values, uint8_t rxAlign = 0) const;

    /**
     * @brief Sets bits in a register via OR bitmask.
     */
    void setRegisterBitMask(uint8_t reg, uint8_t mask);

    /**
     * @brief Clears bits in a register via AND NOT bitmask.
     */
    void clearRegisterBitMask(uint8_t reg, uint8_t mask);

    /**
     * @brief Turns RF antenna carrier on or off.
     */
    void setAntennaOn(bool enable);

    /**
     * @brief Executes direct command transceiver exchange with PICC.
     */
    StatusCode communicateWithPicc(
        uint8_t command,
        uint8_t waitIrq,
        const uint8_t* sendData,
        uint8_t sendLen,
        uint8_t* backData = nullptr,
        uint8_t* backLen = nullptr,
        uint8_t* validBits = nullptr,
        uint8_t rxAlign = 0,
        bool checkCrc = false
    );

    /**
     * @brief Calculates CRC coprocessor hardware sum for data payload.
     */
    StatusCode calculateCrc(const uint8_t* data, uint8_t length, uint8_t* result);

    /**
     * @brief Transmits REQA (Request Type A) to detect cards in IDLE state.
     */
    StatusCode requestA(uint8_t* bufferAtqa, uint8_t* bufferSize);

    /**
     * @brief Transmits WUPA (Wake-Up Type A) to detect cards in IDLE or HALT state.
     */
    StatusCode wakeUpA(uint8_t* bufferAtqa, uint8_t* bufferSize);

    /**
     * @brief Puts active PICC into HALT state.
     */
    StatusCode haltA();

    /**
     * @brief Returns the most recently detected UID formatted as a hexadecimal string (e.g., "A1:B2:C3:D4").
     */
    [[nodiscard]] const char* getUid() const;

    /**
     * @brief Returns the raw UID bytes and length.
     */
    [[nodiscard]] const uint8_t* getUidBytes(uint8_t& outLength) const;

    /**
     * @brief Checks if a tag is actively within the RF proximity field.
     */
    [[nodiscard]] bool isTagPresent() const;

    /**
     * @brief Puts the RFID transceiver into low-power sleep mode.
     */
    void powerDown();

    /**
     * @brief Wakes up the transceiver from sleep mode and re-enables the antenna.
     */
    void powerUp();
};

#endif // RFID_SENSOR_H
