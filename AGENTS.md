# Repository Guidelines

Start with [README.md](README.md), [docs/architecture.md](docs/architecture.md),
[docs/plan.md](docs/plan.md), [docs/status.md](docs/status.md), and the parent
[platform guidelines](../AGENTS.md).

## Project Structure & Module Organization
`common/include` and `common/src` contain shared communication abstractions such as `CommInterface`, `SerialComm`, `TcpComm`, `TcpServerComm`, `UdpComm`, and `DeviceSession`. `libs/motor` and `libs/mirobot` provide device-specific shared libraries. `app/` is a standalone SDK consumer and must use staged headers and dynamic libraries from `app/sdk/<platform-tag>/` instead of linking project sources directly. `tests/main_test.cpp` builds the fixed-parameter `steel_test` executable used for direct motor hardware checks. The root `CMakeLists.txt`, `CMakePresets.json`, and `app/CMakePresets.json` provide unified build entry points. Keep new headers next to their owning module; expose only reusable interfaces through `common/include`.

## Build, Test, and Development Commands
Preferred entry point: `./steel ...`

`./steel native` builds the root project, runs root CTest, builds `app`, and runs the `app` smoke test. Root CTest includes the hardware-labeled `steel_test`, so use `ctest --test-dir .build/native --output-on-failure -LE hardware` when no device is available.

`./steel arm64` builds the root project and `app` for Linux ARM64.

`./steel win` builds the root project and `app` for Windows x64 with MinGW.

`./steel msvc-cross` builds the root project and `app` for Windows x64 MSVC ABI from Linux.

`./steel motor-test` rebuilds and runs `steel_test`.

`./steel clean` removes the unified `.build/` directory, and `./steel clean-legacy` removes old scattered `build*` and `app/build*` directories.

Manual CMake usage is still supported, but new build directories should live under `.build/` instead of creating more root-level `build-*` directories. Cross-builds should be validated through `./steel arm64`, `./steel win`, and especially `./steel msvc-cross` when changing public headers, install rules, or SDK staging.

## Coding Style & Naming Conventions
Use 4-space indentation. Preserve the local brace and spacing style of the file you are editing rather than reformatting unrelated lines. Class names use `PascalCase`; filenames and many methods use `snake_case`, for example `motor.cpp` and `set_speed`. Follow the dominant style within each module instead of mixing conventions in one file. Keep headers minimal, prefer existing include-guard patterns, and avoid introducing new `using namespace std;` directives.

## Testing Guidelines
Test coverage is centered on transport-backed behavior and SDK consumption. Add new tests under `tests/` and prefer filenames ending in `*_test.cpp`. Verify both successful builds and expected comm behavior. If a test depends on hardware, document the required IP, port, or serial device in the test source and PR. When changing comm code, avoid infinite waits; add timeouts or clear failure paths. When changing public API or install/stage behavior, also build `app` so the standalone dynamic-library consumer path is covered.

## Commit & Pull Request Guidelines
Recent history uses short subjects such as `tcp_comm`, `v2.0`, and date tags. Keep commits small and topic-focused, but prefer descriptive subjects over date-only labels, for example `motor: guard wait loop`. PRs should include a short summary, affected modules, the exact build or test commands run, and any hardware assumptions. For protocol changes, include a sample command, log, or observed device response.

## Documentation Updates

Update `README.md` for user commands, `docs/architecture.md` for API/module
boundaries, `docs/plan.md` for ordered remaining work, and `docs/status.md` for
completed capability and verification changes. Do not create a parallel roadmap.
