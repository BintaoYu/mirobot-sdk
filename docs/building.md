# Mirobot SDK 构建与测试

命令从 `mirobot-sdk/` 根目录执行，标明 `cd app` 的段落除外。preset 声明 CMake ≥ 3.22。
首次无硬件构建见[README](../README.md)；`./steel native` 包含真实设备测试。

## 依赖

基础构建依赖：

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build
```

Linux ARM64 交叉编译：

```bash
sudo apt-get install -y g++-aarch64-linux-gnu
```

Windows x64 MinGW 交叉编译：

```bash
sudo apt-get install -y g++-mingw-w64-x86-64-posix binutils-mingw-w64-x86-64
```

Linux host 上交叉编译 Windows x64 MSVC ABI：

```bash
sudo apt-get install -y curl xz-utils
```

MSVC ABI 交叉编译还需要本地 LLVM 和 xwin sysroot。可以用仓库脚本准备：

```bash
./scripts/setup_windows_msvc_cross_toolchain.sh
```

## 快速命令

优先使用仓库根目录的 `./steel`。所有新构建目录都统一放在 `.build/` 下。

```bash
./steel native
./steel arm64
./steel win
./steel msvc-cross
./steel motor-test
./steel clean
```

命令含义：

- `./steel native`：构建主工程，运行主工程 CTest，构建 `app`，运行 `app` smoke test
- `./steel arm64`：构建 Linux ARM64 主工程和 `app`
- `./steel win`：用 MinGW 构建 Windows x64 主工程和 `app`
- `./steel msvc-cross`：用 Linux host 上的 `clang-cl + lld-link + xwin` 构建 Windows x64 MSVC ABI 主工程和 `app`
- `./steel motor-test`：构建并运行固定现场参数的 `steel_test`
- `./steel clean`：删除 `.build/`

`./steel native` 会运行注册到 CTest 的主工程测试，其中 `steel_test` 是硬件测试，依赖默认设备可达；无设备时的命令见[测试](#测试)。

## 产物位置

### SDK stage 产物

主工程构建后会把给外部项目消费的 SDK stage 到 `app/sdk/<platform-tag>/`：

- `app/sdk/<platform-tag>/include/*.h`
- `app/sdk/<platform-tag>/lib/*`

常见平台标签：

- `linux-x86_64`
- `linux-aarch64`
- `windows-x86_64-mingw`
- `windows-x86_64-msvc`

不同平台的动态库和 import library 命名不同：

- Linux：`libcommon.so`、`libmotor.so`、`libmirobot.so`
- Windows MinGW：`libcommon.dll` + `libcommon.dll.a`，`libmotor.dll` + `libmotor.dll.a`，`libmirobot.dll` + `libmirobot.dll.a`
- Windows MSVC ABI：`common.dll` + `common.lib`，`motor.dll` + `motor.lib`，`mirobot.dll` + `mirobot.lib`

注意：`app/sdk/<platform-tag>/` 是 SDK stage 目录，不是 `app` 示例程序的输出目录。

### app 构建产物

`app` 的可执行程序输出在仓库根目录的 `.build/app-<preset>/bin/` 下：

- native：`.build/app-native/bin/`
- Linux ARM64：`.build/app-arm64/bin/`
- Windows x64 MinGW：`.build/app-win-mingw/bin/`
- Windows x64 MSVC ABI cross：`.build/app-win-msvc-cross/bin/`

这些目录里包含 `tcp_comm_example`、`tcp_server_example`、`udp_comm_example`、`motor_example`、`mirobot_example` 和 `steel_sdk_smoke_test`。Windows 构建会带 `.exe` 后缀，并把运行需要的 SDK `.dll` 复制到同一个 `bin/` 目录。

## 主工程构建

推荐用 preset：

```bash
cmake --preset native
cmake --build --preset native
```

等价的手动模式也可以使用，但新的 build tree 应放在 `.build/` 下：

```bash
cmake -S . -B .build/native -G Ninja
cmake --build .build/native
```

## app 独立构建

先完成对应平台的主工程构建，让 SDK stage 到 `app/sdk/<platform-tag>/`。然后进入 `app/`：

```bash
cd app
cmake --preset native
cmake --build --preset native
ctest --preset native --output-on-failure
```

构建完成后，示例程序和 smoke test 在 `../.build/app-native/bin/`。

`app` 默认消费当前目标平台对应的 SDK。也可以手动指定 SDK tag 或 SDK root：

```bash
cd app
cmake -S . -B ../.build/app-native -G Ninja \
  -DSTEEL_APP_SDK_TAG=linux-x86_64

cmake -S . -B ../.build/app-native -G Ninja \
  -DSTEEL_APP_SDK_ROOT=/absolute/path/to/sdk
```

## 交叉编译

`./steel arm64`、`./steel win`、`./steel msvc-cross` 会先构建主工程并 stage 对应平台 SDK，然后构建 `app`，验证外部消费工程可以链接这些动态库；`./steel msvc-cross` 在缺少工具链时自动运行 `scripts/setup_windows_msvc_cross_toolchain.sh`。下面是等价的手动 preset。

### Linux ARM64

```bash
cmake --preset arm64
cmake --build --preset arm64

cd app
cmake --preset arm64
cmake --build --preset arm64
```

SDK 会 stage 到 `app/sdk/linux-aarch64/`，`app` 可执行程序在 `.build/app-arm64/bin/`。

### Windows x64 MinGW

```bash
cmake --preset win-mingw
cmake --build --preset win-mingw

cd app
cmake --preset win-mingw
cmake --build --preset win-mingw
```

SDK 会 stage 到 `app/sdk/windows-x86_64-mingw/`，`app` 可执行程序在 `.build/app-win-mingw/bin/`。

### Windows x64 MSVC ABI from Linux

手动构建前先按[依赖](#依赖)准备 LLVM 与 xwin sysroot：

```bash
cmake --preset win-msvc-cross
cmake --build --preset win-msvc-cross

cd app
cmake --preset win-msvc-cross
cmake --build --preset win-msvc-cross
```

SDK 会 stage 到 `app/sdk/windows-x86_64-msvc/`，`app` 可执行程序在 `.build/app-win-msvc-cross/bin/`。这条路径使用 Linux host 上的 `clang-cl`、`lld-link`、`llvm-lib` 和 xwin sysroot，输出 MSVC ABI 兼容的 Windows x64 `.dll` 和 `.lib`。

交叉编译出的 Windows 程序和 ARM64 程序在当前 Linux x86_64 host 上只验证配置、编译、链接和 SDK stage，不在本机直接运行。

### Windows host 上的 MSVC

如果在 Windows 机器上直接用 Visual Studio/MSVC，先打开 x64 Developer Command Prompt，或先执行 `vcvars64.bat`。可以使用 toolchain：

```bash
cmake -S . -B .build/win-msvc -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/msvc_cl_x64_toolchain.cmake
cmake --build .build/win-msvc
```

也可以直接使用 Visual Studio 生成器。关键是主工程和 `app` 使用同一套 MSVC 环境，并保持 SDK tag 为 `windows-x86_64-msvc`。


## 测试

非硬件测试：

```bash
cmake --preset native
cmake --build --preset native
ctest --test-dir .build/native --output-on-failure -LE hardware
```

`app` smoke test 见 [app 独立构建](#app-独立构建)。

真实电机硬件入口：

```bash
./steel motor-test
```

`tests/main_test.cpp` 当前固定参数：

- IP：`192.168.31.120`
- 端口：`8000`
- `device_port`：`10`
- 运行速度：`300`
- 超时：`1000ms`

这个入口会直接构造 `Motor(ip, port, device_port)` 并执行 `set_speed(0)`、`set_speed(300)`、`set_speed(0)`，适合现场固定设备验收。


相关资料：[API 与示例](sdk-api.md) · [架构](architecture.md) · [验证状态](status.md)。
