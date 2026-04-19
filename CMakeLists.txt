cmake_minimum_required(VERSION 3.16)
project(Steel LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 【新增】强制全项目默认生成动态库 (.so / .dll)
set(BUILD_SHARED_LIBS ON)

if (WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

include(CTest)

# Add subdirectories
add_subdirectory(common)
add_subdirectory(libs)
if (BUILD_TESTING)
    add_subdirectory(tests)
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/common/include
    ${CMAKE_SOURCE_DIR}/libs/mirobot
    ${CMAKE_SOURCE_DIR}/libs/motor
)

# Platform-specific sources
set(PLATFORM_SRC ${CMAKE_SOURCE_DIR}/common/src/serial_comm.cpp)

target_include_directories(motor PRIVATE ${CMAKE_SOURCE_DIR}/common/include)
target_include_directories(mirobot PRIVATE ${CMAKE_SOURCE_DIR}/common/include)
