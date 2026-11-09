# Mirobot SDK 架构

## 定位

Mirobot SDK 提供设备通信抽象和 Motor / Mirobot 公共 SDK，保留既有 Steel / Foundation 构建标识。主工程构建共享库并按目标平台 stage
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

- Apps 是可选的构建期消费方：编译的 Algorithm 调用 `Motor`/`Mirobot` 时，消费方解析对应平台的 staged SDK。
  `app/sdk/<platform-tag>/{include,lib}` 是公开分发布局，独立应用也按同一布局消费；消费方自身的依赖探测与安装配置由其维护。
- Runtime 与本 SDK 独立构建，不链接本 SDK。如消费方将 `IoRequestState` 映射到自己的 wire 协议，
  应以本仓库公开头文件 [io_adapter.h](../common/include/io_adapter.h) 中的枚举为依据，并在该枚举变化时核对映射与兼容性。
  SDK 文档不以消费方私有源码路径作为使用前提。
- 设备 SDK 不负责 Apps lifecycle、Softbus 容器或共享内存通信；consumer 只能依赖公开头文件和 staged library。
- 固定现场 IP/串口/电机命令只属于 `hardware` 测试或 commissioning，不进入默认无设备回归。
- SDK ABI、timeout、错误结果与 cancellation 变化必须同时验证主工程和独立 `app` consumer。

## 模块边界与编译组合（设计）

目标是通信、会话、异步适配和设备接口可分别开发与测试，消费者按所需能力组合。
当前仍由 `common` 聚合通信/会话/I/O Adapter，`motor`、`mirobot` 分别构建设备库；以下为职责设计，
尚未提供新的拆分库、CMake 选择项或分发格式。

| 职责单元 | 接口与依赖边界 | 独立验证与扩展位置 |
| --- | --- | --- |
| 通信合同与会话 | `CommInterface` 与 `DeviceSession` 表达有界收发、设备前缀、等待及错误；不依赖具体设备类 | 协议会话使用 fake transport；平台/传输实现通过公开通信合同接入 |
| 具体通信与平台适配 | TCP、UDP、共享 UDP、server 和串口维护所属连接/句柄 | 按 OS 与传输分别验证连接、超时和退出；原生句柄保留在适配实现中 |
| I/O Adapter | 请求身份、Accepted/terminal、deadline、容量与取消/清理 | 通过通信适配及受控 worker 组合，不反向依赖 Motor/Mirobot 或平台 Runtime |
| Motor 与 Mirobot | 各自设备命令、解析和错误，通过会话或通信接口消费传输 | 设备合同测试用隔离通信夹具；实体设备验收单独进行；新增设备库不要求修改已有设备实现 |
| 构建与 SDK stage | 选择实现、输出公开头/库及配套链接依赖 | 保留现有平台 tag 和 staged 独立消费；选择项及拆分粒度由实际消费需求确定 |
| app 与示例 | 公共 SDK 消费者，不作为库的反向依赖 | 从 staged SDK 构建，按所需设备库组合；无设备 smoke 与现场动作分别组织 |

设备对象可以继续提供兼容的便捷构造，默认会话由受控装配创建；它与显式注入会话共享协议实现，
socket/串口控制由通信层统一实现。接口与实现分层保持已发布方法、错误枚举、请求终态和同步等待语义。

后续若拆分 `common` 或增加按设备/传输选择，须同时核对共享 UDP 的进程内共享状态、线程/句柄所有权、
静态或动态链接依赖和 stage 内容，确保跨库组合仍复用同一会话责任。现有 target、库名、头文件路径和已发布 ABI
在给出版本化迁移前保持兼容，不将逻辑划界写成独立构建已交付。
候选审计与验证见[计划](plan.md)，现有命令和资格见[构建指南](building.md)及[状态](status.md)。

## I/O Adapter SDK

`common/include/io_adapter.h` 提供 `IoAdapterClient`：`io_request_async(adapter_key, payload, deadline_ns)`
生成稳定的 `uint64_t` `IoRequestId`，completion callback 先交付 `Accepted`，再交付一次 terminal `IoResponse`；
`io_request_sync(...)` 复用同一提交路径并以 bounded wait 返回 terminal state。`deadline_ns` 是 monotonic
绝对时刻，`0` 表示不设置 adapter deadline；SDK 只做 deadline 协调，不提供物理设备时限保证。

`IoPayload` 默认做一次显式 copy，也可通过 `IoPayload::from_shared()` 复用已有 byte storage。
`CommInterfaceIoAdapter`、`TcpClientIoAdapter` 和 `Rs485IoAdapter` 复用现有 `CommInterface`/`TcpComm`/
`SerialComm`，通过专用 worker callback 实现 async，不改变 `DeviceSession`、Motor 和 Mirobot 的同步注入路径。
