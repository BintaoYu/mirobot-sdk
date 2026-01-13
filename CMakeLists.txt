cmake_minimum_required(VERSION 3.16)
project(Steel)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add subdirectories
add_subdirectory(common)
add_subdirectory(libs)
add_subdirectory(tests)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/common/include
    ${CMAKE_SOURCE_DIR}/libs/mirobot
    ${CMAKE_SOURCE_DIR}/libs/motor
)

# Platform-specific sources
set(PLATFORM_SRC ${CMAKE_SOURCE_DIR}/common/src/serial_comm.cpp)

# Add shared libraries for each folder in libs
add_library(motor SHARED ${CMAKE_SOURCE_DIR}/libs/motor/motor.cpp)
add_library(mirobot SHARED ${CMAKE_SOURCE_DIR}/libs/mirobot/mirobot.cpp)

target_include_directories(motor PRIVATE ${CMAKE_SOURCE_DIR}/common/include)
target_include_directories(mirobot PRIVATE ${CMAKE_SOURCE_DIR}/common/include)