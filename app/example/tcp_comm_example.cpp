#include "tcp_comm.h"
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

namespace {

struct TcpExampleConfig {
    std::string ip = "192.168.31.120";
    int port = 8000;
    std::string first_command = "@10 G6 F0";
    std::string second_command = "@2 $H";
};

constexpr bool kEnableTcpExample = false;

void runTcpExample(const TcpExampleConfig& config) {
    TcpComm& tcp_comm = TcpComm::getInstance();

    if (!tcp_comm.connectToServer(config.ip, config.port)) {
        std::cerr << "TCP 连接失败: " << config.ip << ":" << config.port << std::endl;
        return;
    }

    std::cout << "TCP 已连接: " << config.ip << ":" << config.port << std::endl;

    // writeData() 会自动补换行。
    tcp_comm.writeData(config.first_command);
    tcp_comm.writeData(config.second_command);

    for (int i = 0; i < 20; ++i) {
        if (tcp_comm.dataAvailable()) {
            std::cout << "tcp recv: " << tcp_comm.readLine() << std::endl;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    tcp_comm.disconnect();
}

} // namespace

int main() {
    if (!kEnableTcpExample) {
        std::cout
            << "tcp_comm_example 是实际接入模板。\n"
            << "先修改文件里的 IP、端口和命令，再把 kEnableTcpExample 改成 true 后运行。\n";
        return 0;
    }

    TcpExampleConfig config;
    runTcpExample(config);
    return 0;
}
