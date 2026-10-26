# Mirobot SDK 协作规则

本文件适用于独立的公开 `mirobot-sdk` 仓库；在完整平台中同时遵循平台根规则。
按任务查阅 [README](README.md)、[架构](docs/architecture.md)、[状态](docs/status.md)、[计划](docs/plan.md)和[构建指南](docs/building.md)。
维护范围由当前任务及 plan/status 决定，不把历史“冻结”状态作为永久禁止开发的规则。

## 模块与消费边界

- `common/include`、`common/src` 承载通信抽象、会话和 I/O Adapter；`libs/motor`、`libs/mirobot` 承载设备接口。公共头放在所属模块，避免暴露私有实现。
- `app/` 是独立 SDK consumer，只使用 `app/sdk/<platform-tag>/` staged 头文件和动态库，不直接链接主项目源码来绕过分发验证。
- 平台与 ABI 使用不同 stage 目录；保持公开设备方法和包/导出标识兼容，既有方法如 `set_speed` 沿用原符号风格。
- Apps 按实际设备调用需要消费本 SDK；通用 Runtime 与其独立构建。共享 wire 枚举或合同变化需同步已知消费者及验证，不引入双方私有源码依赖。
- SDK 负责设备协议与通信结果，不负责 Apps 工程、Runtime 部署或宿主资源管理。保留请求身份、有界等待、超时、取消和真实设备错误语义，不伪造回执或动作完成。
- 公开仓库仅包含本 SDK 的源码、文档、示例和验证材料；凭据、现场私有配置、平台数据和其他组件源码不进入公开历史。

## 构建与验证

- 当前构建入口见 README 与 `./steel`；新构建目录位于 `.build/`。按目标平台选择工具链，不把交叉编译通过写成目标设备运行通过。
- 无硬件验证使用 README 的 CMake 流程及 `ctest --test-dir .build/native --output-on-failure -LE hardware`。`./steel native` 包含硬件测试，不能作为默认离线验证快捷方式。
- 协议/通信改动验证受影响路径；公共接口、安装或 stage 改动同时验证独立 `app` consumer，并选择受影响的 native/ARM64/Windows 目标。
- 工具链或目标机器不可用时明确报告未运行范围，不为文档改动或不受影响目标重复执行全量交叉构建。
- 硬件测试明确设备、地址、动作和停止责任，按用户有效授权执行；默认单元/回归路径不发送真实运动命令。
- 仅文档/AGENTS 修改做链接、结构和差异检查，不构建、安装或操作设备。

## 文档、风格与提交

使用和构建入口更新 README/building，接口与职责更新 sdk-api/architecture，剩余工作更新 plan，验证事实更新 status。
不在 AGENTS 记录临时冻结日期、最近提交风格或实验进度，不创建重复 roadmap。
使用四空格缩进，保持既有格式与 include guard；源码文件使用 snake_case、普通文档使用 kebab-case，新类型 PascalCase，既有 API 保持兼容。
公共头保持最小依赖，不新增 `using namespace std;`。
检查并保护工作区；仅在已有授权内精确提交/推送本轮文件，使用 Conventional Commits，平台集成先推子仓库再更新父仓库引用。
