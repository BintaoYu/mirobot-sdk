# Mirobot SDK 状态

## 当前能力

| 方面 | 范围 |
| --- | --- |
| 库与 stage | `common`、`motor`、`mirobot` 共享库；头文件和库在 `app/sdk/<platform-tag>/` |
| 构建目标 | 本机、Linux ARM64、Windows x64 MinGW、Linux-hosted MSVC ABI |
| 独立消费 | `app/` 构建示例与 SDK smoke |
| I/O Adapter | 稳定 request ID、Accepted / terminal 回调、有界同步包装、monotonic 绝对 deadline 和 typed 错误 |
| 平台依赖 | Apps 的可选构建依赖，Runtime 不链接设备 SDK |

`TcpClientIoAdapter` / `Rs485IoAdapter` 复用既有 TCP / serial 原语。
当前没有新增功能开发计划，消费边界见[架构](architecture.md#消费方与边界)，维护范围见[计划](plan.md)。

## 文档同步

2026-10-20补齐独立构建、SDK API和消费示例文档，核对本仓库链接与既有源码接口。
本轮未修改设备SDK源码，也未重新运行现场设备或交叉编译测试；已完成的工程验证见下文。

## 最近验证

2026-09-19：native Release、Ninja、`BUILD_TESTING=ON` 构建，`ctest -LE hardware` 8/8 通过：
motor、mirobot、device_session、io_adapter、tcp_comm、udp_comm、shared_udp_device、tcp_server_comm。

## 验证边界

非硬件测试验证构建、传输合同和 SDK 消费路径。`steel_test` 带 `hardware` 标签，需要实际设备；
Motor / Mirobot 现场验收尚无可重复记录。命令和固定设备参数见[构建与测试](building.md)。
