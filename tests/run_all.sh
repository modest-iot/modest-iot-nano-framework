#!/usr/bin/env bash
set -euo pipefail

# Modest IoT Nano Framework - Unified Host Test Runner
# Builds and runs all host-based unit tests with strict compiler warnings and optional sanitizers.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

cd "${PROJECT_ROOT}"

CXX="${CXX:-g++}"
SANITIZERS="${ENABLE_SANITIZERS:-1}"

BASE_FLAGS="-std=c++17 -Wall -Wextra -Werror -Itests/fakes -Isrc"
if [[ "${SANITIZERS}" == "1" ]]; then
    EXTRA_FLAGS="-fsanitize=address,undefined"
else
    EXTRA_FLAGS=""
fi

BUILD_DIR="$(mktemp -d /tmp/modest_iot_tests.XXXXXX)"
cleanup() {
    rm -rf "${BUILD_DIR}"
}
trap cleanup EXIT

echo "=========================================================="
echo " Running Modest IoT Native Host Unit Tests"
echo " Compiler: ${CXX}"
echo " Flags:    ${BASE_FLAGS} ${EXTRA_FLAGS}"
echo "=========================================================="

PASSED=0
FAILED=0

run_test() {
    local test_name="$1"
    local binary="${BUILD_DIR}/${test_name}"
    shift
    local sources=("$@")

    echo -n "==> [BUILD] ${test_name} ... "
    if ${CXX} ${BASE_FLAGS} ${EXTRA_FLAGS} "${sources[@]}" -o "${binary}"; then
        echo "OK"
    else
        echo "FAILED (compilation)"
        FAILED=$((FAILED + 1))
        return 1
    fi

    echo -n "==> [RUN]   ${test_name} ... "
    if "${binary}"; then
        echo "PASS"
        PASSED=$((PASSED + 1))
    else
        echo "FAIL"
        FAILED=$((FAILED + 1))
        return 1
    fi
}

# 1. Digital Light Sensor Suite
run_test "test_DigitalLightIntensitySensor" \
    tests/fakes/ArduinoFake.cpp \
    src/Sensor.cpp \
    src/DigitalLightIntensitySensor.cpp \
    tests/DigitalLightIntensitySensor/test.cpp

# 2. RGB LED Actuator Suite
run_test "test_RgbLed" \
    tests/fakes/ArduinoFake.cpp \
    src/Actuator.cpp \
    src/RgbLed.cpp \
    tests/RgbLed/test.cpp

# 3. RFID Sensor Suite
run_test "test_RfidSensor" \
    tests/fakes/ArduinoFake.cpp \
    src/Sensor.cpp \
    src/RfidSensor.cpp \
    tests/RfidSensor/test.cpp

# 4. BLE Connectivity Driver Suite
run_test "test_BleConnectivityDriver" \
    -DTEST_MOCK_BLE \
    tests/fakes/ArduinoFake.cpp \
    src/BleConnectivityDriver.cpp \
    tests/BleConnectivityDriver/test.cpp

# 5. Logging and MIOT_LOG Aliases Suite
run_test "test_ModestLog" \
    tests/ModestLog/test.cpp

# 6. MQTT Gateway and WiFi Reconnection Suite
run_test "test_MqttGatewayClient" \
    tests/fakes/ArduinoFake.cpp \
    src/MqttGatewayClient.cpp \
    src/WiFiConnectivityDriver.cpp \
    src/Device.cpp \
    src/SamplingScheduler.cpp \
    src/Sensor.cpp \
    src/JsonTelemetryWriter.cpp \
    src/JsonParser.cpp \
    src/DiagnosticsTelemetryPackage.cpp \
    tests/MqttGatewayClient/test.cpp

# 7. EngineConfiguration and Device Configuration Suite
run_test "test_EngineConfiguration" \
    tests/fakes/ArduinoFake.cpp \
    tests/EngineConfiguration/test.cpp

# 8. Diagnostics Snapshot and Shared Counters Suite
run_test "test_DiagnosticsSnapshot" \
    -DTEST_MOCK_DEVICE \
    tests/fakes/ArduinoFake.cpp \
    tests/DiagnosticsSnapshot/test.cpp

# 9. Sensor Fault and Recovery Events Suite
run_test "test_SensorFault" \
    tests/fakes/ArduinoFake.cpp \
    src/Sensor.cpp \
    tests/SensorFault/test.cpp

# 10. SamplingScheduler Suite
run_test "test_SamplingScheduler" \
    tests/fakes/ArduinoFake.cpp \
    src/SamplingScheduler.cpp \
    tests/SamplingScheduler/test.cpp

# 11. Telemetry Serialization Golden Tests Suite
run_test "test_TelemetryPackage" \
    tests/fakes/ArduinoFake.cpp \
    src/Device.cpp \
    src/SamplingScheduler.cpp \
    src/Sensor.cpp \
    src/JsonTelemetryWriter.cpp \
    src/JsonParser.cpp \
    src/DiagnosticsTelemetryPackage.cpp \
    tests/TelemetryPackage/test.cpp

# 11b. JsonParser Suite
run_test "test_JsonParser" \
    src/JsonParser.cpp \
    tests/JsonParser/test.cpp

# 12. Triggering Scheduler Suite
run_test "test_TriggeringScheduler" \
    tests/fakes/ArduinoFake.cpp \
    src/Sensor.cpp \
    src/Device.cpp \
    src/SamplingScheduler.cpp \
    src/JsonTelemetryWriter.cpp \
    src/JsonParser.cpp \
    src/DiagnosticsTelemetryPackage.cpp \
    tests/TriggeringScheduler/test.cpp

# 13. Command Marshaling Suite
run_test "test_CommandMarshaling" \
    tests/fakes/ArduinoFake.cpp \
    src/Device.cpp \
    src/SamplingScheduler.cpp \
    src/Sensor.cpp \
    src/JsonTelemetryWriter.cpp \
    src/JsonParser.cpp \
    src/DiagnosticsTelemetryPackage.cpp \
    tests/CommandMarshaling/test.cpp

# 14. WaterFlowSensor Suite
run_test "test_WaterFlowSensor" \
    tests/fakes/ArduinoFake.cpp \
    src/Sensor.cpp \
    src/WaterFlowSensor.cpp \
    tests/WaterFlowSensor/test.cpp

# 15. SoilMoistureSensor Suite
run_test "test_SoilMoistureSensor" \
    tests/fakes/ArduinoFake.cpp \
    src/Sensor.cpp \
    src/SoilMoistureSensor.cpp \
    tests/SoilMoistureSensor/test.cpp

# 16. RelayActuator Suite
run_test "test_RelayActuator" \
    tests/fakes/ArduinoFake.cpp \
    src/Actuator.cpp \
    src/RelayActuator.cpp \
    tests/RelayActuator/test.cpp

# 17. DeepSleep Suite
run_test "test_DeepSleep" \
    tests/fakes/ArduinoFake.cpp \
    src/Device.cpp \
    src/SamplingScheduler.cpp \
    src/Sensor.cpp \
    src/JsonTelemetryWriter.cpp \
    src/JsonParser.cpp \
    src/DiagnosticsTelemetryPackage.cpp \
    tests/DeepSleep/test.cpp

# 18. ServoMotor Suite
run_test "test_ServoMotor" \
    tests/fakes/ArduinoFake.cpp \
    src/Actuator.cpp \
    src/ServoMotor.cpp \
    tests/ServoMotor/test.cpp

# 19. CharacterLcdDisplay Suite
run_test "test_CharacterLcdDisplay" \
    tests/fakes/ArduinoFake.cpp \
    src/Actuator.cpp \
    src/CharacterLcdDisplay.cpp \
    src/Hd44780Driver.cpp \
    tests/CharacterLcdDisplay/test.cpp

# 20. OledDisplay Suite
run_test "test_OledDisplay" \
    tests/fakes/ArduinoFake.cpp \
    src/Actuator.cpp \
    src/OledDisplay.cpp \
    src/Ssd1306Driver.cpp \
    tests/OledDisplay/test.cpp

# 21. PthSensor Suite
run_test "test_PthSensor" \
    tests/fakes/ArduinoFake.cpp \
    src/Sensor.cpp \
    src/BmeEnvironmentalSensor.cpp \
    src/PthSensor.cpp \
    tests/PthSensor/test.cpp

# 22. WaterLeakSensor Suite
run_test "test_WaterLeakSensor" \
    tests/fakes/ArduinoFake.cpp \
    src/Sensor.cpp \
    src/WaterLeakSensor.cpp \
    tests/WaterLeakSensor/test.cpp

# 23. DoorWindowSensor Suite
run_test "test_DoorWindowSensor" \
    tests/fakes/ArduinoFake.cpp \
    src/Sensor.cpp \
    src/DoorWindowSensor.cpp \
    tests/DoorWindowSensor/test.cpp

# 24. OneWireTemperatureSensor Suite
run_test "test_OneWireTemperatureSensor" \
    tests/fakes/ArduinoFake.cpp \
    src/Sensor.cpp \
    src/OneWireTemperatureSensor.cpp \
    tests/OneWireTemperatureSensor/test.cpp

# 25. DhtSensor Suite
run_test "test_DhtSensor" \
    tests/fakes/ArduinoFake.cpp \
    src/Sensor.cpp \
    src/DhtSensor.cpp \
    tests/DhtSensor/test.cpp

# 26. EnvironmentalGasSensor Suite
run_test "test_EnvironmentalGasSensor" \
    tests/fakes/ArduinoFake.cpp \
    src/Sensor.cpp \
    src/BmeEnvironmentalSensor.cpp \
    src/EnvironmentalGasSensor.cpp \
    tests/EnvironmentalGasSensor/test.cpp

# 27. BleGatewayClient Suite
run_test "test_BleGatewayClient" \
    -DTEST_MOCK_BLE \
    tests/fakes/ArduinoFake.cpp \
    src/BleGatewayClient.cpp \
    src/BleConnectivityDriver.cpp \
    src/JsonTelemetryWriter.cpp \
    src/JsonParser.cpp \
    tests/BleGatewayClient/test.cpp

# 28. HttpGatewayClient Suite
run_test "test_HttpGatewayClient" \
    tests/fakes/ArduinoFake.cpp \
    src/HttpGatewayClient.cpp \
    src/WiFiConnectivityDriver.cpp \
    src/JsonTelemetryWriter.cpp \
    src/JsonParser.cpp \
    tests/HttpGatewayClient/test.cpp

# 29. Buzzer Suite
run_test "test_Buzzer" \
    tests/fakes/ArduinoFake.cpp \
    src/Actuator.cpp \
    src/Buzzer.cpp \
    tests/Buzzer/test.cpp

# 30. ConfigurationStore Suite
run_test "test_ConfigurationStore" \
    tests/fakes/ArduinoFake.cpp \
    src/ConfigurationStore.cpp \
    tests/ConfigurationStore/test.cpp

# 31. TimeSynchronizationDriver Suite
run_test "test_TimeSynchronizationDriver" \
    tests/fakes/ArduinoFake.cpp \
    src/TimeSynchronizationDriver.cpp \
    tests/TimeSynchronizationDriver/test.cpp

# 32. GasSensor Suite
run_test "test_GasSensor" \
    tests/fakes/ArduinoFake.cpp \
    src/Sensor.cpp \
    src/GasSensor.cpp \
    tests/GasSensor/test.cpp

# 33. Button Suite
run_test "test_Button" \
    tests/fakes/ArduinoFake.cpp \
    src/Sensor.cpp \
    src/Button.cpp \
    tests/Button/test.cpp

# 34. PowerManager Suite
run_test "test_PowerManager" \
    tests/fakes/ArduinoFake.cpp \
    src/PowerManager.cpp \
    tests/PowerManager/test.cpp

# 35. TelemetryBuffer Suite
run_test "test_TelemetryBuffer" \
    tests/fakes/ArduinoFake.cpp \
    src/TelemetryBuffer.cpp \
    tests/TelemetryBuffer/test.cpp

# 36. OtaUpdateHandler Suite
run_test "test_OtaUpdateHandler" \
    tests/fakes/ArduinoFake.cpp \
    src/OtaUpdateHandler.cpp \
    tests/OtaUpdateHandler/test.cpp

# 37. Led Suite
run_test "test_Led" \
    tests/fakes/ArduinoFake.cpp \
    src/Actuator.cpp \
    src/Led.cpp \
    tests/Led/test.cpp

# 38. PassiveInfraredSensor Suite
run_test "test_PassiveInfraredSensor" \
    tests/fakes/ArduinoFake.cpp \
    src/Sensor.cpp \
    src/PassiveInfraredSensor.cpp \
    tests/PassiveInfraredSensor/test.cpp

# 39. PulseSensor Suite
run_test "test_PulseSensor" \
    tests/fakes/ArduinoFake.cpp \
    src/Sensor.cpp \
    src/PulseSensor.cpp \
    tests/PulseSensor/test.cpp

# 40. Hd44780Driver Suite
run_test "test_Hd44780Driver" \
    tests/fakes/ArduinoFake.cpp \
    src/Hd44780Driver.cpp \
    tests/Hd44780Driver/test.cpp

# 41. Ssd1306Driver Suite
run_test "test_Ssd1306Driver" \
    tests/fakes/ArduinoFake.cpp \
    src/Ssd1306Driver.cpp \
    tests/Ssd1306Driver/test.cpp

# 42. WiFiConnectivityDriver Suite
run_test "test_WiFiConnectivityDriver" \
    tests/fakes/ArduinoFake.cpp \
    src/WiFiConnectivityDriver.cpp \
    tests/WiFiConnectivityDriver/test.cpp

# 43. BmeEnvironmentalSensor Suite
run_test "test_BmeEnvironmentalSensor" \
    tests/fakes/ArduinoFake.cpp \
    src/Sensor.cpp \
    src/BmeEnvironmentalSensor.cpp \
    src/PthSensor.cpp \
    tests/BmeEnvironmentalSensor/test.cpp

# 44. UltrasonicSensor Suite
run_test "test_UltrasonicSensor" \
    tests/fakes/ArduinoFake.cpp \
    src/Sensor.cpp \
    src/UltrasonicSensor.cpp \
    tests/UltrasonicSensor/test.cpp

# 45. LoadCellAmplifier Suite
run_test "test_LoadCellAmplifier" \
    tests/fakes/ArduinoFake.cpp \
    src/Sensor.cpp \
    src/LoadCellAmplifier.cpp \
    tests/LoadCellAmplifier/test.cpp

# 46. GeolocationSensor Suite
run_test "test_GeolocationSensor" \
    tests/fakes/ArduinoFake.cpp \
    src/Sensor.cpp \
    src/GeolocationSensor.cpp \
    tests/GeolocationSensor/test.cpp

# 47. Device Suite
run_test "test_Device" \
    tests/fakes/ArduinoFake.cpp \
    src/Sensor.cpp \
    src/Actuator.cpp \
    src/Device.cpp \
    src/SamplingScheduler.cpp \
    src/JsonTelemetryWriter.cpp \
    src/JsonParser.cpp \
    src/DiagnosticsTelemetryPackage.cpp \
    tests/Device/test.cpp

# 48. EventAggregator Suite
run_test "test_EventAggregator" \
    tests/fakes/ArduinoFake.cpp \
    src/EventAggregator.cpp \
    src/TimeSynchronizationDriver.cpp \
    tests/EventAggregator/test.cpp

# 49. ChangeFilter Suite
run_test "test_ChangeFilter" \
    tests/ChangeFilter/test.cpp

# 50. AiMicroModelController Suite
run_test "test_AiMicroModelController" \
    -DUSE_EMLEARN \
    tests/AiMicroModelController/test.cpp

echo "=========================================================="
echo " Test Summary: ${PASSED} passed, ${FAILED} failed"
echo "=========================================================="

if [[ ${FAILED} -ne 0 ]]; then
    exit 1
fi

exit 0
