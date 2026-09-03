# Industrial Foundation Architecture

## 定位

Industrial Foundation 提供设备通信抽象和 Motor/Mirobot 公共 SDK。主工程构建共享库并按目标平台 stage 头文件与动态库；`app/` 是独立 consumer，只使用 staged SDK，不直接链接项目源码。

## 模块

| 目录 | 职责 |
| --- | --- |
| `common/include`、`common/src` | `DeviceSession`、Serial/TCP/UDP 公共通信抽象、I/O Adapter SDK 与实现 |
| `libs/motor` | Motor 公共库 |
| `libs/mirobot` | Mirobot 公共库 |
| `tests` | 主工程测试与显式 hardware test |
| `app` | staged SDK 的独立消费、示例和 smoke test |
| `cmake`、`scripts` | 平台标签、交叉编译和工具链准备 |

## Authority 边界

- Foundation 负责设备 SDK 的接口、传输和链接产物，不负责 Apps lifecycle、Softbus 容器或共享内存通信。
- Apps 与其他 consumer 只能依赖公开头文件和 staged library。
- 固定现场 IP/串口/电机命令属于 hardware test 或 commissioning，不进入默认无设备回归。
- SDK ABI、timeout、错误结果与 cancellation 变化必须同时验证主工程和独立 `app` consumer。

## 2026-09-03 Softbus integration boundary

The public SDK is the lower-layer I/O contract consumed by the Softbus Runtime. Both layers use the
same eight numeric request states: `Unknown`, `Accepted`, `Completed`, `Timeout`, `DeadlineMissed`,
`TransportError`, `AdapterUnavailable` and `InvalidRequest`. `IoRequestId` is `uint64_t` in the public
API; Softbus may serialize a checked narrower ID only at its existing shared-memory ABI boundary.

`deadline_ns` is an absolute value from the monotonic clock and `0` means no adapter deadline. SDK
submission preserves `adapter_key`, request ID and deadline through accepted then exactly one terminal
completion. The SDK coordinates and reports timeout/deadline outcomes but does not establish a physical
device deadline guarantee or WCET; `deadline_supported`, `deadline_observed` and
`deadline_guaranteed` are evidence facts owned by the Runtime adapter layer.

## I/O Adapter SDK

`common/include/io_adapter.h` 提供独立的 `IoAdapterClient` contract。其
`io_request_async(adapter_key, payload, deadline_ns)` 生成稳定 `IoRequestId`，通过
completion callback 先交付 `Accepted`，再交付一次 terminal `IoResponse`；
`io_request_sync(...)` 复用同一提交路径并以 bounded wait 返回显式 terminal state。
terminal state 包括 `Completed`、`Timeout`、`DeadlineMissed`、`TransportError`、
`AdapterUnavailable` 和 `InvalidRequest`。

`IoPayload` 默认支持一次显式 copy，也可携带 shared owner 复用已有 byte storage。SDK
不会把 `deadline_ns` 转换成保证时限；它只在 monotonic clock 上做 request deadline
协调。`CommInterfaceIoAdapter`、`TcpClientIoAdapter` 和 `Rs485IoAdapter` 复用现有
`CommInterface`/`TcpComm`/`SerialComm`，通过专用 worker callback 实现 async，不改变
`DeviceSession` 或 Mirobot 的同步注入路径。
