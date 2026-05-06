add_library(mirobot SHARED
    mirobot.cpp
)

target_include_directories(mirobot PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(mirobot PUBLIC common)
target_compile_features(mirobot PUBLIC cxx_std_17)

if (WIN32)
    set_target_properties(mirobot PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
else()
    set_target_properties(mirobot PROPERTIES INSTALL_RPATH "$ORIGIN")
endif()
