#include "mirobot.h"
#include "motor.h"
#include "test_support.h"
#include "udp_test_support.h"
#include <vector>

using test_support::LoopbackUdpServer;
using test_support::TestCase;

namespace {

void motor_udp_constructor_sends_expected_datagram() {
    LoopbackUdpServer server;

    Motor motor("127.0.0.1", server.port(), 10);
    motor.send_msg("G6 F500", false);

    CHECK_EQ(server.readDatagram(), std::string("@10 G6 F500\n"));
}

void mirobot_udp_constructor_sends_expected_datagram() {
    LoopbackUdpServer server;

    Mirobot mirobot("127.0.0.1", server.port(), 2);
    mirobot.send_msg("M50", false);

    CHECK_EQ(server.readDatagram(), std::string("@2M50\n"));
}

void motor_and_mirobot_share_the_same_udp_source_port() {
    LoopbackUdpServer server;

    Motor motor("127.0.0.1", server.port(), 10);
    Mirobot mirobot("127.0.0.1", server.port(), 2);

    motor.send_msg("G6 F500", false);
    CHECK_EQ(server.readDatagram(), std::string("@10 G6 F500\n"));
    const uint16_t first_peer_port = server.lastPeerPort();

    mirobot.send_msg("M50", false);
    CHECK_EQ(server.readDatagram(), std::string("@2M50\n"));
    CHECK_EQ(server.lastPeerPort(), first_peer_port);
}

} // namespace

int main() {
    if (!test_support::canCreateUdpSockets()) {
        std::cout << "[SKIP] Shared UDP device tests skipped because this environment blocks creating UDP sockets." << std::endl;
        return 0;
    }

    const std::vector<TestCase> tests = {
        {"Motor can be constructed directly from a UDP endpoint", motor_udp_constructor_sends_expected_datagram},
        {"Mirobot can be constructed directly from a UDP endpoint", mirobot_udp_constructor_sends_expected_datagram},
        {"Motor and Mirobot reuse the same shared UDP source port", motor_and_mirobot_share_the_same_udp_source_port},
    };

    return test_support::runTests(tests);
}
