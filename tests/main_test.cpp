#include "mirobot.h"
#include "motor.h"
#include "serial_comm.h"
#include "tcp_comm.h"
#include <iostream>
#include <string>

namespace {

void example_tcp_comm_basic() {
    // 1. 获取 TcpComm 单例。
    // 2. 连接到控制器 TCP 服务。
    // 3. 直接通过 CommInterface 接口发送原始指令。
    // 4. 按需轮询返回内容。
    TcpComm& tcp_comm = TcpComm::getInstance();

    const std::string ip = "192.168.31.120";
    const int port = 8000;

    if (!tcp_comm.connectToServer(ip, port)) {
        std::cerr << "TCP 连接失败: " << ip << ":" << port << std::endl;
        return;
    }

    std::cout << "TCP 已连接: " << ip << ":" << port << std::endl;

    // 直接发送协议字符串。
    // writeData() 会自动补 '\n'，所以这里不需要自己再加换行。
    tcp_comm.writeData("@10 G6 F0");
    tcp_comm.writeData("@2 $H");

    // 如果设备有返回，可以用 dataAvailable() + readLine() 读取。
    while (tcp_comm.dataAvailable()) {
        std::cout << "tcp recv: " << tcp_comm.readLine() << std::endl;
    }

    tcp_comm.disconnect();
}

void example_motor_over_tcp() {
    // Motor 通过 CommInterface 工作，所以可以直接挂在 TcpComm 上。
    // device_port 是协议里的设备号，例如 @10。
    TcpComm& tcp_comm = TcpComm::getInstance();

    const std::string ip = "192.168.31.120";
    const int port = 8000;
    const int device_port = 10;

    if (!tcp_comm.connectToServer(ip, port)) {
        std::cerr << "Motor 所需 TCP 连接失败: " << ip << ":" << port << std::endl;
        return;
    }

    Motor motor(&tcp_comm, device_port, true);

    // 直接发底层运动指令，不等待返回。
    motor.send_msg("G6 F500", false);

    // set_speed() 内部会发送 "G6 Fxxx"，并等待设备有返回。
    // 如果控制器不会回包，这类接口可能阻塞。
    motor.set_speed(1000);
    motor.set_speed(0);

    // load() 会先发送初始化指令，再设置速度。
    motor.load(300);

    // 如果当前通道里还有状态回包，可以主动读出。
    std::cout << "motor status: " << motor.read_status() << std::endl;

    tcp_comm.disconnect();
}

void example_mirobot_over_serial() {
    // Mirobot 最常见的是串口接入。
    // Linux 常见串口路径: /dev/ttyUSB0, /dev/ttyACM0
    // Windows COM10 以上通常写成: \\\\.\\COM10
    const std::string serial_port = "/dev/ttyUSB0";
    const int device_port = 2;

    SerialComm serial_comm(serial_port);
    serial_comm.flush();

    Mirobot mirobot(&serial_comm, device_port, true);

    // 常见初始化顺序:
    // 1. 解锁
    // 2. 设置速度
    // 3. 回零
    mirobot.unlock_shaft();
    mirobot.set_speed(2000);
    mirobot.go_to_zero(0);

    // 关节运动示例。
    mirobot.go_to_axis(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    mirobot.increment_axis(5.0, 0.0, 0.0, 0.0, 0.0, 0.0);

    // 笛卡尔运动示例。
    mirobot.go_to_cartesian_lin(
        mirobot.motiontype.MOVJ,
        200.0, 0.0, 120.0,
        0.0, 0.0, 0.0,
        0);

    mirobot.increment_cartesian_lin(
        mirobot.motiontype.MOVL,
        10.0, 0.0, 0.0,
        0.0, 0.0, 0.0,
        0);

    // 末端执行器示例。
    mirobot.suction_cup_on(1);
    mirobot.suction_cup_off(1);
    mirobot.gripper_on();
    mirobot.gripper_off();

    // 读取状态示例。
    mirobot.send_msg("?", false);
    mirobot.waitForEnd();
    std::cout << "mirobot status: " << mirobot.read_status() << std::endl;
}

void example_mirobot_over_tcp() {
    // 如果 Mirobot 控制器暴露的是 TCP 接口，也可以直接复用 TcpComm。
    TcpComm& tcp_comm = TcpComm::getInstance();

    const std::string ip = "192.168.31.120";
    const int port = 8000;
    const int device_port = 2;

    if (!tcp_comm.connectToServer(ip, port)) {
        std::cerr << "Mirobot TCP 连接失败: " << ip << ":" << port << std::endl;
        return;
    }

    Mirobot mirobot(&tcp_comm, device_port, true);

    mirobot.home_simultaneous();
    mirobot.set_speed(1500);
    mirobot.direction_mobility(mirobot.movedirection.dforward, 20.0, 0);
    mirobot.move_to_axis(1, 1, 10.0);

    // camera_msg() 适合发送一条消息并等待一条返回。
    std::cout << "camera/tcp recv: " << mirobot.camera_msg("STATUS?", true, 0) << std::endl;

    tcp_comm.disconnect();
}

} // namespace

int main() {
    // 这个文件现在作为“接口使用示例”，默认不执行任何真实设备动作。
    // 使用方式:
    // 1. 先把 IP、端口、串口路径、device_port 改成现场参数。
    // 2. 再取消下面某一个示例调用的注释。
    // 3. 确认设备在线且允许当前动作后再运行。

    // example_tcp_comm_basic();
    // example_motor_over_tcp();
    // example_mirobot_over_serial();
    // example_mirobot_over_tcp();

    std::cout << "tests/main_test.cpp is an example file. Uncomment one example in main() before running." << std::endl;
    return 0;
}
