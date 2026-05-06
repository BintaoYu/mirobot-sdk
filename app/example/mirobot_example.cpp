#include "device_session.h"
#include "mirobot.h"
#include "serial_comm.h"
#include <iostream>
#include <string>

namespace {

struct MirobotExampleConfig {
    std::string serial_port = "/dev/ttyUSB0";
    int device_port = 2;
    int speed = 2000;
};

constexpr bool kEnableMirobotExample = false;

void runMirobotExample(const MirobotExampleConfig& config) {
    SerialComm serial_comm(config.serial_port);
    serial_comm.flush();

    DeviceSession mirobot_session(serial_comm, config.device_port);
    Mirobot mirobot(mirobot_session, true);

    // 常见初始化顺序：解锁 -> 设置速度 -> 回零。
    mirobot.unlock_shaft();
    mirobot.set_speed(config.speed);
    mirobot.go_to_zero(0);

    // 关节移动模板。
    mirobot.go_to_axis(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    mirobot.increment_axis(5.0, 0.0, 0.0, 0.0, 0.0, 0.0);

    // 读取状态模板。
    mirobot.send_msg("?", false);
    mirobot.waitForEnd();
    std::cout << "mirobot status: " << mirobot.read_status() << std::endl;
}

} // namespace

int main() {
    if (!kEnableMirobotExample) {
        std::cout
            << "mirobot_example 是 Mirobot 类模板。\n"
            << "先修改串口和 device_port，再把 kEnableMirobotExample 改成 true 后运行。\n";
        return 0;
    }

    MirobotExampleConfig config;
    runMirobotExample(config);
    return 0;
}
