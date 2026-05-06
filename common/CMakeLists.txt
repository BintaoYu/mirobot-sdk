add_library(common SHARED
    src/device_session.cpp
    src/serial_comm.cpp
    src/shared_udp_comm.cpp
    src/tcp_server_comm.cpp
    src/tcp_comm.cpp
    src/udp_comm.cpp
)

target_include_directories(common PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(common PUBLIC cxx_std_17)

if (MSVC)
    target_compile_options(common PRIVATE /W4)
else()
    target_compile_options(common PRIVATE -Wall -Wextra -Wpedantic)
endif()

if (WIN32)
    target_compile_definitions(common PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    target_link_libraries(common PUBLIC ws2_32)
endif()
