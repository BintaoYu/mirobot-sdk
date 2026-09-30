# Industrial Foundation Status

## 当前能力

- 构建 `common`、`motor`、`mirobot` 共享库，并把公开头文件和库 stage 到 `app/sdk/<platform-tag>/`。
- 支持 native、Linux ARM64、Windows x64 MinGW 和 Linux-hosted MSVC ABI cross build。
- `app/` 作为独立 SDK consumer 构建示例与 smoke test。
- `common` 提供 event-driven `IoAdapterClient` SDK（stable request ID、accepted/terminal callback、bounded sync
  wrapper、绝对 monotonic deadline、typed `IoError`/request state）；`TcpClientIoAdapter`/`Rs485IoAdapter`
  复用现有 TCP/serial primitive。
- 自 2026-09-18 起冻结，作为 Industrial-Apps 的可选构建期依赖；Softbus 不消费本项目（见
  [architecture.md](architecture.md)「消费方与边界」）。未完成项见 [plan.md](plan.md)。

## 最近验证

2026-09-19：native Release 构建（Ninja，`BUILD_TESTING=ON`）+ `ctest -LE hardware`：8/8 通过（motor、mirobot、
device_session、io_adapter、tcp_comm、udp_comm、shared_udp_device、tcp_server_comm）。

## 验证边界

无硬件测试只证明构建、传输合同和 SDK 消费路径；`steel_test`（`hardware` 标签）与 Motor/Mirobot 现场验收需要真实设备，
尚无可重复的现场验收记录。
