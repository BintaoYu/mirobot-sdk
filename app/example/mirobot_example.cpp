#include "mirobot.h"
#include <iostream>
#include <string>

namespace {

struct MirobotExampleConfig {
    std::string ip = "192.168.31.120";
    int port = 8000;
    int device_port = 2;
    int speed = 2000;
};

constexpr bool kEnableMirobotExample = false;

void runMirobotExample(const MirobotExampleConfig& config) {
    Mirobot mirobot(config.ip, config.port, config.device_port, true);

    // 直接传 IP/端口时，Mirobot 默认复用共享 UDP 通道。
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
            << "先修改 IP、端口和 device_port，再把 kEnableMirobotExample 改成 true 后运行。\n";
        return 0;
    }

    MirobotExampleConfig config;
    runMirobotExample(config);
    return 0;
}
