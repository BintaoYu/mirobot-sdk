#include "tcp_comm.h"
#include "tcp_test_support.h"
#include "test_support.h"
#include <chrono>
#include <thread>
#include <vector>

using test_support::LoopbackTcpServer;
using test_support::TestCase;

namespace {

void invalid_ipv4_address_is_rejected() {
    TcpComm& tcp_comm = TcpComm::getInstance();
    tcp_comm.disconnect();

    CHECK(!tcp_comm.connectToServer("not-an-ip", 8000));
    CHECK(!tcp_comm.dataAvailable());
}

void writeData_appends_newline_for_server_side_reads() {
    LoopbackTcpServer server;
    TcpComm& tcp_comm = TcpComm::getInstance();
    tcp_comm.disconnect();

    CHECK(tcp_comm.connectToServer("127.0.0.1", server.port()));
    tcp_comm.writeData("@10 G6 F1000");

    CHECK_EQ(server.readLine(), std::string("@10 G6 F1000"));
    tcp_comm.disconnect();
}

void dataAvailable_and_readLine_consume_crlf_messages() {
    LoopbackTcpServer server;
    TcpComm& tcp_comm = TcpComm::getInstance();
    tcp_comm.disconnect();

    CHECK(tcp_comm.connectToServer("127.0.0.1", server.port()));
    server.sendRaw("READY\r\n");

    bool ready = false;
    for (int i = 0; i < 50; ++i) {
        if (tcp_comm.dataAvailable()) {
            ready = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    CHECK(ready);
    CHECK_EQ(tcp_comm.readLine(), std::string("READY"));
    CHECK(!tcp_comm.dataAvailable());
    tcp_comm.disconnect();
}

} // namespace

int main() {
    if (!test_support::canCreateTcpSockets()) {
        std::cout << "[SKIP] TcpComm tests skipped because this environment blocks creating TCP sockets." << std::endl;
        return 0;
    }

    const std::vector<TestCase> tests = {
        {"TcpComm rejects invalid IPv4 addresses", invalid_ipv4_address_is_rejected},
        {"TcpComm::writeData appends a newline", writeData_appends_newline_for_server_side_reads},
        {"TcpComm::dataAvailable and readLine consume CRLF", dataAvailable_and_readLine_consume_crlf_messages},
    };

    return test_support::runTests(tests);
}
