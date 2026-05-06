#include "tcp_server_comm.h"
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

namespace {

struct TcpServerExampleConfig {
    std::string bind_ip = "0.0.0.0";
    int listen_port = 9000;
    int accept_timeout_ms = 10000;
    std::string reply_prefix = "ack: ";
};

constexpr bool kEnableTcpServerExample = true;

void runTcpServerExample(const TcpServerExampleConfig& config) {
    TcpServerComm tcp_server;

    if (!tcp_server.listen(config.bind_ip, config.listen_port)) {
        std::cerr << "TCP 服务启动失败: " << config.bind_ip << ":" << config.listen_port << std::endl;
        return;
    }

    std::cout << "TCP 服务已监听: " << config.bind_ip << ":" << tcp_server.port() << std::endl;

    if (!tcp_server.acceptClient(config.accept_timeout_ms)) {
        std::cerr << "等待客户端连接超时: " << config.accept_timeout_ms << "ms" << std::endl;
        tcp_server.stop();
        return;
    }

    std::cout << "客户端已连接，可以开始收发消息。" << std::endl;

    for (int i = 0; i < 50; ++i) {
        if (!tcp_server.dataAvailable()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        const std::string request = tcp_server.readLine();
        if (request.empty() && !tcp_server.hasClient()) {
            std::cout << "客户端已断开连接。" << std::endl;
            break;
        }

        std::cout << "tcp recv: " << request << std::endl;

        // writeData() 会自动补换行。
        tcp_server.writeData(config.reply_prefix + request);
    }

    tcp_server.disconnectClient();
    tcp_server.stop();
}

} // namespace

int main() {
    if (!kEnableTcpServerExample) {
        std::cout
            << "tcp_server_example 是实际接入模板。\n"
            << "先修改文件里的监听 IP、端口和回包逻辑，再把 kEnableTcpServerExample 改成 true 后运行。\n";
        return 0;
    }

    TcpServerExampleConfig config;
    runTcpServerExample(config);
    return 0;
}
