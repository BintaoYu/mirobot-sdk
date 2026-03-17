# arm64_toolchain.cmake

# 1. 声明目标操作系统的名称（必须设置）
# 如果是带操作系统的（如嵌入式 Linux），写 Linux
# 如果是裸机（Bare-metal）或者某些微内核（如 seL4），通常写 Generic
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# 2. 指定交叉编译器的完整路径或命令名称
# 确保这些交叉编译器所在的路径已经加入了系统环境变量 PATH 中
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)


# 4. 严格控制 find_program, find_library, find_path 的搜索行为
# 查找可执行程序时（比如通过 find_program 找个打包工具），只找 Host（本机）的
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# 查找库文件（.a/.so）、头文件（.h）和包（Package）时，只找 Target（板子）上的
# 这一步极其关键！防止你把本机 x86 的库链接到 ARM 的程序里去，导致编译报错。
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)