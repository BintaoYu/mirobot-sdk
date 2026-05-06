#include "device_session.h"
#include "motor.h"
#include "tcp_comm.h"
#include <iostream>
#include <string>

namespace {

struct MotorExampleConfig {
    std::string ip = "192.168.31.120";
    int port = 8000;
    int device_port = 10;
    int run_speed = 1000;
};

constexpr bool kEnableMotorExample = false;

void runMotorExample(const MotorExampleConfig& config) {
    TcpComm tcp_comm;

    if (!tcp_comm.connect(config.ip, config.port)) {
        std::cerr << "Motor 所需 TCP 连接失败: " << config.ip << ":" << config.port << std::endl;
        return;
    }

    DeviceSession motor_session(tcp_comm, config.device_port, " ");
    Motor motor(motor_session, true);

    // 不等返回，适合控制器无回包或回包不可预期的场景。
    motor.send_msg("G6 F500", false);

    // 下面这些接口会等待设备返回，如果控制器不回包，可能阻塞。
    motor.set_speed(config.run_speed);
    motor.set_speed(0);

    std::cout << "motor status: " << motor.read_status() << std::endl;
    tcp_comm.disconnect();
}

} // namespace

int main() {
    if (!kEnableMotorExample) {
        std::cout
            << "motor_example 是 Motor 类模板。\n"
            << "先修改 IP、端口、device_port 和速度，再把 kEnableMotorExample 改成 true 后运行。\n";
        return 0;
    }

    MotorExampleConfig config;
    runMotorExample(config);
    return 0;
}
