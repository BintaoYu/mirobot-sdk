#include "udp_comm.h"
#include "test_support.h"
#include "udp_test_support.h"
#include <chrono>
#include <thread>
#include <vector>

using test_support::LoopbackUdpServer;
using test_support::TestCase;

namespace {

void invalid_ipv4_address_is_rejected() {
    UdpComm udp_comm;
    udp_comm.disconnect();

    CHECK(!udp_comm.connect("not-an-ip", 8000));
    CHECK(!udp_comm.dataAvailable());
}

void invalid_udp_ports_are_rejected() {
    UdpComm udp_comm;
    udp_comm.disconnect();

    CHECK(!udp_comm.connect("127.0.0.1", 0));
    CHECK(!udp_comm.connect("127.0.0.1", -1));
    CHECK(!udp_comm.connect("127.0.0.1", 70000));
    CHECK(!udp_comm.isConnected());
}

void writeData_appends_newline_for_udp_datagrams() {
    LoopbackUdpServer server;
    UdpComm udp_comm;
    udp_comm.disconnect();

    CHECK(udp_comm.connect("127.0.0.1", server.port()));
    udp_comm.writeData("@10 G6 F1000");

    CHECK_EQ(server.readDatagram(), std::string("@10 G6 F1000\n"));
    udp_comm.disconnect();
}

void readLine_returns_trimmed_udp_payloads_and_uses_buffered_lines() {
    LoopbackUdpServer server;
    UdpComm udp_comm;
    udp_comm.disconnect();

    CHECK(udp_comm.connect("127.0.0.1", server.port()));
    udp_comm.writeData("HELLO");
    CHECK_EQ(server.readDatagram(), std::string("HELLO\n"));

    server.sendRawToLastPeer("READY\r\nDONE\r\n");

    bool ready = false;
    for (int i = 0; i < 50; ++i) {
        if (udp_comm.dataAvailable()) {
            ready = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    CHECK(ready);
    CHECK_EQ(udp_comm.readLine(), std::string("READY"));
    CHECK(udp_comm.dataAvailable());
    CHECK_EQ(udp_comm.readLine(), std::string("DONE"));
    CHECK(!udp_comm.dataAvailable());
    udp_comm.disconnect();
}

void readLine_returns_buffered_tail_without_waiting_for_another_datagram() {
    LoopbackUdpServer server;
    UdpComm udp_comm;
    udp_comm.disconnect();

    CHECK(udp_comm.connect("127.0.0.1", server.port()));
    udp_comm.writeData("HELLO");
    CHECK_EQ(server.readDatagram(), std::string("HELLO\n"));

    server.sendRawToLastPeer("READY\nDONE");

    bool ready = false;
    for (int i = 0; i < 50; ++i) {
        if (udp_comm.dataAvailable()) {
            ready = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    CHECK(ready);
    CHECK_EQ(udp_comm.readLine(), std::string("READY"));
    CHECK(udp_comm.dataAvailable());
    CHECK_EQ(udp_comm.readLine(), std::string("DONE"));
    CHECK(!udp_comm.dataAvailable());
    udp_comm.disconnect();
}

void readLine_returns_a_whole_datagram_when_no_newline_is_present() {
    LoopbackUdpServer server;
    UdpComm udp_comm;
    udp_comm.disconnect();

    CHECK(udp_comm.connect("127.0.0.1", server.port()));
    udp_comm.writeData("PING");
    CHECK_EQ(server.readDatagram(), std::string("PING\n"));

    server.sendRawToLastPeer("STATUS=OK");

    bool ready = false;
    for (int i = 0; i < 50; ++i) {
        if (udp_comm.dataAvailable()) {
            ready = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    CHECK(ready);
    CHECK_EQ(udp_comm.readLine(), std::string("STATUS=OK"));
    CHECK(!udp_comm.dataAvailable());
    udp_comm.disconnect();
}

} // namespace

int main() {
    if (!test_support::canCreateUdpSockets()) {
        std::cout << "[SKIP] UdpComm tests skipped because this environment blocks creating UDP sockets." << std::endl;
        return 0;
    }

    const std::vector<TestCase> tests = {
        {"UdpComm rejects invalid IPv4 addresses", invalid_ipv4_address_is_rejected},
        {"UdpComm rejects invalid UDP ports", invalid_udp_ports_are_rejected},
        {"UdpComm::writeData appends a newline to UDP datagrams", writeData_appends_newline_for_udp_datagrams},
        {"UdpComm::readLine consumes buffered UDP lines", readLine_returns_trimmed_udp_payloads_and_uses_buffered_lines},
        {"UdpComm::readLine returns a buffered datagram tail", readLine_returns_buffered_tail_without_waiting_for_another_datagram},
        {"UdpComm::readLine returns the full datagram when no newline exists", readLine_returns_a_whole_datagram_when_no_newline_is_present},
    };

    return test_support::runTests(tests);
}
