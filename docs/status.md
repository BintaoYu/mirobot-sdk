# Industrial Foundation Status

## 当前能力

- 构建 `common`、`motor`、`mirobot` 共享库。
- 将公开头文件和库 stage 到 `app/sdk/<platform-tag>/`。
- 支持 native、Linux ARM64、Windows x64 MinGW 和 Linux-hosted MSVC ABI cross build。
- `app/` 可作为独立 SDK consumer 构建示例与 smoke test。

## 验证边界

- `./steel native` 包含主工程和 `app` 验证，但注册的 `steel_test` 依赖真实硬件。
- 无设备环境应使用 `ctest --test-dir .build/native --output-on-failure -LE hardware`。
- loopback transport 和 build smoke 不构成 Motor/Mirobot 现场验收。

## 当前未完成

- 更完整的 timeout/result/cancellation 公共语义。
- 真实设备、网络和安全条件下的可重复验收记录。
