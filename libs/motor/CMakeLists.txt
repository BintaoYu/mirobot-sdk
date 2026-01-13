# CMake configuration for motor library

add_library(motor SHARED
    motor.cpp
)

target_include_directories(motor PUBLIC
    ${CMAKE_SOURCE_DIR}/common/include
)
