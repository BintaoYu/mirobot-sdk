# CMake configuration for tests

add_executable(test_comm
    test_comm.cpp
)

target_include_directories(test_comm PRIVATE
    ${CMAKE_SOURCE_DIR}/common/include
    ${CMAKE_SOURCE_DIR}/libs/mirobot
    ${CMAKE_SOURCE_DIR}/libs/motor
)

target_link_libraries(test_comm PRIVATE
    common
    motor
    mirobot
)
