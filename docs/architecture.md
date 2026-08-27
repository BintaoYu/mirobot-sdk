# Industrial Foundation Architecture

## 定位

Industrial Foundation 提供设备通信抽象和 Motor/Mirobot 公共 SDK。主工程构建共享库并按目标平台 stage 头文件与动态库；`app/` 是独立 consumer，只使用 staged SDK，不直接链接项目源码。

## 模块

| 目录 | 职责 |
| --- | --- |
| `common/include`、`common/src` | `DeviceSession` 与 Serial/TCP/UDP 公共通信抽象和实现 |
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
