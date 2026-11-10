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
    src/Sensor.cpp \
    src/DigitalLightIntensitySensor.cpp \
    tests/DigitalLightIntensitySensor/test.cpp

# 2. RGB LED Actuator Suite
run_test "test_RgbLed" \
    src/Actuator.cpp \
    src/RgbLed.cpp \
    tests/RgbLed/test.cpp

# 3. RFID Sensor Suite
run_test "test_RfidSensor" \
    src/Sensor.cpp \
    src/RfidSensor.cpp \
    tests/RfidSensor/test.cpp

# 4. BLE Connectivity Driver Suite
run_test "test_BleConnectivityDriver" \
    -DTEST_MOCK_BLE \
    src/BleConnectivityDriver.cpp \
    tests/BleConnectivityDriver/test.cpp

# 5. Logging and MIOT_LOG Aliases Suite
run_test "test_ModestLog" \
    tests/ModestLog/test.cpp

# 6. MQTT Gateway and WiFi Reconnection Suite
run_test "test_MqttGatewayClient" \
    src/MqttGatewayClient.cpp \
    src/WiFiConnectivityDriver.cpp \
    src/Device.cpp \
    src/SamplingScheduler.cpp \
    tests/MqttGatewayClient/test.cpp

# 7. EngineConfig and Device Configuration Suite
run_test "test_EngineConfig" \
    tests/EngineConfig/test.cpp

# 8. Diagnostics Snapshot and Shared Counters Suite
run_test "test_DiagnosticsSnapshot" \
    -DTEST_MOCK_DEVICE \
    tests/DiagnosticsSnapshot/test.cpp

# 9. Sensor Fault and Recovery Events Suite
run_test "test_SensorFault" \
    src/Sensor.cpp \
    tests/SensorFault/test.cpp

# 10. SamplingScheduler Suite
run_test "test_SamplingScheduler" \
    src/SamplingScheduler.cpp \
    tests/SamplingScheduler/test.cpp

# 11. Telemetry Serialization Golden Tests Suite
run_test "test_TelemetryPackage" \
    tests/TelemetryPackage/test.cpp

# 12. Triggering Scheduler Suite
run_test "test_TriggeringScheduler" \
    src/Sensor.cpp \
    src/Device.cpp \
    src/SamplingScheduler.cpp \
    tests/TriggeringScheduler/test.cpp

# 13. Command Marshaling Suite
run_test "test_CommandMarshaling" \
    src/Device.cpp \
    src/SamplingScheduler.cpp \
    tests/CommandMarshaling/test.cpp

# 14. WaterFlowSensor Suite
run_test "test_WaterFlowSensor" \
    src/Sensor.cpp \
    src/WaterFlowSensor.cpp \
    tests/WaterFlowSensor/test.cpp

# 15. SoilMoistureSensor Suite
run_test "test_SoilMoistureSensor" \
    src/Sensor.cpp \
    src/SoilMoistureSensor.cpp \
    tests/SoilMoistureSensor/test.cpp

# 16. RelayActuator Suite
run_test "test_RelayActuator" \
    src/Actuator.cpp \
    src/RelayActuator.cpp \
    tests/RelayActuator/test.cpp

# 17. DeepSleep Suite
run_test "test_DeepSleep" \
    src/Device.cpp \
    src/SamplingScheduler.cpp \
    src/Sensor.cpp \
    tests/DeepSleep/test.cpp

# 18. ServoMotor Suite
run_test "test_ServoMotor" \
    src/Actuator.cpp \
    src/ServoMotor.cpp \
    tests/ServoMotor/test.cpp

# 19. CharacterLcdDisplay Suite
run_test "test_CharacterLcdDisplay" \
    src/Actuator.cpp \
    src/CharacterLcdDisplay.cpp \
    src/Hd44780Driver.cpp \
    tests/CharacterLcdDisplay/test.cpp

# 20. PthSensor Suite
run_test "test_PthSensor" \
    src/Sensor.cpp \
    src/PthSensor.cpp \
    tests/PthSensor/test.cpp

# 21. WaterLeakSensor Suite
run_test "test_WaterLeakSensor" \
    src/Sensor.cpp \
    src/WaterLeakSensor.cpp \
    tests/WaterLeakSensor/test.cpp

# 22. DoorWindowSensor Suite
run_test "test_DoorWindowSensor" \
    src/Sensor.cpp \
    src/DoorWindowSensor.cpp \
    tests/DoorWindowSensor/test.cpp

echo "=========================================================="
echo " Test Summary: ${PASSED} passed, ${FAILED} failed"
echo "=========================================================="

if [[ ${FAILED} -ne 0 ]]; then
    exit 1
fi

exit 0
