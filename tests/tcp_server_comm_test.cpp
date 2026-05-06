#include "tcp_comm.h"
#include "tcp_server_comm.h"
#include "tcp_test_support.h"
#include "test_support.h"
#include <chrono>
#include <thread>
#include <vector>

using test_support::TestCase;

namespace {

bool waitForData(CommInterface& comm, std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (comm.dataAvailable()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return false;
}

void invalid_bind_address_is_rejected() {
    TcpServerComm tcp_server;

    CHECK(!tcp_server.listen("not-an-ip", 9000));
    CHECK(!tcp_server.isListening());
    CHECK(!tcp_server.hasClient());
    CHECK_EQ(tcp_server.port(), 0);
}

void server_accepts_a_client_and_exchanges_lines() {
    TcpServerComm tcp_server;
    TcpComm tcp_client;

    CHECK(tcp_server.listen("127.0.0.1", 0));
    CHECK(tcp_server.port() > 0);

    CHECK(tcp_client.connect("127.0.0.1", tcp_server.port()));
    CHECK(tcp_server.acceptClient(1000));
    CHECK(tcp_server.hasClient());

    tcp_client.writeData("PING");
    CHECK(waitForData(tcp_server));
    CHECK_EQ(tcp_server.readLine(), std::string("PING"));

    tcp_server.writeData("PONG");
    CHECK(waitForData(tcp_client));
    CHECK_EQ(tcp_client.readLine(), std::string("PONG"));

    tcp_client.disconnect();
    tcp_server.disconnectClient();
    tcp_server.stop();
}

void server_can_accept_a_second_client_after_disconnect() {
    TcpServerComm tcp_server;
    TcpComm first_client;
    TcpComm second_client;

    CHECK(tcp_server.listen("127.0.0.1", 0));
    CHECK(first_client.connect("127.0.0.1", tcp_server.port()));
    CHECK(tcp_server.acceptClient(1000));

    tcp_server.disconnectClient();
    first_client.disconnect();
    CHECK(!tcp_server.hasClient());

    CHECK(second_client.connect("127.0.0.1", tcp_server.port()));
    CHECK(tcp_server.acceptClient(1000));

    second_client.writeData("SECOND");
    CHECK(waitForData(tcp_server));
    CHECK_EQ(tcp_server.readLine(), std::string("SECOND"));

    second_client.disconnect();
    tcp_server.disconnectClient();
    tcp_server.stop();
}

} // namespace

int main() {
    if (!test_support::canCreateTcpSockets()) {
        std::cout << "[SKIP] TcpServerComm tests skipped because this environment blocks creating TCP sockets." << std::endl;
        return 0;
    }

    const std::vector<TestCase> tests = {
        {"TcpServerComm rejects invalid bind addresses", invalid_bind_address_is_rejected},
        {"TcpServerComm accepts a client and exchanges lines", server_accepts_a_client_and_exchanges_lines},
        {"TcpServerComm can accept a second client after disconnect", server_can_accept_a_second_client_after_disconnect},
    };

    return test_support::runTests(tests);
}
