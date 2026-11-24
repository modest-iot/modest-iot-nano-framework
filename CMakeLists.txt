cmake_minimum_required(VERSION 3.15...4.3)

# 1. Project Configuration & Metadata
project(Modest-IoT-Nano-Framework VERSION 1.2.1 LANGUAGES C CXX)

# Work around macOS SDK bug where <atomic> triggers <ctime> before <time.h> defines tm
add_compile_options(-D_LIBCPP_DISABLE_AVAILABILITY)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directories to keep build artifacts organized within the build folder
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

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
set(SKETCH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/examples/StatusIndicator" CACHE PATH "Path to sketch directory for arduino-cli compilation")

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
        "${ARDUINO_LIBS}/MQTT/src"
        "${ESP32_PLATFORM_BASE}/libraries/BluetoothSerial/src"
        "${ESP32_PLATFORM_BASE}/libraries/BLE/src"
        "${ESP32_PLATFORM_BASE}/libraries/BLE/src/core"
        "${ESP32_PLATFORM_BASE}/libraries/BLE/src/server"
        "${ESP32_PLATFORM_BASE}/libraries/BLE/src/types"
        "${ESP32_PLATFORM_BASE}/libraries/HTTPClient/src"
        "${ESP32_PLATFORM_BASE}/libraries/WiFi/src"
        "${ESP32_PLATFORM_BASE}/libraries/Wire/src"
        "${ESP32_PLATFORM_BASE}/libraries/SPI/src"
        "${ESP32_PLATFORM_BASE}/libraries/Network/src"
        "${ESP32_PLATFORM_BASE}/libraries/NetworkClientSecure/src"
        "${ESP32_PLATFORM_BASE}/libraries/Preferences/src"
        "${ESP32_PLATFORM_BASE}/libraries/FS/src"
        "${ESP32_PLATFORM_BASE}/libraries/LittleFS/src"
        "${ESP32_PLATFORM_BASE}/libraries/ArduinoOTA/src"
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
        "${ESP32_PLATFORM_BASE}/libraries/NetworkClientSecure/src"
        "${ESP32_PLATFORM_BASE}/libraries/Preferences/src"
        "${ESP32_PLATFORM_BASE}/libraries/FS/src"
        "${ESP32_PLATFORM_BASE}/libraries/LittleFS/src"
        "${ESP32_PLATFORM_BASE}/libraries/ArduinoOTA/src"
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
file(GLOB TEST_FAKES_SRC "tests/fakes/*.cpp")

add_executable(test_DigitalLightIntensitySensor
        tests/DigitalLightIntensitySensor/test.cpp
        src/Sensor.cpp
        src/DigitalLightIntensitySensor.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_DigitalLightIntensitySensor PRIVATE tests/fakes src)

add_executable(test_RgbLed
        tests/RgbLed/test.cpp
        src/Actuator.cpp
        src/RgbLed.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_RgbLed PRIVATE tests/fakes src)

add_executable(test_RelayActuator
        tests/RelayActuator/test.cpp
        src/Actuator.cpp
        src/RelayActuator.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_RelayActuator PRIVATE tests/fakes src)

add_executable(test_RfidSensor
        tests/RfidSensor/test.cpp
        src/Sensor.cpp
        src/RfidSensor.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_RfidSensor PRIVATE tests/fakes src)

add_executable(test_BleConnectivityDriver
        tests/BleConnectivityDriver/test.cpp
        src/BleConnectivityDriver.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_compile_definitions(test_BleConnectivityDriver PRIVATE TEST_MOCK_BLE)
target_include_directories(test_BleConnectivityDriver PRIVATE tests/fakes src)

add_executable(test_ModestLog
        tests/ModestLog/test.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_ModestLog PRIVATE tests/fakes src)

add_executable(test_MqttGatewayClient
        tests/MqttGatewayClient/test.cpp
        src/MqttGatewayClient.cpp
        src/WiFiConnectivityDriver.cpp
        src/Device.cpp
        src/SamplingScheduler.cpp
        src/Sensor.cpp
        src/JsonTelemetryWriter.cpp
        src/JsonParser.cpp
        src/DiagnosticsTelemetryPackage.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_MqttGatewayClient PRIVATE tests/fakes src)

add_executable(test_EngineConfiguration
        tests/EngineConfiguration/test.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_EngineConfiguration PRIVATE tests/fakes src)

add_executable(test_DiagnosticsSnapshot
        tests/DiagnosticsSnapshot/test.cpp
        src/JsonTelemetryWriter.cpp
        src/JsonParser.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_compile_definitions(test_DiagnosticsSnapshot PRIVATE TEST_MOCK_DEVICE)
target_include_directories(test_DiagnosticsSnapshot PRIVATE tests/fakes src)

add_executable(test_SensorFault
        tests/SensorFault/test.cpp
        src/Sensor.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_SensorFault PRIVATE tests/fakes src)

add_executable(test_SamplingScheduler
        tests/SamplingScheduler/test.cpp
        src/SamplingScheduler.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_SamplingScheduler PRIVATE tests/fakes src)

add_executable(test_TelemetryPackage
        tests/TelemetryPackage/test.cpp
        src/Device.cpp
        src/SamplingScheduler.cpp
        src/Sensor.cpp
        src/JsonTelemetryWriter.cpp
        src/JsonParser.cpp
        src/DiagnosticsTelemetryPackage.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_TelemetryPackage PRIVATE tests/fakes src)

add_executable(test_JsonParser
        tests/JsonParser/test.cpp
        src/JsonParser.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_JsonParser PRIVATE tests/fakes src)

add_executable(test_TriggeringScheduler
        tests/TriggeringScheduler/test.cpp
        src/Sensor.cpp
        src/Device.cpp
        src/SamplingScheduler.cpp
        src/JsonTelemetryWriter.cpp
        src/JsonParser.cpp
        src/DiagnosticsTelemetryPackage.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_TriggeringScheduler PRIVATE tests/fakes src)

add_executable(test_CommandMarshaling
        tests/CommandMarshaling/test.cpp
        src/Device.cpp
        src/SamplingScheduler.cpp
        src/Sensor.cpp
        src/JsonTelemetryWriter.cpp
        src/JsonParser.cpp
        src/DiagnosticsTelemetryPackage.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_CommandMarshaling PRIVATE tests/fakes src)

add_executable(test_WaterFlowSensor
        tests/WaterFlowSensor/test.cpp
        src/Sensor.cpp
        src/WaterFlowSensor.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_WaterFlowSensor PRIVATE tests/fakes src)

add_executable(test_SoilMoistureSensor
        tests/SoilMoistureSensor/test.cpp
        src/Sensor.cpp
        src/SoilMoistureSensor.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_SoilMoistureSensor PRIVATE tests/fakes src)

add_executable(test_PthSensor
        tests/PthSensor/test.cpp
        src/Sensor.cpp
        src/BmeEnvironmentalSensor.cpp
        src/PthSensor.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_PthSensor PRIVATE tests/fakes src)

add_executable(test_WaterLeakSensor
        tests/WaterLeakSensor/test.cpp
        src/Sensor.cpp
        src/WaterLeakSensor.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_WaterLeakSensor PRIVATE tests/fakes src)

add_executable(test_DoorWindowSensor
        tests/DoorWindowSensor/test.cpp
        src/Sensor.cpp
        src/DoorWindowSensor.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_DoorWindowSensor PRIVATE tests/fakes src)

add_executable(test_OneWireTemperatureSensor
        tests/OneWireTemperatureSensor/test.cpp
        src/Sensor.cpp
        src/OneWireTemperatureSensor.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_OneWireTemperatureSensor PRIVATE tests/fakes src)

add_executable(test_DhtSensor
        tests/DhtSensor/test.cpp
        src/Sensor.cpp
        src/DhtSensor.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_DhtSensor PRIVATE tests/fakes src)

add_executable(test_DeepSleep
        tests/DeepSleep/test.cpp
        src/Device.cpp
        src/SamplingScheduler.cpp
        src/Sensor.cpp
        src/JsonTelemetryWriter.cpp
        src/JsonParser.cpp
        src/DiagnosticsTelemetryPackage.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_DeepSleep PRIVATE tests/fakes src)

add_executable(test_ServoMotor
        tests/ServoMotor/test.cpp
        src/Actuator.cpp
        src/ServoMotor.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_ServoMotor PRIVATE tests/fakes src)

add_executable(test_CharacterLcdDisplay
        tests/CharacterLcdDisplay/test.cpp
        src/Actuator.cpp
        src/CharacterLcdDisplay.cpp
        src/Hd44780Driver.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_CharacterLcdDisplay PRIVATE tests/fakes src)

add_executable(test_OledDisplay
        tests/OledDisplay/test.cpp
        src/Actuator.cpp
        src/OledDisplay.cpp
        src/Ssd1306Driver.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_OledDisplay PRIVATE tests/fakes src)

add_executable(test_EnvironmentalGasSensor
        tests/EnvironmentalGasSensor/test.cpp
        src/Sensor.cpp
        src/BmeEnvironmentalSensor.cpp
        src/EnvironmentalGasSensor.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_EnvironmentalGasSensor PRIVATE tests/fakes src)

add_executable(test_BleGatewayClient
        tests/BleGatewayClient/test.cpp
        src/BleGatewayClient.cpp
        src/BleConnectivityDriver.cpp
        src/JsonTelemetryWriter.cpp
        src/JsonParser.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_compile_definitions(test_BleGatewayClient PRIVATE TEST_MOCK_BLE)
target_include_directories(test_BleGatewayClient PRIVATE tests/fakes src)

add_executable(test_HttpGatewayClient
        tests/HttpGatewayClient/test.cpp
        src/HttpGatewayClient.cpp
        src/WiFiConnectivityDriver.cpp
        src/JsonTelemetryWriter.cpp
        src/JsonParser.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_HttpGatewayClient PRIVATE tests/fakes src)

add_executable(test_Buzzer
        tests/Buzzer/test.cpp
        src/Buzzer.cpp
        src/Actuator.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_Buzzer PRIVATE tests/fakes src)

add_executable(test_ConfigurationStore
        tests/ConfigurationStore/test.cpp
        src/ConfigurationStore.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_ConfigurationStore PRIVATE tests/fakes src)

add_executable(test_TimeSynchronizationDriver
        tests/TimeSynchronizationDriver/test.cpp
        src/TimeSynchronizationDriver.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_TimeSynchronizationDriver PRIVATE tests/fakes src)

add_executable(test_GasSensor
        tests/GasSensor/test.cpp
        src/Sensor.cpp
        src/GasSensor.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_GasSensor PRIVATE tests/fakes src)

add_executable(test_Button
        tests/Button/test.cpp
        src/Sensor.cpp
        src/Button.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_Button PRIVATE tests/fakes src)
target_compile_options(test_Button PRIVATE -Wall -Wextra -Werror)

add_executable(test_PowerManager
        tests/PowerManager/test.cpp
        src/PowerManager.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_PowerManager PRIVATE tests/fakes src)
target_compile_options(test_PowerManager PRIVATE -Wall -Wextra -Werror)

add_executable(test_TelemetryBuffer
        tests/TelemetryBuffer/test.cpp
        src/TelemetryBuffer.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_TelemetryBuffer PRIVATE tests/fakes src)
target_compile_options(test_TelemetryBuffer PRIVATE -Wall -Wextra -Werror)

add_executable(test_OtaUpdateHandler
        tests/OtaUpdateHandler/test.cpp
        src/OtaUpdateHandler.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_OtaUpdateHandler PRIVATE tests/fakes src)
target_compile_options(test_OtaUpdateHandler PRIVATE -Wall -Wextra -Werror)

# --- New tests for previously untested components ---

add_executable(test_Led
        tests/Led/test.cpp
        src/Led.cpp
        src/Actuator.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_Led PRIVATE tests/fakes src)
target_compile_options(test_Led PRIVATE -Wall -Wextra -Werror)

add_executable(test_PassiveInfraredSensor
        tests/PassiveInfraredSensor/test.cpp
        src/Sensor.cpp
        src/PassiveInfraredSensor.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_PassiveInfraredSensor PRIVATE tests/fakes src)
target_compile_options(test_PassiveInfraredSensor PRIVATE -Wall -Wextra -Werror)

add_executable(test_PulseSensor
        tests/PulseSensor/test.cpp
        src/Sensor.cpp
        src/PulseSensor.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_PulseSensor PRIVATE tests/fakes src)
target_compile_options(test_PulseSensor PRIVATE -Wall -Wextra -Werror)

add_executable(test_Hd44780Driver
        tests/Hd44780Driver/test.cpp
        src/Hd44780Driver.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_Hd44780Driver PRIVATE tests/fakes src)
target_compile_options(test_Hd44780Driver PRIVATE -Wall -Wextra -Werror)

add_executable(test_Ssd1306Driver
        tests/Ssd1306Driver/test.cpp
        src/Ssd1306Driver.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_Ssd1306Driver PRIVATE tests/fakes src)
target_compile_options(test_Ssd1306Driver PRIVATE -Wall -Wextra -Werror)

add_executable(test_WiFiConnectivityDriver
        tests/WiFiConnectivityDriver/test.cpp
        src/WiFiConnectivityDriver.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_WiFiConnectivityDriver PRIVATE tests/fakes src)
target_compile_options(test_WiFiConnectivityDriver PRIVATE -Wall -Wextra -Werror)

add_executable(test_BmeEnvironmentalSensor
        tests/BmeEnvironmentalSensor/test.cpp
        src/Sensor.cpp
        src/BmeEnvironmentalSensor.cpp
        src/PthSensor.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_BmeEnvironmentalSensor PRIVATE tests/fakes src)
target_compile_options(test_BmeEnvironmentalSensor PRIVATE -Wall -Wextra -Werror)

add_executable(test_GeolocationSensor
        tests/GeolocationSensor/test.cpp
        src/Sensor.cpp
        src/GeolocationSensor.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_GeolocationSensor PRIVATE tests/fakes src)
target_compile_options(test_GeolocationSensor PRIVATE -Wall -Wextra -Werror)

add_executable(test_Device
        tests/Device/test.cpp
        src/Sensor.cpp
        src/Actuator.cpp
        src/Device.cpp
        src/SamplingScheduler.cpp
        src/JsonTelemetryWriter.cpp
        src/JsonParser.cpp
        src/DiagnosticsTelemetryPackage.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_Device PRIVATE tests/fakes src)
target_compile_options(test_Device PRIVATE -Wall -Wextra -Werror)

add_executable(test_UltrasonicSensor
        tests/UltrasonicSensor/test.cpp
        src/Sensor.cpp
        src/UltrasonicSensor.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_UltrasonicSensor PRIVATE tests/fakes src)
target_compile_options(test_UltrasonicSensor PRIVATE -Wall -Wextra -Werror)

add_executable(test_LoadCellAmplifier
        tests/LoadCellAmplifier/test.cpp
        src/Sensor.cpp
        src/LoadCellAmplifier.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_LoadCellAmplifier PRIVATE tests/fakes src)
target_compile_options(test_LoadCellAmplifier PRIVATE -Wall -Wextra -Werror)

add_executable(test_EventAggregator
        tests/EventAggregator/test.cpp
        src/EventAggregator.cpp
        src/TimeSynchronizationDriver.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_EventAggregator PRIVATE tests/fakes src)
target_compile_options(test_EventAggregator PRIVATE -Wall -Wextra -Werror)

add_executable(test_RulesEngine
        tests/RulesEngine/test.cpp
        src/RulesEngine.cpp
        ${TEST_FAKES_SRC}
)
target_include_directories(test_RulesEngine PRIVATE tests/fakes src)
target_compile_options(test_RulesEngine PRIVATE -Wall -Wextra -Werror)

add_executable(test_ChangeFilter
        tests/ChangeFilter/test.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_ChangeFilter PRIVATE tests/fakes src)
target_compile_options(test_ChangeFilter PRIVATE -Wall -Wextra -Werror)

add_executable(test_AiMicroModelController
        tests/AiMicroModelController/test.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_compile_definitions(test_AiMicroModelController PRIVATE USE_EMLEARN)
target_include_directories(test_AiMicroModelController PRIVATE tests/fakes src)
target_compile_options(test_AiMicroModelController PRIVATE -Wall -Wextra -Werror)

add_executable(test_ImuSensor
        tests/ImuSensor/test.cpp
        src/Sensor.cpp
        src/ImuSensor.cpp
        ${TEST_FAKES}
        ${TEST_FAKES_SRC}
)
target_include_directories(test_ImuSensor PRIVATE tests/fakes src)
target_compile_options(test_ImuSensor PRIVATE -Wall -Wextra -Werror)

add_test(NAME DigitalLightIntensitySensorTest COMMAND test_DigitalLightIntensitySensor)
add_test(NAME RgbLedTest COMMAND test_RgbLed)
add_test(NAME RelayActuatorTest COMMAND test_RelayActuator)
add_test(NAME RfidSensorTest COMMAND test_RfidSensor)
add_test(NAME BleConnectivityDriverTest COMMAND test_BleConnectivityDriver)
add_test(NAME ModestLogTest COMMAND test_ModestLog)
add_test(NAME MqttGatewayClientTest COMMAND test_MqttGatewayClient)
add_test(NAME EngineConfigurationTest COMMAND test_EngineConfiguration)
add_test(NAME DiagnosticsSnapshotTest COMMAND test_DiagnosticsSnapshot)
add_test(NAME SensorFaultTest COMMAND test_SensorFault)
add_test(NAME SamplingSchedulerTest COMMAND test_SamplingScheduler)
add_test(NAME TelemetryPackageTest COMMAND test_TelemetryPackage)
add_test(NAME TriggeringSchedulerTest COMMAND test_TriggeringScheduler)
add_test(NAME CommandMarshalingTest COMMAND test_CommandMarshaling)
add_test(NAME WaterFlowSensorTest COMMAND test_WaterFlowSensor)
add_test(NAME SoilMoistureSensorTest COMMAND test_SoilMoistureSensor)
add_test(NAME PthSensorTest COMMAND test_PthSensor)
add_test(NAME WaterLeakSensorTest COMMAND test_WaterLeakSensor)
add_test(NAME DoorWindowSensorTest COMMAND test_DoorWindowSensor)
add_test(NAME OneWireTemperatureSensorTest COMMAND test_OneWireTemperatureSensor)
add_test(NAME DhtSensorTest COMMAND test_DhtSensor)
add_test(NAME DeepSleepTest COMMAND test_DeepSleep)
add_test(NAME ServoMotorTest COMMAND test_ServoMotor)
add_test(NAME CharacterLcdDisplayTest COMMAND test_CharacterLcdDisplay)
add_test(NAME OledDisplayTest COMMAND test_OledDisplay)
add_test(NAME EnvironmentalGasSensorTest COMMAND test_EnvironmentalGasSensor)
add_test(NAME BleGatewayClientTest COMMAND test_BleGatewayClient)
add_test(NAME HttpGatewayClientTest COMMAND test_HttpGatewayClient)
add_test(NAME BuzzerTest COMMAND test_Buzzer)
add_test(NAME ConfigurationStoreTest COMMAND test_ConfigurationStore)
add_test(NAME TimeSynchronizationDriverTest COMMAND test_TimeSynchronizationDriver)
add_test(NAME GasSensorTest COMMAND test_GasSensor)
add_test(NAME ButtonTest COMMAND test_Button)
add_test(NAME PowerManagerTest COMMAND test_PowerManager)
add_test(NAME TelemetryBufferTest COMMAND test_TelemetryBuffer)
add_test(NAME OtaUpdateHandlerTest COMMAND test_OtaUpdateHandler)
add_test(NAME LedTest COMMAND test_Led)
add_test(NAME PassiveInfraredSensorTest COMMAND test_PassiveInfraredSensor)
add_test(NAME PulseSensorTest COMMAND test_PulseSensor)
add_test(NAME Hd44780DriverTest COMMAND test_Hd44780Driver)
add_test(NAME Ssd1306DriverTest COMMAND test_Ssd1306Driver)
add_test(NAME WiFiConnectivityDriverTest COMMAND test_WiFiConnectivityDriver)
add_test(NAME BmeEnvironmentalSensorTest COMMAND test_BmeEnvironmentalSensor)
add_test(NAME UltrasonicSensorTest COMMAND test_UltrasonicSensor)
add_test(NAME LoadCellAmplifierTest COMMAND test_LoadCellAmplifier)
add_test(NAME DeviceTest COMMAND test_Device)
add_test(NAME GeolocationSensorTest COMMAND test_GeolocationSensor)
add_test(NAME EventAggregatorTest COMMAND test_EventAggregator)
add_test(NAME RulesEngineTest COMMAND test_RulesEngine)
add_test(NAME ChangeFilterTest COMMAND test_ChangeFilter)
add_test(NAME AiMicroModelControllerTest COMMAND test_AiMicroModelController)
add_test(NAME ImuSensorTest COMMAND test_ImuSensor)

add_custom_target(run_tests
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        DEPENDS test_DigitalLightIntensitySensor test_RgbLed test_RelayActuator test_RfidSensor test_BleConnectivityDriver test_ModestLog test_MqttGatewayClient test_EngineConfiguration test_DiagnosticsSnapshot test_SensorFault test_SamplingScheduler test_TelemetryPackage test_TriggeringScheduler test_CommandMarshaling test_WaterFlowSensor test_SoilMoistureSensor test_PthSensor test_WaterLeakSensor test_DoorWindowSensor test_OneWireTemperatureSensor test_DhtSensor test_DeepSleep test_ServoMotor test_CharacterLcdDisplay test_OledDisplay test_EnvironmentalGasSensor test_BleGatewayClient test_HttpGatewayClient test_Buzzer test_ConfigurationStore test_TimeSynchronizationDriver test_GasSensor test_Button test_PowerManager test_TelemetryBuffer test_OtaUpdateHandler test_Led test_PassiveInfraredSensor test_PulseSensor test_Hd44780Driver test_Ssd1306Driver test_WiFiConnectivityDriver test_BmeEnvironmentalSensor test_GeolocationSensor test_UltrasonicSensor test_LoadCellAmplifier test_Device test_EventAggregator test_RulesEngine test_ChangeFilter test_AiMicroModelController test_ImuSensor
        COMMENT "Executing all native host test suites..."
)
