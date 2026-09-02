# Industrial Foundation Status

## 当前能力

- 构建 `common`、`motor`、`mirobot` 共享库。
- 将公开头文件和库 stage 到 `app/sdk/<platform-tag>/`。
- 支持 native、Linux ARM64、Windows x64 MinGW 和 Linux-hosted MSVC ABI cross build。
- `app/` 可作为独立 SDK consumer 构建示例与 smoke test。
- `common` 已提供 event-driven `IoAdapterClient` SDK：stable request ID、accepted/terminal
  callback、bounded sync wrapper、absolute monotonic deadline 传播和 typed
  `IoError`/request state；`TcpClientIoAdapter`/`Rs485IoAdapter` 复用现有 TCP/serial
  communication primitives。

## 验证边界

- `./steel native` 包含主工程和 `app` 验证，但注册的 `steel_test` 依赖真实硬件。
- 无设备环境应使用 `ctest --test-dir .build/native --output-on-failure -LE hardware`。
- loopback transport 和 build smoke 不构成 Motor/Mirobot 现场验收。
- I/O Adapter tests 是 contract/transport correctness evidence，不声称 deadline guarantee、
  WCET 或现场设备验收。

## 当前未完成

- cancellation 与 peer identity 的后续公共语义仍未完成；当前 timeout/deadline/result
  contract 已冻结。
- 真实设备、网络和安全条件下的可重复验收记录。
