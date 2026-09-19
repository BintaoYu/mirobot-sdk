# 2026-09-03 integration window（frozen history）

本文件是 2026-09-03 平台集成窗口（Window H）在 Foundation 文档中的原文记录，已冻结。它描述的多 Runtime
transport matrix、S0 target cells 与 Softbus 直接消费 Foundation I/O SDK 的关系已于 2026-09-18 随单 Runtime
重构删除；当前事实见 [architecture.md](../architecture.md) 与 [status.md](../status.md)。

## README：2026-09-03 integration window

Foundation 是统一 I/O Adapter 链路的设备 SDK authority：Softbus Runtime 通过公开
`IoAdapterClient`/backend contract 到达 `Rs485IoAdapter` 或 `TcpClientIoAdapter`，不会复制
设备通信实现。`IoRequestState`、`adapter_key`、`IoRequestId`、`deadline_ns` 和 accepted/terminal
语义已与 Softbus Runtime 对齐；`deadline_ns` 是 monotonic absolute deadline，`0` 表示不设置
adapter deadline，物理设备服务时间不作 guarantee claim。

Window H 的最终集成使用本项目重新 stage 的 SDK，并通过独立 consumer 和 Apps
benchmark-s0 Build 验证；Foundation 不拥有 Softbus target installer 或 Performance Run authority。
最终 S0 的 18/18 target cells 因而消费同一 staged SDK identity，但本项目仍未连接现场设备、
未执行硬件 commissioning，也不把 loopback transport smoke 写成设备时限保证。

## architecture：Window H integration status

The staged SDK was rebuilt and consumed by the final integration candidate. Its SDK tree identity is
recorded as d0b6e0f3298b6b03f9fa36129105ef041fca95026a133348592fabe20d4c6e8f in the bounded
integration evidence. Foundation remains the public device-SDK authority; Softbus owns target
installation/runtime lifecycle and Apps owns the Project-first S0 definition. The resulting
same-device-cross-runtime-transport-smoke is not physical hardware or deadline evidence.

## architecture：2026-09-03 Softbus integration boundary

The public SDK is the lower-layer I/O contract consumed by the Softbus Runtime. Both layers use the
same eight numeric request states: `Unknown`, `Accepted`, `Completed`, `Timeout`, `DeadlineMissed`,
`TransportError`, `AdapterUnavailable` and `InvalidRequest`. `IoRequestId` is `uint64_t` in the public
API; Softbus may serialize a checked narrower ID only at its existing shared-memory ABI boundary.

`deadline_ns` is an absolute value from the monotonic clock and `0` means no adapter deadline. SDK
submission preserves `adapter_key`, request ID and deadline through accepted then exactly one terminal
completion. The SDK coordinates and reports timeout/deadline outcomes but does not establish a physical
device deadline guarantee or WCET; `deadline_supported`, `deadline_observed` and
`deadline_guaranteed` are evidence facts owned by the Runtime adapter layer.

## status：2026-09-03 platform-next-integration

- I/O Adapter SDK ✅：Foundation public `IoAdapterClient`/backend contract is the canonical lower-layer
  contract for Softbus Runtime/RS485/TCP integration.
- Lifecycle and deadline semantics ✅：accepted then one terminal state, stable `uint64_t` request ID,
  adapter key/payload propagation, absolute monotonic `deadline_ns`, and `0=no adapter deadline`.
  The SDK reports coordination outcomes without claiming physical-device deadline guarantee or WCET.
- Native build, Foundation CTest, staged SDK consumer and I/O Adapter tests passed in this window. Real
  device/network acceptance and cancellation/peer-identity expansion remain incomplete.
- The final integration rebuilt and consumed the staged SDK; its recorded tree identity is
  d0b6e0f3298b6b03f9fa36129105ef041fca95026a133348592fabe20d4c6e8f. The complete S0 target baseline
  remains owned by Apps/Softbus and is scoped to same-device-cross-runtime-transport-smoke.
- Foundation does not own formal S0 target installation or Run lifecycle; Gate0B and other historical
  evidence remain bound to their original candidates.

## plan：2026-09-03 integration checkpoint（已完成项）

1. [x] Publish the canonical I/O Adapter request/result contract for Softbus consumption, including stable
   request IDs, lifecycle states, payload ownership and monotonic absolute deadlines.
2. [x] Verify native SDK and staged consumer integration in the platform next window, including the
   final Window H Apps/Softbus candidate consumption.
