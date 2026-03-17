# 添加测试可执行文件
add_executable(steel_test main_test.cpp)

# 链接你的动态库 (注意：这里的名字必须和 add_library 定义的名字完全一致)
target_link_libraries(steel_test PRIVATE common motor mirobot)

# （可选）如果测试文件需要引用头文件，确保包含路径正确
target_include_directories(steel_test PRIVATE 
    ${CMAKE_SOURCE_DIR}/common/include
    ${CMAKE_SOURCE_DIR}/libs/mirobot
    ${CMAKE_SOURCE_DIR}/libs/motor
)