# Steel

这是一个基于 CMake 的 C++ 共享库项目，包含通信层、`motor`、`mirobot`，以及一个独立的 `app` 消费工程。

## 主要目录

- `common/`：通信实现
- `libs/`：业务库源码
- `tests/`：主工程测试
- `app/`：独立消费工程
- `app/include/`：主工程构建后自动复制的头文件
- `app/lib/`：主工程构建后自动复制的共享库
- `app/example/`：外部使用模板
- `app/test/`：`app` 工程自己的 smoke test

## 依赖安装

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build
```

## 主工程构建

在仓库根目录执行：

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

构建完成后会自动复制：

- `app/include/*.h`
- `app/lib/*.so`

## app 独立构建

先完成一次主工程构建，再进入 `app/`：

```bash
cd app
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

`app` 不引用 `common/` 或 `libs/` 里的源码，只使用：

- `app/include`
- `app/lib`

## Example 模板

`app/example/` 提供三个真实接入模板：

- `tcp_comm_example.cpp`：`TcpComm` 的连接、发命令、收响应模板
- `motor_example.cpp`：`Motor` 通过 `TcpComm` 控制设备的模板
- `mirobot_example.cpp`：`Mirobot` 通过 `SerialComm` 控制设备的模板

这些模板默认不会真的连接设备。使用方式：

1. 打开对应示例文件
2. 修改里面的 IP、端口、串口、设备号等参数
3. 把文件里的 `kEnable...Example` 改成 `true`
4. 重新编译后运行对应程序

示例目标如下：

- `app/build/bin/tcp_comm_example`
- `app/build/bin/motor_example`
- `app/build/bin/mirobot_example`

## 交叉编译

Linux ARM64：

```bash
cmake -S . -B build-arm64 -DCMAKE_TOOLCHAIN_FILE=cmake/arm64_toolchain.cmake
cmake --build build-arm64
```

Windows x64：

```bash
cmake -S . -B build-win-cross -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64_toolchain.cmake
cmake --build build-win-cross
```

## 说明

- Linux 串口常见为 `/dev/ttyUSB0` 或 `/dev/ttyACM0`
- Windows `COM10` 及以上通常要写成 `\\\\.\\COM10`
- `Motor::set_speed()`、`Mirobot::set_speed()` 这类接口通常会等待设备返回；如果控制器不回包，调用可能阻塞
