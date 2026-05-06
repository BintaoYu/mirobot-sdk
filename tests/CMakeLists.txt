function(add_steel_test test_name)
    add_executable(${test_name} ${ARGN})
    target_include_directories(${test_name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
    target_compile_features(${test_name} PRIVATE cxx_std_17)
    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

add_steel_test(motor_test motor_test.cpp)
target_link_libraries(motor_test PRIVATE motor)

add_steel_test(mirobot_test mirobot_test.cpp)
target_link_libraries(mirobot_test PRIVATE mirobot)

add_steel_test(device_session_test device_session_test.cpp)
target_link_libraries(device_session_test PRIVATE common)

add_steel_test(tcp_comm_test tcp_comm_test.cpp)
target_link_libraries(tcp_comm_test PRIVATE common)

add_steel_test(tcp_server_comm_test tcp_server_comm_test.cpp)
target_link_libraries(tcp_server_comm_test PRIVATE common)

add_executable(steel_test main_test.cpp)
target_include_directories(steel_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_features(steel_test PRIVATE cxx_std_17)
target_link_libraries(steel_test PRIVATE common motor mirobot)
