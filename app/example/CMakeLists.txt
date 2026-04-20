function(add_steel_example target_name source_file)
    add_executable(${target_name}
        ${source_file}
    )

    configure_sdk_consumer(${target_name})
endfunction()

add_steel_example(tcp_comm_example tcp_comm_example.cpp)
add_steel_example(motor_example motor_example.cpp)
add_steel_example(mirobot_example mirobot_example.cpp)
