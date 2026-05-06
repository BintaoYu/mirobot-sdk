add_library(motor SHARED
    motor.cpp
)

target_include_directories(motor PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(motor PUBLIC common)
target_compile_features(motor PUBLIC cxx_std_17)

if (WIN32)
    set_target_properties(motor PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
else()
    set_target_properties(motor PROPERTIES INSTALL_RPATH "$ORIGIN")
endif()
