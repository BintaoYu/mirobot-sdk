# Industrial Foundation Architecture

## 定位

Industrial Foundation 提供设备通信抽象和 Motor/Mirobot 公共 SDK。主工程构建共享库并按目标平台 stage
头文件与动态库到 `app/sdk/<platform-tag>/{include,lib}`；`app/` 是独立 consumer，只使用 staged SDK，不直接
链接项目源码。

## 模块

| 目录 | 职责 |
| --- | --- |
| `common/include`、`common/src` | `CommInterface`、`SerialComm`、`TcpComm`、`TcpServerComm`、`UdpComm`、`SharedUdpComm`、`DeviceSession`，以及 I/O Adapter SDK（`io_adapter.h`） |
| `libs/motor` | Motor 公共库 |
| `libs/mirobot` | Mirobot 公共库 |
| `tests` | 主工程测试；`steel_test` 是显式 `hardware` 标签的现场入口 |
| `app` | staged SDK 的独立消费：示例、smoke test、`support/` 测试辅助 |
| `cmake`、`scripts` | 平台标签、交叉编译 toolchain 与 MSVC 交叉工具链准备 |

## 消费方与边界

- **Industrial-Apps** 是唯一消费方，且是可选的构建期依赖：只有编译的 Algorithm 调用 `Motor`/`Mirobot` 时，
  Apps 才解析 staged SDK（`INDUSTRIAL_ENABLE_STEEL`、`INDUSTRIAL_STEEL_SDK_ROOT`，`build.py` 的
  `probe_steel_sdk` 默认取同级 `Industrial-Foundation/app/sdk/linux-x86_64`）。因此 `app/sdk/<platform-tag>/{include,lib}`
  的布局是对 Apps 的合同。
- **Virtualized-Softbus 不链接、不消费本项目。** Softbus 的 `src/runtime/io/` 独立实现 RS485/TCP adapter，
  只把 `IoRequestState` 的八个数值（`Unknown`、`Accepted`、`Completed`、`Timeout`、`DeadlineMissed`、
  `TransportError`、`AdapterUnavailable`、`InvalidRequest`）镜像为 wire ABI（见
  `Virtualized-Softbus/src/runtime/io/public/softbus/io/io_types.h` 的注释），以保持独立可构建。改动本项目的
  `IoRequestState` 数值时必须同步该文件。
- Foundation 不负责 Apps lifecycle、Softbus 容器或共享内存通信；consumer 只能依赖公开头文件和 staged library。
- 固定现场 IP/串口/电机命令只属于 `hardware` 测试或 commissioning，不进入默认无设备回归。
- SDK ABI、timeout、错误结果与 cancellation 变化必须同时验证主工程和独立 `app` consumer。

## I/O Adapter SDK

`common/include/io_adapter.h` 提供 `IoAdapterClient`：`io_request_async(adapter_key, payload, deadline_ns)`
生成稳定的 `uint64_t` `IoRequestId`，completion callback 先交付 `Accepted`，再交付一次 terminal `IoResponse`；
`io_request_sync(...)` 复用同一提交路径并以 bounded wait 返回 terminal state。`deadline_ns` 是 monotonic
绝对时刻，`0` 表示不设置 adapter deadline；SDK 只做 deadline 协调，不提供物理设备时限保证。

`IoPayload` 默认做一次显式 copy，也可通过 `IoPayload::from_shared()` 复用已有 byte storage。
`CommInterfaceIoAdapter`、`TcpClientIoAdapter` 和 `Rs485IoAdapter` 复用现有 `CommInterface`/`TcpComm`/
`SerialComm`，通过专用 worker callback 实现 async，不改变 `DeviceSession`、Motor 和 Mirobot 的同步注入路径。
