# Industrial Foundation Plan

Foundation 自 2026-09-18 起冻结：它是 Industrial-Apps 的可选构建期依赖（见 [architecture.md](architecture.md)
「消费方与边界」），任何改动都必须保持 `app/sdk/<platform-tag>/{include,lib}` 的 staged 布局。

## 未完成工作

1. 为公共 transport/session API 补充 peer identity 与 cancellation 语义（当前 timeout/deadline/result 合同已冻结）。
2. 公共头文件、install/stage 规则或 ABI 改动时运行 native、ARM64、MinGW、MSVC-cross 四条构建及独立 `app` consumer；
   改动 `IoRequestState` 数值时同步 Softbus `io_types.h`。
3. 在设备、endpoint、安全负责人和验收标准确认后执行 Motor/Mirobot 现场验证，并记录可重复的验收结果。
4. Foundation 被 Apps 使用的方式变化时，同步 [status.md](status.md) 与上级平台状态。
