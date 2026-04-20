add_executable(steel_sdk_smoke_test
    sdk_smoke_test.cpp
)

configure_sdk_consumer(steel_sdk_smoke_test)
add_test(NAME steel_sdk_smoke_test COMMAND steel_sdk_smoke_test)
