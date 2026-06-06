cmake_minimum_required(VERSION 3.30)
project(Modest-IoT-Nano-Framework VERSION 1.0.0 LANGUAGES CXX C)
set(BOARD_ID, esp32)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ArduinoToolchain.cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE LIB_SOURCES "src/*.cpp")
file(GLOB_RECURSE LIB_HEADERS "src/*.h")

add_library(${PROJECT_NAME} STATIC ${LIB_SOURCES} ${LIB_HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)

if(APPLE)
    set(ARDUINO_CORE_PATH "/Applications/Arduino.app/Contents/Java/hardware/arduino/avr/cores/arduino")
    set(ARDUINO_VARIANT_PATH "/Applications/Arduino.app/Contents/Java/hardware/arduino/avr/variants/standard")
elseif(WIN32)
    set(ARDUINO_CORE_PATH "C:/Program Files (x86)/Arduino/hardware/arduino/avr/cores/arduino")
    set(ARDUINO_VARIANT_PATH "C:/Program Files (x86)/Arduino/hardware/arduino/avr/variants/standard")
else() # Linux
    set(ARDUINO_CORE_PATH "/usr/share/arduino/hardware/arduino/avr/cores/arduino")
    set(ARDUINO_VARIANT_PATH "/usr/share/arduino/hardware/arduino/avr/variants/standard")
endif()

if(EXISTS ${ARDUINO_CORE_PATH})
    target_include_directories(${PROJECT_NAME} PRIVATE ${ARDUINO_CORE_PATH} ${ARDUINO_VARIANT_PATH})
endif()

add_executable(clion_indexer main.cpp)

target_link_libraries(clion_indexer PRIVATE ${PROJECT_NAME})