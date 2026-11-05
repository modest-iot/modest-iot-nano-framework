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
run_test "test_digital_light" \
    src/Sensor.cpp \
    src/DigitalLightIntensitySensor.cpp \
    tests/digital_light/test.cpp

# 2. RGB LED Actuator Suite
run_test "test_rgb_led" \
    src/Actuator.cpp \
    src/RgbLed.cpp \
    tests/rgb_led/test.cpp

# 3. RFID Sensor Suite
run_test "test_rfid_sensor" \
    src/Sensor.cpp \
    src/RfidSensor.cpp \
    tests/rfid_sensor/test.cpp

# 4. BLE Connectivity Driver Suite
run_test "test_ble_connectivity" \
    -DTEST_MOCK_BLE \
    src/BleConnectivityDriver.cpp \
    tests/ble_connectivity/test.cpp

# 5. Logging and MIOT_LOG Aliases Suite
run_test "test_log" \
    tests/log/test.cpp

# 6. MQTT Gateway and WiFi Reconnection Suite
run_test "test_mqtt_gateway" \
    src/MqttGatewayClient.cpp \
    src/WiFiConnectivityDriver.cpp \
    tests/mqtt_gateway/test.cpp

# 7. EngineConfig and Device Configuration Suite
run_test "test_engine_config" \
    tests/engine_config/test.cpp

# 8. Diagnostics Snapshot and Shared Counters Suite
run_test "test_diagnostics" \
    -DTEST_MOCK_DEVICE \
    tests/diagnostics/test.cpp

# 9. Sensor Fault and Recovery Events Suite
run_test "test_sensor_fault" \
    src/Sensor.cpp \
    tests/sensor_fault/test.cpp

echo "=========================================================="
echo " Test Summary: ${PASSED} passed, ${FAILED} failed"
echo "=========================================================="

if [[ ${FAILED} -ne 0 ]]; then
    exit 1
fi

exit 0
