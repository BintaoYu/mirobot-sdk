#include "udp_comm.h"
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

namespace {

struct UdpExampleConfig {
    std::string ip = "192.168.31.120";
    int port = 8000;
    std::string first_command = "@10 G6 F0";
    std::string second_command = "@10 G6 F300";
};

constexpr bool kEnableUdpExample = false;

void runUdpExample(const UdpExampleConfig& config) {
    UdpComm udp_comm;

    if (!udp_comm.connect(config.ip, config.port)) {
        std::cerr << "UDP 目标不可用: " << config.ip << ":" << config.port << std::endl;
        return;
    }

    std::cout << "UDP 已准备好: " << config.ip << ":" << config.port << std::endl;

    // UdpComm 会像 TcpComm 一样自动补换行。
    // 控制器回包会发给最后一次发出查询的源 IP 和端口，所以查询和收包要复用同一个 UdpComm。
    udp_comm.writeData(config.first_command);
    udp_comm.writeData(config.second_command);

    for (int i = 0; i < 20; ++i) {
        if (udp_comm.dataAvailable()) {
            std::cout << "udp recv: " << udp_comm.readLine() << std::endl;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    udp_comm.disconnect();
}

} // namespace

int main() {
    if (!kEnableUdpExample) {
        std::cout
            << "udp_comm_example 是实际接入模板。\n"
            << "先修改文件里的 IP、端口和命令，再把 kEnableUdpExample 改成 true 后运行。\n";
        return 0;
    }

    UdpExampleConfig config;
    runUdpExample(config);
    return 0;
}
