# Repository Guidelines

## Project Structure & Module Organization
`common/include` and `common/src` contain shared communication abstractions such as `CommInterface`, `SerialComm`, and `TcpComm`. `libs/motor` and `libs/mirobot` provide device-specific shared libraries. `tests/main_test.cpp` builds the `steel_test` executable used for integration-style checks. The root [`CMakeLists.txt`](/mnt/nvme/comfyui-pt29/Steel/CMakeLists.txt) wires targets together, and `cmake/arm64_toolchain.cmake` supports AArch64 cross-builds. Keep new headers next to their owning module; expose only reusable interfaces through `common/include`.

## Build, Test, and Development Commands
`cmake -S . -B build` configures a local build tree.

`cmake --build build` builds `common`, `motor`, `mirobot`, and `steel_test`.

`cmake --build build --target steel_test` rebuilds only the test executable during iteration.

`cmake -S . -B build-arm64 -DCMAKE_TOOLCHAIN_FILE=cmake/arm64_toolchain.cmake` configures an ARM64 cross-build.

`./build/tests/steel_test` is a hardware or integration check, not a pure unit test. Run it only with a reachable comm backend or after adapting the harness.

## Coding Style & Naming Conventions
Use 4-space indentation. Preserve the local brace and spacing style of the file you are editing rather than reformatting unrelated lines. Class names use `PascalCase`; filenames and many methods use `snake_case`, for example `motor.cpp` and `set_speed`. Follow the dominant style within each module instead of mixing conventions in one file. Keep headers minimal, prefer existing include-guard patterns, and avoid introducing new `using namespace std;` directives.

## Testing Guidelines
Test coverage is currently light and centered on transport-backed behavior. Add new tests under `tests/` and prefer filenames ending in `*_test.cpp`. Verify both successful builds and expected comm behavior. If a test depends on hardware, document the required serial device, IP, or port in the test source and PR. When changing comm code, avoid infinite waits; add timeouts or clear failure paths.

## Commit & Pull Request Guidelines
Recent history uses short subjects such as `tcp_comm`, `v2.0`, and date tags. Keep commits small and topic-focused, but prefer descriptive subjects over date-only labels, for example `motor: guard wait loop`. PRs should include a short summary, affected modules, the exact build or test commands run, and any hardware assumptions. For protocol changes, include a sample command, log, or observed device response.
