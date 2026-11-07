cmake_minimum_required(VERSION 3.15...4.3)

# 1. Project Configuration & Metadata
project(Modest-IoT-Nano-Framework VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. Hardware and Environment Specifications
set(BOARD_FQBN "esp32:esp32:esp32da" CACHE STRING "Target hardware device FQBN selection identifier")
set(PORT "/dev/ttyUSB0" CACHE STRING "Serial flash port for ESP32 layout deployment")
set(ARDUINO_CLI_PATH "/opt/homebrew/bin/arduino-cli" CACHE FILEPATH "Local path to your executable binary")
set(ARDUINO_LIBS "$ENV{HOME}/Documents/Arduino/libraries" CACHE FILEPATH "Path to Arduino libraries")

# 1. Dynamically discover the Arduino data directory
execute_process(
    COMMAND ${ARDUINO_CLI_PATH} config dump --format json
    OUTPUT_VARIABLE ARDUINO_CLI_CONFIG_JSON
)

# Use a regex to extract the data directory from the JSON output if it's set,
# or fallback to the standard macOS default if not.
string(REGEX MATCH "\"data\": \"([^\"]+)\"" _ ${ARDUINO_CLI_CONFIG_JSON})
if (CMAKE_MATCH_1)
    set(ARDUINO_DATA_DIR "${CMAKE_MATCH_1}")
else()
    set(ARDUINO_DATA_DIR "$ENV{HOME}/Library/Arduino15")
endif()

file(GLOB ESP32_PLATFORM_VERSIONS LIST_DIRECTORIES true
        "${ARDUINO_DATA_DIR}/packages/esp32/hardware/esp32/*")
list(SORT ESP32_PLATFORM_VERSIONS COMPARE NATURAL ORDER DESCENDING)
list(GET ESP32_PLATFORM_VERSIONS 0 ESP32_PLATFORM_BASE)
get_filename_component(ESP32_PLATFORM_VERSION "${ESP32_PLATFORM_BASE}" NAME)
set(ESP32_TOOLS_BASE "${ARDUINO_DATA_DIR}/packages/esp32/tools/esp32-libs/${ESP32_PLATFORM_VERSION}")
set(ARDUINO_CORE_PATH "${ESP32_PLATFORM_BASE}/cores/esp32")
set(ARDUINO_ESP32_LIBS_INCLUDE_PATH "${ESP32_TOOLS_BASE}/include")
set(ARDUINO_ESP32_FREERTOS_INCLUDE_PATH "${ESP32_TOOLS_BASE}/include/freertos/FreeRTOS-Kernel/include")
set(ARDUINO_ESP32_FREERTOS_PORTABLE_INCLUDE_PATH "${ESP32_TOOLS_BASE}/include/freertos/FreeRTOS-Kernel/portable/xtensa/include")
set(ARDUINO_ESP32_FREERTOS_CONFIG_INCLUDE_PATH "${ESP32_TOOLS_BASE}/include/freertos/config/include")
set(SKETCH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/examples/ClassicBlinker" CACHE PATH "Path to sketch directory for arduino-cli compilation")

add_definitions(-DARDUINO)

# 3. Native Source Target (Keeps CLion indexing perfect)
file(GLOB_RECURSE LIB_SOURCES "src/*.cpp")
file(GLOB_RECURSE LIB_HEADERS "src/*.h")

add_library(${PROJECT_NAME} STATIC EXCLUDE_FROM_ALL ${LIB_SOURCES} ${LIB_HEADERS})
target_compile_definitions(${PROJECT_NAME} PUBLIC
        IRAM_ATTR=
        __xtensa__=1
        __XTENSA__=1
        ESP32=1
        ARDUINO_ARCH_ESP32=1
        ESP_PLATFORM=1
        SOC_WIFI_SUPPORTED=1
        CONFIG_SOC_WIFI_SUPPORTED=1
)

file(GLOB ESP32_COMPONENT_INCLUDES "${ESP32_TOOLS_BASE}/include/*/include")
file(GLOB ESP32_PORT_INCLUDES "${ESP32_TOOLS_BASE}/include/*/port/include")

target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        "${ARDUINO_LIBS}/ArduinoJson/src"
        "${ARDUINO_LIBS}/PubSubClient/src"
        "${ARDUINO_LIBS}/LiquidCrystal_I2C"
        "${ARDUINO_LIBS}/DHT_sensor_library"
        "${ARDUINO_LIBS}/ESP32Servo/src"
        "${ESP32_PLATFORM_BASE}/libraries/BluetoothSerial/src"
        "${ESP32_PLATFORM_BASE}/libraries/BLE/src"
        "${ESP32_PLATFORM_BASE}/libraries/HTTPClient/src"
        "${ESP32_PLATFORM_BASE}/libraries/WiFi/src"
        "${ESP32_PLATFORM_BASE}/libraries/Wire/src"
        "${ESP32_PLATFORM_BASE}/libraries/SPI/src"
        "${ESP32_PLATFORM_BASE}/libraries/Network/src"
        "${ARDUINO_CORE_PATH}"
        ${ESP32_COMPONENT_INCLUDES}
        ${ESP32_PORT_INCLUDES}
        "${ARDUINO_ESP32_LIBS_INCLUDE_PATH}"
        "${ARDUINO_ESP32_FREERTOS_INCLUDE_PATH}"
        "${ARDUINO_ESP32_FREERTOS_INCLUDE_PATH}/freertos"
        "${ARDUINO_ESP32_FREERTOS_PORTABLE_INCLUDE_PATH}"
        "${ARDUINO_ESP32_FREERTOS_PORTABLE_INCLUDE_PATH}/freertos"
        "${ARDUINO_ESP32_FREERTOS_CONFIG_INCLUDE_PATH}"
        "${ARDUINO_ESP32_FREERTOS_CONFIG_INCLUDE_PATH}/freertos"
        "${ESP32_TOOLS_BASE}/include/freertos/config/xtensa/include"
        "${ESP32_TOOLS_BASE}/include/freertos/config/xtensa/include/freertos"
        "${ESP32_TOOLS_BASE}/include/freertos/esp_additions/include"
        "${ESP32_TOOLS_BASE}/include/freertos/esp_additions/include/freertos"
        "${ESP32_TOOLS_BASE}/include/soc/esp32/include"
        "${ESP32_TOOLS_BASE}/include/esp_common/include"
        "${ESP32_TOOLS_BASE}/include/xtensa/include"
        "${ESP32_TOOLS_BASE}/include/xtensa/esp32/include"
        "${ESP32_TOOLS_BASE}/include/lwip/lwip/src/include"
        "${ESP32_TOOLS_BASE}/qio_qspi/include"
        "${ESP32_PLATFORM_BASE}/variants/esp32da"
        "${ESP32_TOOLS_BASE}/include/newlib/platform_include/"
        "${CMAKE_CURRENT_SOURCE_DIR}/include_indexer"
)

# 4. IDE Code-Completion Indexer Targets
add_library(clion_indexer INTERFACE)
target_compile_definitions(clion_indexer INTERFACE
        IRAM_ATTR=
        __xtensa__=1
        __XTENSA__=1
        ESP32=1
        ARDUINO_ARCH_ESP32=1
        ESP_PLATFORM=1
        SOC_WIFI_SUPPORTED=1
        CONFIG_SOC_WIFI_SUPPORTED=1
)
target_include_directories(clion_indexer INTERFACE
        "${ARDUINO_ESP32_FREERTOS_INCLUDE_PATH}"
        "${ARDUINO_ESP32_FREERTOS_INCLUDE_PATH}/freertos"
        "${ARDUINO_ESP32_FREERTOS_PORTABLE_INCLUDE_PATH}"
        "${ARDUINO_ESP32_FREERTOS_PORTABLE_INCLUDE_PATH}/freertos"
        "${ARDUINO_ESP32_FREERTOS_CONFIG_INCLUDE_PATH}"
        "${ARDUINO_ESP32_FREERTOS_CONFIG_INCLUDE_PATH}/freertos"
        "${ESP32_TOOLS_BASE}/include/freertos/config/xtensa/include"
        "${ESP32_TOOLS_BASE}/include/freertos/config/xtensa/include/freertos"
        "${ESP32_TOOLS_BASE}/include/freertos/esp_additions/include"
        "${ESP32_TOOLS_BASE}/include/freertos/esp_additions/include/freertos"
        "${ESP32_TOOLS_BASE}/include/soc/esp32/include"
        "${ESP32_TOOLS_BASE}/include/esp_common/include"
        "${ESP32_TOOLS_BASE}/include/xtensa/include"
        "${ESP32_TOOLS_BASE}/include/xtensa/esp32/include"
        "${ESP32_TOOLS_BASE}/include/lwip/lwip/src/include"
        "${ESP32_TOOLS_BASE}/qio_qspi/include"
        "${ESP32_PLATFORM_BASE}/variants/esp32da"
        "${ESP32_TOOLS_BASE}/include/newlib/platform_include/"
        "${ESP32_PLATFORM_BASE}/libraries/SPI/src"
)
target_link_libraries(clion_indexer INTERFACE ${PROJECT_NAME})

# 5. Dedicated arduino-cli Target Pipeline
# Compiles your project using the esp32da compiler ecosystem safely isolated from host flags
add_custom_target(arduino_compile
        COMMAND ${ARDUINO_CLI_PATH} compile
        --fqbn ${BOARD_FQBN}
        --library "${CMAKE_CURRENT_SOURCE_DIR}"
        --build-path "${CMAKE_CURRENT_BINARY_DIR}/esp32_out"
        "${SKETCH_DIR}"
        COMMENT "Compiling IoT Firmware via arduino-cli for ${BOARD_FQBN}..."
        VERBATIM
)

# 6. Flash Deployment Target
add_custom_target(arduino_upload
        COMMAND ${ARDUINO_CLI_PATH} upload
        --fqbn ${BOARD_FQBN}
        --port ${PORT}
        --input-dir "${CMAKE_CURRENT_BINARY_DIR}/esp32_out"
        "${SKETCH_DIR}"
        DEPENDS arduino_compile
        COMMENT "Flashing firmware workspace layout to ESP32-WROOM-DA on ${PORT}..."
        VERBATIM
)

# 7. Native Host Unit Tests Target
enable_testing()

file(GLOB TEST_FAKES "tests/fakes/*.h")

add_executable(test_DigitalLightIntensitySensor
        tests/DigitalLightIntensitySensor/test.cpp
        src/Sensor.cpp
        src/DigitalLightIntensitySensor.cpp
        ${TEST_FAKES}
)
target_include_directories(test_DigitalLightIntensitySensor PRIVATE tests/fakes src)

add_executable(test_RgbLed
        tests/RgbLed/test.cpp
        src/Actuator.cpp
        src/RgbLed.cpp
        ${TEST_FAKES}
)
target_include_directories(test_RgbLed PRIVATE tests/fakes src)

add_executable(test_RfidSensor
        tests/RfidSensor/test.cpp
        src/Sensor.cpp
        src/RfidSensor.cpp
        ${TEST_FAKES}
)
target_include_directories(test_RfidSensor PRIVATE tests/fakes src)

add_executable(test_BleConnectivityDriver
        tests/BleConnectivityDriver/test.cpp
        src/BleConnectivityDriver.cpp
        ${TEST_FAKES}
)
target_compile_definitions(test_BleConnectivityDriver PRIVATE TEST_MOCK_BLE)
target_include_directories(test_BleConnectivityDriver PRIVATE tests/fakes src)

add_executable(test_ModestLog
        tests/ModestLog/test.cpp
        ${TEST_FAKES}
)
target_include_directories(test_ModestLog PRIVATE tests/fakes src)

add_executable(test_MqttGatewayClient
        tests/MqttGatewayClient/test.cpp
        src/MqttGatewayClient.cpp
        src/WiFiConnectivityDriver.cpp
        ${TEST_FAKES}
)
target_include_directories(test_MqttGatewayClient PRIVATE tests/fakes src)

add_executable(test_EngineConfig
        tests/EngineConfig/test.cpp
        ${TEST_FAKES}
)
target_include_directories(test_EngineConfig PRIVATE tests/fakes src)

add_executable(test_DiagnosticsSnapshot
        tests/DiagnosticsSnapshot/test.cpp
        ${TEST_FAKES}
)
target_compile_definitions(test_DiagnosticsSnapshot PRIVATE TEST_MOCK_DEVICE)
target_include_directories(test_DiagnosticsSnapshot PRIVATE tests/fakes src)

add_executable(test_SensorFault
        tests/SensorFault/test.cpp
        src/Sensor.cpp
        ${TEST_FAKES}
)
target_include_directories(test_SensorFault PRIVATE tests/fakes src)

add_executable(test_SamplingScheduler
        tests/SamplingScheduler/test.cpp
        src/SamplingScheduler.cpp
        ${TEST_FAKES}
)
target_include_directories(test_SamplingScheduler PRIVATE tests/fakes src)

add_executable(test_TelemetryPackage
        tests/TelemetryPackage/test.cpp
        ${TEST_FAKES}
)
target_include_directories(test_TelemetryPackage PRIVATE tests/fakes src)

add_test(NAME DigitalLightIntensitySensorTest COMMAND test_DigitalLightIntensitySensor)
add_test(NAME RgbLedTest COMMAND test_RgbLed)
add_test(NAME RfidSensorTest COMMAND test_RfidSensor)
add_test(NAME BleConnectivityDriverTest COMMAND test_BleConnectivityDriver)
add_test(NAME ModestLogTest COMMAND test_ModestLog)
add_test(NAME MqttGatewayClientTest COMMAND test_MqttGatewayClient)
add_test(NAME EngineConfigTest COMMAND test_EngineConfig)
add_test(NAME DiagnosticsSnapshotTest COMMAND test_DiagnosticsSnapshot)
add_test(NAME SensorFaultTest COMMAND test_SensorFault)
add_test(NAME SamplingSchedulerTest COMMAND test_SamplingScheduler)
add_test(NAME TelemetryPackageTest COMMAND test_TelemetryPackage)

add_custom_target(run_tests
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        DEPENDS test_DigitalLightIntensitySensor test_RgbLed test_RfidSensor test_BleConnectivityDriver test_ModestLog test_MqttGatewayClient test_EngineConfig test_DiagnosticsSnapshot test_SensorFault test_SamplingScheduler test_TelemetryPackage
        COMMENT "Executing all native host test suites..."
)
