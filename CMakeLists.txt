cmake_minimum_required(VERSION 3.15...4.3)

# 1. Project Configuration & Metadata
project(Modest-IoT-Nano-Framework VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. Hardware and Environment Specifications
set(BOARD_FQBN "esp32:esp32:esp32da" CACHE STRING "Target hardware device FQBN selection identifier")
set(PORT "/dev/ttyUSB0" CACHE STRING "Serial flash port for ESP32 layout deployment")
set(ARDUINO_CLI_PATH "/opt/homebrew/bin/arduino-cli" CACHE FILEPATH "Local path to your executable binary")
set(ARDUINO_LIBS "/Users/angelvelasquez/Documents/Arduino/libraries" CACHE FILEPATH "Path to Arduino libraries")
set(ARDUINO_CORE_PATH "/Users/angelvelasquez/Library/Arduino15/packages/esp32/hardware/esp32/3.3.10/cores/esp32" CACHE FILEPATH "Path to Arduino ESP32 core")
set(ARDUINO_ESP32_LIBS_INCLUDE_PATH "/Users/angelvelasquez/Library/Arduino15/packages/esp32/tools/esp32-libs/3.3.10/include" CACHE FILEPATH "Path to Arduino ESP32 tools include")
set(SKETCH_DIR "${CMAKE_CURRENT_SOURCE_DIR}") # Assumes main.cpp/main.ino acts as sketch root

# 3. Native Source Target (Keeps CLion indexing perfect)
file(GLOB_RECURSE LIB_SOURCES "src/*.cpp")
file(GLOB_RECURSE LIB_HEADERS "src/*.h")

add_library(${PROJECT_NAME} STATIC ${LIB_SOURCES} ${LIB_HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        "${ARDUINO_LIBS}/ArduinoJson/src"
        "${ARDUINO_LIBS}/PubSubClient/src"
        "${ARDUINO_LIBS}/LiquidCrystal_I2C"
        "${ARDUINO_CORE_PATH}"
        "${ARDUINO_ESP32_LIBS_INCLUDE_PATH}"
)

# 4. IDE Code-Completion Indexer Targets
# Added main.cpp to allow the IDE to discover full symbols across your static code base
add_executable(clion_indexer main.cpp)
target_link_libraries(clion_indexer PRIVATE ${PROJECT_NAME})

# Add Arduino libraries for indexing
target_include_directories(clion_indexer PRIVATE
    "${ARDUINO_LIBS}/EspSoftwareSerial/src"
    "${ARDUINO_LIBS}/LiquidCrystal_I2C"
    "${ARDUINO_LIBS}/DHT_sensor_library"
    "${ARDUINO_LIBS}/ArduinoJson/src"
    "${ARDUINO_LIBS}/PubSubClient/src"
    "${ARDUINO_CORE_PATH}"
    "${ARDUINO_ESP32_LIBS_INCLUDE_PATH}"
)

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
