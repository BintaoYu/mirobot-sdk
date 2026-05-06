#include "motor.h"
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
    Motor motor(config.ip, config.port, config.device_port, true);

    // 直接传 IP/端口时，Motor 默认复用共享 UDP 通道。
    motor.send_msg("G6 F500", false);

    // 控制器如果会回包，同一个共享 UDP socket 会收到返回。
    motor.set_speed(config.run_speed);
    motor.set_speed(0);

    std::cout << "motor status: " << motor.read_status() << std::endl;
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
