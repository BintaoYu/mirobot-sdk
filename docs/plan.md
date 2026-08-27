# Industrial Foundation Plan

## 当前目标

保持设备通信 SDK 小而稳定，并确保 native、ARM64、Windows MinGW 与 Windows MSVC ABI consumer 路径可复现。

## 后续工作

1. 为公共 transport/session API 明确 timeout、错误结果、peer identity 与 cancellation 语义。
2. 对公共头文件、install/stage 规则和 ABI 改动运行全部跨平台构建及独立 `app` consumer。
3. 将无硬件回归与现场 `steel_test` 明确分离，避免默认 CI 阻塞在真实设备。
4. 在设备、endpoint、安全负责人和验收标准确认后执行 Motor/Mirobot 现场验证。
5. Foundation API 被 Apps/Paper 使用方式变化时，同步 `docs/status.md` 和上级 platform 状态。
