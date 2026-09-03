# Industrial Foundation

Industrial Foundation（原 Steel）是一个基于 CMake 的 C++ 底层动态链接库项目。仓库会构建三个共享库：

- `common`：通信抽象和传输实现，包括 `DeviceSession`、`SerialComm`、`TcpComm`、`TcpServerComm`、`UdpComm`
- `motor`：电机控制库
- `mirobot`：Mirobot 控制库

主工程构建完成后，会把动态库和公开头文件按目标平台 stage 到 `app/sdk/<platform-tag>/`。`app/` 是独立的 SDK 消费工程，只依赖 stage 出来的头文件和库，不直接引用主工程源码。

## 2026-09-03 integration window

Foundation 是统一 I/O Adapter 链路的设备 SDK authority：Softbus Runtime 通过公开
`IoAdapterClient`/backend contract 到达 `Rs485IoAdapter` 或 `TcpClientIoAdapter`，不会复制
设备通信实现。`IoRequestState`、`adapter_key`、`IoRequestId`、`deadline_ns` 和 accepted/terminal
语义已与 Softbus Runtime 对齐；`deadline_ns` 是 monotonic absolute deadline，`0` 表示不设置
adapter deadline，物理设备服务时间不作 guarantee claim。

Window H 的最终集成使用本项目重新 stage 的 SDK，并通过独立 consumer 和 Apps
benchmark-s0 Build 验证；Foundation 不拥有 Softbus target installer 或 Performance Run authority。
最终 S0 的 18/18 target cells 因而消费同一 staged SDK identity，但本项目仍未连接现场设备、
未执行硬件 commissioning，也不把 loopback transport smoke 写成设备时限保证。

项目文档入口：[架构](docs/architecture.md)、[计划](docs/plan.md)、[状态](docs/status.md)、[agent 规则](AGENTS.md)。

## 目录结构

- `common/include/`：公开通信接口和会话头文件
- `common/src/`：通信层实现
- `libs/motor/`：`motor` 动态库源码和公开头文件
- `libs/mirobot/`：`mirobot` 动态库源码和公开头文件
- `tests/`：主工程测试，其中 `steel_test` 是固定现场参数的真实硬件入口
- `app/`：独立 SDK 消费工程
- `app/example/`：外部项目使用模板
- `app/test/`：SDK smoke test
- `cmake/`：平台标签和交叉编译 toolchain
- `scripts/`：辅助脚本，例如 Linux host 上准备 Windows MSVC ABI 交叉工具链

## 依赖

基础构建依赖：

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build
```

Linux ARM64 交叉编译：

```bash
sudo apt-get install -y g++-aarch64-linux-gnu
```

Windows x64 MinGW 交叉编译：

```bash
sudo apt-get install -y g++-mingw-w64-x86-64-posix binutils-mingw-w64-x86-64
```

Linux host 上交叉编译 Windows x64 MSVC ABI：

```bash
sudo apt-get install -y curl xz-utils
```

MSVC ABI 交叉编译还需要本地 LLVM 和 xwin sysroot。可以用仓库脚本准备：

```bash
./scripts/setup_windows_msvc_cross_toolchain.sh
```

## 快速命令

优先使用仓库根目录的 `./steel`。所有新构建目录都统一放在 `.build/` 下。

```bash
./steel native
./steel arm64
./steel win
./steel msvc-cross
./steel motor-test
./steel clean
./steel clean-legacy
```

命令含义：

- `./steel native`：构建主工程，运行主工程 CTest，构建 `app`，运行 `app` smoke test
- `./steel arm64`：构建 Linux ARM64 主工程和 `app`
- `./steel win`：用 MinGW 构建 Windows x64 主工程和 `app`
- `./steel msvc-cross`：用 Linux host 上的 `clang-cl + lld-link + xwin` 构建 Windows x64 MSVC ABI 主工程和 `app`
- `./steel motor-test`：构建并运行固定现场参数的 `steel_test`
- `./steel clean`：删除 `.build/`
- `./steel clean-legacy`：删除历史遗留的 `build*`、`app/build*` 目录

`./steel native` 会运行注册到 CTest 的主工程测试；当前 `steel_test` 是硬件测试，依赖默认设备可达。如果只想跑非硬件测试，可以手动执行：

```bash
cmake --preset native
cmake --build --preset native
ctest --test-dir .build/native --output-on-failure -LE hardware
```

## 产物位置

### SDK stage 产物

主工程构建后会把给外部项目消费的 SDK stage 到 `app/sdk/<platform-tag>/`：

- `app/sdk/<platform-tag>/include/*.h`
- `app/sdk/<platform-tag>/lib/*`

常见平台标签：

- `linux-x86_64`
- `linux-aarch64`
- `windows-x86_64-mingw`
- `windows-x86_64-msvc`

不同平台的动态库和 import library 命名不同：

- Linux：`libcommon.so`、`libmotor.so`、`libmirobot.so`
- Windows MinGW：`libcommon.dll` + `libcommon.dll.a`，`libmotor.dll` + `libmotor.dll.a`，`libmirobot.dll` + `libmirobot.dll.a`
- Windows MSVC ABI：`common.dll` + `common.lib`，`motor.dll` + `motor.lib`，`mirobot.dll` + `mirobot.lib`

注意：`app/sdk/<platform-tag>/` 是 SDK stage 目录，不是 `app` 示例程序的输出目录。

### app 构建产物

`app` 的可执行程序输出在仓库根目录的 `.build/app-<preset>/bin/` 下：

- native：`.build/app-native/bin/`
- Linux ARM64：`.build/app-arm64/bin/`
- Windows x64 MinGW：`.build/app-win-mingw/bin/`
- Windows x64 MSVC ABI cross：`.build/app-win-msvc-cross/bin/`

这些目录里包含 `tcp_comm_example`、`tcp_server_example`、`udp_comm_example`、`motor_example`、`mirobot_example` 和 `steel_sdk_smoke_test`。Windows 构建会带 `.exe` 后缀，并把运行需要的 SDK `.dll` 复制到同一个 `bin/` 目录。

## 主工程构建

推荐用 preset：

```bash
cmake --preset native
cmake --build --preset native
```

等价的手动模式也可以使用，但新的 build tree 应放在 `.build/` 下：

```bash
cmake -S . -B .build/native -G Ninja
cmake --build .build/native
```

## app 独立构建

先完成对应平台的主工程构建，让 SDK stage 到 `app/sdk/<platform-tag>/`。然后进入 `app/`：

```bash
cd app
cmake --preset native
cmake --build --preset native
ctest --preset native --output-on-failure
```

构建完成后，示例程序和 smoke test 在 `../.build/app-native/bin/`。

`app` 默认消费当前目标平台对应的 SDK。也可以手动指定 SDK tag 或 SDK root：

```bash
cd app
cmake -S . -B ../.build/app-native -G Ninja \
  -DSTEEL_APP_SDK_TAG=linux-x86_64

cmake -S . -B ../.build/app-native -G Ninja \
  -DSTEEL_APP_SDK_ROOT=/absolute/path/to/sdk
```

## 交叉编译

最简单的方式是直接用脚本：

```bash
./steel arm64
./steel win
./steel msvc-cross
```

这些命令都会先构建主工程并 stage 对应平台 SDK，然后构建 `app`，验证外部消费工程可以链接这些动态库。

### Linux ARM64

```bash
cmake --preset arm64
cmake --build --preset arm64

cd app
cmake --preset arm64
cmake --build --preset arm64
```

SDK 会 stage 到 `app/sdk/linux-aarch64/`，`app` 可执行程序在 `.build/app-arm64/bin/`。

### Windows x64 MinGW

```bash
cmake --preset win-mingw
cmake --build --preset win-mingw

cd app
cmake --preset win-mingw
cmake --build --preset win-mingw
```

SDK 会 stage 到 `app/sdk/windows-x86_64-mingw/`，`app` 可执行程序在 `.build/app-win-mingw/bin/`。

### Windows x64 MSVC ABI from Linux

先准备工具链：

```bash
./scripts/setup_windows_msvc_cross_toolchain.sh
```

然后构建：

```bash
cmake --preset win-msvc-cross
cmake --build --preset win-msvc-cross

cd app
cmake --preset win-msvc-cross
cmake --build --preset win-msvc-cross
```

SDK 会 stage 到 `app/sdk/windows-x86_64-msvc/`，`app` 可执行程序在 `.build/app-win-msvc-cross/bin/`。这条路径使用 Linux host 上的 `clang-cl`、`lld-link`、`llvm-lib` 和 xwin sysroot，输出 MSVC ABI 兼容的 Windows x64 `.dll` 和 `.lib`。

交叉编译出的 Windows 程序和 ARM64 程序在当前 Linux x86_64 host 上只验证配置、编译、链接和 SDK stage，不在本机直接运行。

### Windows host 上的 MSVC

如果在 Windows 机器上直接用 Visual Studio/MSVC，先打开 x64 Developer Command Prompt，或先执行 `vcvars64.bat`。可以使用 toolchain：

```bash
cmake -S . -B .build/win-msvc -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/msvc-cl-x64-toolchain.cmake
cmake --build .build/win-msvc
```

也可以直接使用 Visual Studio 生成器。关键是主工程和 `app` 使用同一套 MSVC 环境，并保持 SDK tag 为 `windows-x86_64-msvc`。

## Example 模板

`app/example/` 里有五个模板：

- `tcp_comm_example.cpp`：`TcpComm` 连接、发送和接收
- `tcp_server_example.cpp`：`TcpServerComm` 监听、接受客户端和收发消息
- `udp_comm_example.cpp`：`UdpComm` datagram 发送和响应读取
- `motor_example.cpp`：`Motor(ip, port, device_port)` 通过共享 UDP 控制设备
- `mirobot_example.cpp`：`Mirobot(ip, port, device_port)` 通过共享 UDP 控制设备

使用方式：

1. 打开对应示例文件
2. 修改 IP、端口、监听地址、串口路径、设备号或命令
3. 如果文件里有 `kEnable...Example` 开关，改成 `true`
4. 重新构建并运行 `.build/app-<preset>/bin/` 下的目标程序，例如 `.build/app-native/bin/motor_example` 或 `.build/app-win-mingw/bin/motor_example.exe`

## API 使用要点

`TcpComm` 和 `UdpComm` 都实现 `CommInterface`：

```cpp
UdpComm comm;
comm.connect("192.168.31.120", 8000);
comm.writeData("@10 G6 F300");
```

`UdpComm` 使用 connected UDP socket，会自动补 `\n`。如果需要读取控制器回包，必须复用同一个 `UdpComm` 实例，因为控制器会把返回数据发给发起查询的源 IP 和端口。

`Motor` 和 `Mirobot` 支持直接通过 UDP endpoint 构造：

```cpp
Motor motor("192.168.31.120", 8000, 10);
Mirobot mirobot("192.168.31.120", 8000, 2);
```

这条路径会在进程内共享同一个底层 `UdpComm`。如果已有共享 UDP 通道绑定到一个目标，又尝试绑定到另一个目标，构造函数会抛出 `std::runtime_error`，避免静默切换控制器。

也可以注入任意 `CommInterface`：

```cpp
TcpComm tcp;
tcp.connect("192.168.31.120", 8000);

DeviceSession session(tcp, 10, " ");
Motor motor(session);
```

`DeviceSession` 负责设备号前缀、原始发送和同步等待，默认同步等待超时为 1000ms。`Motor::set_speed()`、`Mirobot::set_speed()` 这类同步接口在超时后会抛出 `std::runtime_error`。

I/O Adapter SDK 通过 `IoAdapterClient` 提供 event-driven request/response：

```cpp
IoAdapterClient client;
client.set_default_completion_handler([](IoResponse response) {
    if (response.terminal()) {
        // response.request_id、response.state 和 response.error 是最终事实。
    }
});

const IoRequestResult accepted = client.io_request_async(
    adapter_key, payload, deadline_ns);
IoResponse completed = client.io_request_sync(adapter_key, payload, deadline_ns);
```

async callback 先收到 `Accepted`，随后收到一次 terminal response。terminal state 明确区分
`Completed`、`Timeout`、`DeadlineMissed`、`TransportError`、`AdapterUnavailable` 和
`InvalidRequest`。`deadline_ns` 是 monotonic absolute deadline，只用于 bounded
coordination，不构成 guaranteed deadline。使用共享 byte storage 时可用
`IoPayload::from_shared()` 避免 SDK 边界的额外 payload copy；普通 `IoPayloadView` 会执行
一次显式 SDK-owned copy。

`CommInterfaceIoAdapter`、`TcpClientIoAdapter` 和 `Rs485IoAdapter` 复用既有通信 primitive，
而 `DeviceSession`、Motor 和 Mirobot 的 legacy synchronous/injected APIs 保持不变。

## 测试

非硬件测试：

```bash
cmake --preset native
cmake --build --preset native
ctest --test-dir .build/native --output-on-failure -LE hardware
```

`app` smoke test：

```bash
cd app
cmake --preset native
cmake --build --preset native
ctest --preset native --output-on-failure
```

真实电机硬件入口：

```bash
./steel motor-test
```

`tests/main_test.cpp` 当前固定参数：

- IP：`192.168.31.120`
- 端口：`8000`
- `device_port`：`10`
- 运行速度：`300`
- 超时：`1000ms`

这个入口会直接构造 `Motor(ip, port, device_port)` 并执行 `set_speed(0)`、`set_speed(300)`、`set_speed(0)`，适合现场固定设备验收。

## 清理

```bash
./steel clean
./steel clean-legacy
```

`./steel clean` 删除统一的 `.build/`。`./steel clean-legacy` 删除旧的 `build*`、`app/build*` 目录。

## 备注

- Linux 串口常见路径为 `/dev/ttyUSB0` 或 `/dev/ttyACM0`
- Windows `COM10` 及以上通常写成 `\\\\.\\COM10`
- Windows SDK stage 目录按工具链区分，MinGW 和 MSVC ABI 不会互相覆盖
- `TcpComm` 是普通连接对象，不使用全局单例
- `TcpServerComm` 当前是单监听 socket + 单客户端连接模型；如需切换客户端，先调用 `disconnectClient()`
