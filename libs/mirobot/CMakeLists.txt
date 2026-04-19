# CMake configuration for mirobot library

add_library(mirobot SHARED
    mirobot.cpp
)

target_include_directories(mirobot PUBLIC
    ${CMAKE_SOURCE_DIR}/common/include
)

if (WIN32)
    set_target_properties(mirobot PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()
