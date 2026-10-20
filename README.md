# Mirobot SDK

Mirobot SDK 是 C++17 设备通信库，提供通用通信会话以及 Motor / Mirobot 控制接口。
主工程生成三个动态库：`common`、`motor`、`mirobot`，再把头文件和库整理到 `app/sdk/<platform-tag>/`，供独立应用使用。

本仓库可独立构建，也可作为 Industrial Platform 的 `mirobot-sdk/` 子模块使用。
历史名称 Industrial Foundation / Steel 仍出现在 `steel` 脚本、CMake 配置及 SDK 标识中。

## 获取与依赖

```bash
git clone https://github.com/BintaoYu/mirobot-sdk.git
cd mirobot-sdk
```

Linux 本机构建需要 C++17 编译器、CMake 和 Ninja。Debian / Ubuntu 可安装：

```bash
sudo apt-get install build-essential cmake ninja-build
```

以下 preset 命令要求 CMake ≥ 3.22。其他平台工具链见[构建指南](docs/building.md#依赖)。

## 首次构建：无设备路径

在仓库根目录执行：

```bash
cmake --preset native
cmake --build --preset native
ctest --test-dir .build/native --output-on-failure -LE hardware
```

主工程会自动 stage 当前平台 SDK。然后验证独立的消费工程：

```bash
cd app
cmake --preset native
cmake --build --preset native
ctest --preset native --output-on-failure
```

SDK 位于 `app/sdk/linux-x86_64/`，本机示例程序位于 `.build/app-native/bin/`。
其他平台的目录和动态库名称见[产物说明](docs/building.md#产物位置)。

## 快速命令

`./steel` 封装主工程与独立 app 的构建：

| 命令 | 行为 |
| --- | --- |
| `./steel native` | 本机构建、主工程全部 CTest、app 构建与 smoke |
| `./steel arm64` | Linux ARM64 交叉编译 |
| `./steel win` | Windows x64 MinGW 交叉编译 |
| `./steel msvc-cross` | Linux 上构建 Windows x64 MSVC ABI |
| `./steel motor-test` | 构建并运行真实电机测试 |
| `./steel clean` | 删除 `.build/` |

**`./steel native` 包含硬件测试。** 没有连接设备时，使用上面的 `ctest -LE hardware` 流程。
`motor-test` 会按源码中的固定设备参数实际发送速度命令，使用前查看[硬件测试参数](docs/building.md#测试)。

## 在应用中使用

独立消费示例在 `app/example/`，只使用 staged 头文件和动态库。可按传输方式注入会话：

```cpp
#include "tcp_comm.h"
#include "device_session.h"
#include "motor.h"

TcpComm tcp;
tcp.connect("192.168.31.120", 8000);
DeviceSession session(tcp, 10, " ");
Motor motor(session);
```

IP、端口和设备号应改为实际配置。同步调用有超时，失败会抛出异常；UDP 回包需要复用发起请求的同一通信实例。
完整示例、共享 UDP 限制和 `IoAdapterClient` 的异步结果语义见[API 与示例](docs/sdk-api.md)。

## 文档与目录

| 入口 | 内容 |
| --- | --- |
| [构建与测试](docs/building.md) | 依赖、preset、交叉编译、SDK 产物和硬件测试 |
| [API 与示例](docs/sdk-api.md) | TCP / UDP、会话注入、Motor / Mirobot、I/O Adapter |
| [架构](docs/architecture.md) | 模块、SDK 消费方式和平台边界 |
| [状态](docs/status.md)、[计划](docs/plan.md) | 最近验证、适用范围和剩余工作 |
| `common/` | 通信抽象、传输、DeviceSession 与 I/O Adapter |
| `libs/` | Motor / Mirobot 实现 |
| `app/` | 独立 SDK 消费工程与示例 |
| `tests/`、`cmake/`、`scripts/` | 回归、工具链和辅助脚本 |
