# common/CMakeLists.txt

add_library(common STATIC
    src/serial_comm.cpp
    src/tcp_comm.cpp
)

target_include_directories(common PUBLIC
    ${CMAKE_SOURCE_DIR}/common/include
)

target_compile_features(common PUBLIC cxx_std_20)
target_compile_options(common PRIVATE -Wall -Wextra -Wpedantic)