# Steel

这是一个基于 CMake 的 C++ 共享库项目，包含通信层、设备会话层、`motor`、`mirobot`，以及一个独立的 `app` 消费工程。

## 主要目录

- `common/`：通信实现和 `DeviceSession` 会话抽象
- `libs/`：业务库源码
- `tests/`：主工程测试
- `app/`：独立消费工程
- `app/sdk/<platform-tag>/include/`：主工程构建后按平台分目录 stage 的头文件
- `app/sdk/<platform-tag>/lib/`：主工程构建后按平台分目录 stage 的共享库
- `app/example/`：外部使用模板
- `app/test/`：`app` 工程自己的 smoke test

## 依赖安装

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build
```

## 主工程构建

在仓库根目录执行：

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

构建完成后会自动复制：

- `app/sdk/<platform-tag>/include/*.h`
- `app/sdk/<platform-tag>/lib/*`

平台标签默认由目标系统和架构组成，例如：

- `linux-x86_64`
- `linux-aarch64`
- `windows-x86_64-mingw`
- `windows-x86_64-msvc`

## app 独立构建

先完成一次主工程构建，再进入 `app/`：

```bash
cd app
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

`app` 不引用 `common/` 或 `libs/` 里的源码，只使用：

- `app/sdk/<platform-tag>/include`
- `app/sdk/<platform-tag>/lib`

默认情况下，`app` 会自动选择“当前目标平台”对应的 staged SDK。也可以手动指定：

```bash
cd app
cmake -S . -B build -DSTEEL_APP_SDK_TAG=linux-aarch64
```

## Example 模板

`app/example/` 保留一套跨平台源码，通过不同 build 目录和不同 staged SDK 为不同平台分别编译。当前提供四个真实接入模板：

- `tcp_comm_example.cpp`：`TcpComm` 的连接、发命令、收响应模板
- `tcp_server_example.cpp`：`TcpServerComm` 的监听、接受客户端、收发消息模板
- `motor_example.cpp`：`Motor` 通过 `DeviceSession + TcpComm` 控制设备的模板
- `mirobot_example.cpp`：`Mirobot` 通过 `DeviceSession + SerialComm` 控制设备的模板

使用方式：

1. 打开对应示例文件
2. 修改里面的 IP、端口、监听地址、串口、设备号等参数
3. 如果示例文件里有 `kEnable...Example` 开关，按需要改成 `true`
4. 重新编译后运行对应程序

示例目标如下：

- `app/build/bin/tcp_comm_example`
- `app/build/bin/tcp_server_example`
- `app/build/bin/motor_example`
- `app/build/bin/mirobot_example`

## 交叉编译

Linux ARM64：

```bash
cmake -S . -B build-arm64 -DCMAKE_TOOLCHAIN_FILE=cmake/arm64_toolchain.cmake
cmake --build build-arm64
```

如需构建 `app` 的 ARM64 example，也使用同一个 toolchain，并让它消费 `app/sdk/linux-aarch64`：

```bash
cd app
cmake -S . -B build-arm64 -DCMAKE_TOOLCHAIN_FILE=../cmake/arm64_toolchain.cmake
cmake --build build-arm64
```

Windows x64：

```bash
cmake -S . -B build-win-cross -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64_toolchain.cmake
cmake --build build-win-cross
```

如需构建 Windows x64 的 `app` example：

```bash
cd app
cmake -S . -B build-win-cross -DCMAKE_TOOLCHAIN_FILE=../cmake/mingw-w64-x86_64_toolchain.cmake
cmake --build build-win-cross
```

Windows x64 with MSVC `cl.exe`：

先打开 Visual Studio 的 x64 Developer Command Prompt，或者先执行 `vcvars64.bat`，再配置：

```bash
cmake -S . -B build-msvc -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/msvc-cl-x64-toolchain.cmake
cmake --build build-msvc
```

如需构建 MSVC 的 `app` example：

```bash
cd app
cmake -S . -B build-msvc -G Ninja -DCMAKE_TOOLCHAIN_FILE=../cmake/msvc-cl-x64-toolchain.cmake
cmake --build build-msvc
```

如果你直接用 Visual Studio 生成器，也可以不用 toolchain 文件，只要确保主工程和 `app` 用同一套 MSVC 环境，并把 `STEEL_APP_SDK_TAG` 保持为 `windows-x86_64-msvc`。

Linux host cross-compiling to Windows x64 with MSVC ABI:

先在 Linux 上准备本地 LLVM + xwin 工具链：

```bash
./scripts/setup_windows_msvc_cross_toolchain.sh
```

然后用 `clang-cl` + `lld-link` 交叉编译主工程：

```bash
cmake -S . -B build-msvc-cross -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/clang-cl-msvc-x64-linux-toolchain.cmake
cmake --build build-msvc-cross
```

如需构建 `app` 的 Windows x64 MSVC ABI example：

```bash
cd app
cmake -S . -B build-msvc-cross -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=../cmake/clang-cl-msvc-x64-linux-toolchain.cmake
cmake --build build-msvc-cross
```

这套流程会把 Windows MSVC ABI 的 SDK stage 到 `app/sdk/windows-x86_64-msvc`。

## 说明

- Linux 串口常见为 `/dev/ttyUSB0` 或 `/dev/ttyACM0`
- Windows `COM10` 及以上通常要写成 `\\\\.\\COM10`
- Windows SDK stage 目录现在会按工具链分开，MinGW 和 MSVC 不会互相覆盖
- `TcpComm` 现在是普通连接对象，不再使用全局单例
- `TcpServerComm` 当前是单监听 socket + 单客户端连接模型；如需切换客户端，先调用 `disconnectClient()`
- `DeviceSession` 负责设备号前缀、原始发送和同步等待，默认同步等待超时为 1000ms
- `Motor::set_speed()`、`Mirobot::set_speed()` 这类同步接口在超时后会抛出 `std::runtime_error`
