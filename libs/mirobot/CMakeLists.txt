# CMake configuration for mirobot library

add_library(mirobot SHARED
    mirobot.cpp
)

target_include_directories(mirobot PUBLIC
    ${CMAKE_SOURCE_DIR}/common/include
)
