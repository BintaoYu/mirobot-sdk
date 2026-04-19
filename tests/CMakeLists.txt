function(add_steel_test test_name)
    add_executable(${test_name} ${ARGN})
    target_include_directories(${test_name} PRIVATE
        ${CMAKE_SOURCE_DIR}/common/include
        ${CMAKE_SOURCE_DIR}/libs/mirobot
        ${CMAKE_SOURCE_DIR}/libs/motor
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

add_steel_test(motor_test motor_test.cpp)
target_link_libraries(motor_test PRIVATE common motor)

add_steel_test(mirobot_test mirobot_test.cpp)
target_link_libraries(mirobot_test PRIVATE common mirobot)

add_steel_test(tcp_comm_test tcp_comm_test.cpp)
target_link_libraries(tcp_comm_test PRIVATE common)
