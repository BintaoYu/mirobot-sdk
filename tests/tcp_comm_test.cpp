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
    TcpComm tcp_comm;
    tcp_comm.disconnect();

    CHECK(!tcp_comm.connect("not-an-ip", 8000));
    CHECK(!tcp_comm.dataAvailable());
}

void writeData_appends_newline_for_server_side_reads() {
    LoopbackTcpServer server;
    TcpComm tcp_comm;
    tcp_comm.disconnect();

    CHECK(tcp_comm.connect("127.0.0.1", server.port()));
    tcp_comm.writeData("@10 G6 F1000");

    CHECK_EQ(server.readLine(), std::string("@10 G6 F1000"));
    tcp_comm.disconnect();
}

void dataAvailable_and_readLine_consume_crlf_messages() {
    LoopbackTcpServer server;
    TcpComm tcp_comm;
    tcp_comm.disconnect();

    CHECK(tcp_comm.connect("127.0.0.1", server.port()));
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

void independent_instances_can_connect_to_different_servers() {
    LoopbackTcpServer first_server;
    LoopbackTcpServer second_server;
    TcpComm first_client;
    TcpComm second_client;

    CHECK(first_client.connect("127.0.0.1", first_server.port()));
    CHECK(second_client.connect("127.0.0.1", second_server.port()));

    first_client.writeData("PING-1");
    second_client.writeData("PING-2");

    CHECK_EQ(first_server.readLine(), std::string("PING-1"));
    CHECK_EQ(second_server.readLine(), std::string("PING-2"));

    first_client.disconnect();
    second_client.disconnect();
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
        {"TcpComm instances manage independent connections", independent_instances_can_connect_to_different_servers},
    };

    return test_support::runTests(tests);
}
