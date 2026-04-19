# Steel

This project is a CMake-based C++ library set for communication and device control. The primary workflow is building on Ubuntu, either natively for Linux or by cross-compiling to other targets.

## Repository Layout

- `common/`: shared communication layer, including `SerialComm`, `TcpComm`, and `CommInterface`
- `libs/motor/`: motor control shared library
- `libs/mirobot/`: mirobot control shared library
- `tests/`: integration-style executable `steel_test`
- `cmake/`: cross-compilation toolchain files

## Supported Build Flows

The repository is currently set up for these paths:

- Ubuntu x86_64 -> Ubuntu/Linux x86_64
- Ubuntu x86_64 -> Linux ARM64
- Ubuntu x86_64 -> Windows x64 with MinGW-w64
- Windows native builds are also possible with Visual Studio, but Ubuntu is the main host workflow

## 1. Ubuntu Native Build

### Install build tools

On Ubuntu or Debian:

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build
```

### Configure and build

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

### Build outputs

- `build/common/libcommon.a`
- `build/libs/motor/libmotor.so`
- `build/libs/mirobot/libmirobot.so`
- `build/tests/steel_test`

## 2. Ubuntu x86_64 -> Linux ARM64 Cross Build

### Install cross-compilers

```bash
sudo apt-get update
sudo apt-get install -y gcc-aarch64-linux-gnu g++-aarch64-linux-gnu cmake ninja-build
```

### Toolchain file

The repository already provides:

- `cmake/arm64_toolchain.cmake`

### Configure and build

```bash
cmake -S . -B build-arm64 -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm64_toolchain.cmake
cmake --build build-arm64
```

This generates ARM64 Linux binaries on an Ubuntu x86_64 host.

## 3. Ubuntu x86_64 -> Windows x64 Cross Build

### Install MinGW-w64

```bash
sudo apt-get update
sudo apt-get install -y g++-mingw-w64-x86-64-posix gcc-mingw-w64-x86-64-posix binutils-mingw-w64-x86-64 cmake ninja-build
```

If your system exposes `x86_64-w64-mingw32-g++` through alternatives, the provided toolchain file works directly. If your distro only exposes the `-posix` suffixed compiler names, update the toolchain file accordingly.

### Toolchain file

The repository now provides:

- `cmake/mingw-w64-x86_64_toolchain.cmake`

### Configure and build

```bash
cmake -S . -B build-win-cross -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64_toolchain.cmake
cmake --build build-win-cross
```

Typical outputs will be Windows binaries such as:

- `build-win-cross/libs/motor/libmotor.dll`
- `build-win-cross/libs/mirobot/libmirobot.dll`
- `build-win-cross/tests/steel_test.exe`

These binaries cannot run natively on Ubuntu. Validate them on Windows or under a compatible runtime such as Wine.

## 4. Windows Native Build

If you later build directly on Windows:

```bash
cmake -S . -B build-win -G "Visual Studio 17 2022" -A x64
cmake --build build-win --config Release
```

## Runtime Notes

- `tests/steel_test` is closer to an integration check than a unit test. Without a reachable serial device or TCP backend, it may block waiting for input.
- On Linux, serial devices are typically `/dev/ttyUSB0` or `/dev/ttyACM0`.
- On Windows, ports above `COM9` usually need the `\\\\.\\COM10` form.
- `common` contains the platform-specific serial and socket implementations. Cross-platform support is handled there, not in the higher-level motor or robot APIs.

## Current Limitations

- Ubuntu -> Windows ARM64 is not documented as a default path. That target usually requires an LLVM/clang-based MinGW toolchain rather than the standard Debian or Ubuntu packages.
- Runtime validation for device communication still depends on actual hardware or a mock TCP backend.
