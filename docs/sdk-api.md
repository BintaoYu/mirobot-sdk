# Mirobot SDK API 与示例

本文说明通信会话、设备对象和 I/O Adapter 的调用方式。请先按[构建指南](building.md)生成对应平台的 staged SDK。

## Example 模板

`app/example/` 里有五个模板：

- `tcp_comm_example.cpp`：`TcpComm` 连接、发送和接收
- `tcp_server_example.cpp`：`TcpServerComm` 监听、接受客户端和收发消息
- `udp_comm_example.cpp`：`UdpComm` datagram 发送和响应读取
- `motor_example.cpp`：`Motor(ip, port, device_port)` 通过共享 UDP 控制设备
- `mirobot_example.cpp`：`Mirobot(ip, port, device_port)` 通过共享 UDP 控制设备

使用方式：

1. 打开对应示例文件
2. 修改 IP、端口、监听地址、串口路径、设备号或命令
3. 如果文件里有 `kEnable...Example` 开关，改成 `true`
4. 重新构建并运行 `.build/app-<preset>/bin/` 下的目标程序，例如 `.build/app-native/bin/motor_example` 或 `.build/app-win-mingw/bin/motor_example.exe`

## API 使用要点

`TcpComm` 和 `UdpComm` 都实现 `CommInterface`：

```cpp
UdpComm comm;
comm.connect("192.168.31.120", 8000);
comm.writeData("@10 G6 F300");
```

`UdpComm` 使用 connected UDP socket，会自动补 `\n`。如果需要读取控制器回包，必须复用同一个 `UdpComm` 实例，因为控制器会把返回数据发给发起查询的源 IP 和端口。

`Motor` 和 `Mirobot` 支持直接通过 UDP endpoint 构造：

```cpp
Motor motor("192.168.31.120", 8000, 10);
Mirobot mirobot("192.168.31.120", 8000, 2);
```

这条路径会在进程内共享同一个底层 `UdpComm`。如果已有共享 UDP 通道绑定到一个目标，又尝试绑定到另一个目标，构造函数会抛出 `std::runtime_error`，避免静默切换控制器。

也可以注入任意 `CommInterface`：

```cpp
TcpComm tcp;
tcp.connect("192.168.31.120", 8000);

DeviceSession session(tcp, 10, " ");
Motor motor(session);
```

`DeviceSession` 负责设备号前缀、原始发送和同步等待，默认同步等待超时为 1000ms。`Motor::set_speed()`、`Mirobot::set_speed()` 这类同步接口在超时后会抛出 `std::runtime_error`。

I/O Adapter SDK 通过 `IoAdapterClient` 提供 event-driven request/response：

```cpp
IoAdapterClient client;
client.set_default_completion_handler([](IoResponse response) {
    if (response.terminal()) {
        // response.request_id、response.state 和 response.error 是最终事实。
    }
});

const IoRequestResult accepted = client.io_request_async(
    adapter_key, payload, deadline_ns);
IoResponse completed = client.io_request_sync(adapter_key, payload, deadline_ns);
```

async callback 先收到 `Accepted`，随后收到一次 terminal response。terminal state 明确区分
`Completed`、`Timeout`、`DeadlineMissed`、`TransportError`、`AdapterUnavailable` 和
`InvalidRequest`。`deadline_ns` 是 monotonic absolute deadline，只用于协调。使用共享 byte storage 时可用
`IoPayload::from_shared()` 避免 SDK 边界的额外 payload copy；普通 `IoPayloadView` 会执行
一次显式 SDK-owned copy。

`CommInterfaceIoAdapter`、`TcpClientIoAdapter` 和 `Rs485IoAdapter` 复用既有通信 primitive，
而 `DeviceSession`、Motor 和 Mirobot 的 legacy synchronous/injected APIs 保持不变。

## 备注

- Linux 串口常见路径为 `/dev/ttyUSB0` 或 `/dev/ttyACM0`
- Windows `COM10` 及以上通常写成 `\\\\.\\COM10`
- Windows SDK stage 目录按工具链区分，MinGW 和 MSVC ABI 不会互相覆盖
- `TcpComm` 是普通连接对象，不使用全局单例
- `TcpServerComm` 当前是单监听 socket + 单客户端连接模型；如需切换客户端，先调用 `disconnectClient()`

相关资料：[README](../README.md) · [架构](architecture.md) · [验证状态](status.md)。
