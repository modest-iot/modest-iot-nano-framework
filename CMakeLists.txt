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

set(ESP32_PLATFORM_VERSION "3.3.10")
set(ESP32_PLATFORM_BASE "${ARDUINO_DATA_DIR}/packages/esp32/hardware/esp32/${ESP32_PLATFORM_VERSION}")
set(ESP32_TOOLS_BASE "${ARDUINO_DATA_DIR}/packages/esp32/tools/esp32-libs/${ESP32_PLATFORM_VERSION}")
set(ARDUINO_CORE_PATH "${ESP32_PLATFORM_BASE}/cores/esp32" CACHE FILEPATH "Path to Arduino ESP32 core")
set(ARDUINO_ESP32_LIBS_INCLUDE_PATH "${ESP32_TOOLS_BASE}/include" CACHE FILEPATH "Path to Arduino ESP32 tools include")
set(ARDUINO_ESP32_FREERTOS_INCLUDE_PATH "${ESP32_TOOLS_BASE}/include/freertos/FreeRTOS-Kernel/include" CACHE FILEPATH "Path to Arduino ESP32 FreeRTOS include")
set(ARDUINO_ESP32_FREERTOS_PORTABLE_INCLUDE_PATH "${ESP32_TOOLS_BASE}/include/freertos/FreeRTOS-Kernel/portable/xtensa/include" CACHE FILEPATH "Path to Arduino ESP32 FreeRTOS portable include")
set(ARDUINO_ESP32_FREERTOS_CONFIG_INCLUDE_PATH "${ESP32_TOOLS_BASE}/include/freertos/config/include" CACHE FILEPATH "Path to Arduino ESP32 FreeRTOS config include")
set(SKETCH_DIR "${CMAKE_CURRENT_SOURCE_DIR}") # Assumes main.cpp/main.ino acts as sketch root

add_definitions(-DARDUINO)

# 3. Native Source Target (Keeps CLion indexing perfect)
file(GLOB_RECURSE LIB_SOURCES "src/*.cpp")
file(GLOB_RECURSE LIB_HEADERS "src/*.h")

add_library(${PROJECT_NAME} STATIC EXCLUDE_FROM_ALL ${LIB_SOURCES} ${LIB_HEADERS})
target_compile_definitions(${PROJECT_NAME} PUBLIC IRAM_ATTR= __xtensa__=1 __XTENSA__=1)

target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        "${ARDUINO_LIBS}/ArduinoJson/src"
        "${ARDUINO_LIBS}/PubSubClient/src"
        "${ARDUINO_LIBS}/LiquidCrystal_I2C"
        "${ARDUINO_LIBS}/DHT_sensor_library"
        "${ARDUINO_LIBS}/EspSoftwareSerial/src"
        "${ARDUINO_LIBS}/ESP32Servo/src"
        "${ESP32_PLATFORM_BASE}/libraries/BluetoothSerial/src"
        "${ESP32_PLATFORM_BASE}/libraries/HTTPClient/src"
        "${ESP32_PLATFORM_BASE}/libraries/WiFi/src"
        "${ESP32_PLATFORM_BASE}/libraries/Network/src"
        "${ARDUINO_CORE_PATH}"
        "${ARDUINO_ESP32_LIBS_INCLUDE_PATH}"
        "${ARDUINO_ESP32_FREERTOS_INCLUDE_PATH}"
        "${ARDUINO_ESP32_FREERTOS_PORTABLE_INCLUDE_PATH}"
        "${ARDUINO_ESP32_FREERTOS_CONFIG_INCLUDE_PATH}"
        "${ARDUINO_ESP32_FREERTOS_CONFIG_INCLUDE_PATH}/freertos"
        "${ESP32_TOOLS_BASE}/include/freertos/config/xtensa/include"
        "${ESP32_TOOLS_BASE}/include/soc/esp32/include"
        "${ESP32_TOOLS_BASE}/include/esp_common/include"
        "${ESP32_TOOLS_BASE}/include/xtensa/include"
        "${ESP32_TOOLS_BASE}/include/xtensa/esp32/include"
        "${ESP32_TOOLS_BASE}/include/freertos/esp_additions/include"
        "${ESP32_TOOLS_BASE}/qio_qspi/include"
        "${ESP32_PLATFORM_BASE}/variants/esp32da"
        "${ESP32_TOOLS_BASE}/include/newlib/platform_include/"
        "${CMAKE_CURRENT_SOURCE_DIR}/include_indexer"
)

# 4. IDE Code-Completion Indexer Targets
# Added main.cpp to allow the IDE to discover full symbols across your static code base
add_library(clion_indexer INTERFACE)
target_compile_definitions(clion_indexer INTERFACE IRAM_ATTR= __xtensa__=1 __XTENSA__=1)
target_link_libraries(clion_indexer INTERFACE ${PROJECT_NAME})

# 5. Dedicated arduino-cli Target Pipeline
# Compiles your project using the esp32da compiler ecosystem safely isolated from host flags
add_custom_target(arduino_compile
        COMMAND ${ARDUINO_CLI_PATH} compile
        --fqbn ${BOARD_FQBN}
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
