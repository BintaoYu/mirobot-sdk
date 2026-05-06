#include "device_session.h"
#include "mirobot.h"
#include "motor.h"
#include "recording_comm.h"
#include "tcp_comm.h"
#include "tcp_server_comm.h"
#include "udp_comm.h"
#include <iostream>
#include <string>

namespace {

int fail(const std::string& message) {
    std::cerr << message << std::endl;
    return 1;
}

} // namespace

int main() {
    RecordingComm comm;
    comm.queueResponse("speed-ok");

    DeviceSession motor_session(comm, 10, " ");
    Motor motor(motor_session);
    motor.set_speed(1000);

    if (comm.writes().size() != static_cast<std::size_t>(1)) {
        return fail("Motor smoke test expected exactly one command write.");
    }

    if (comm.writes()[0] != "@10 G6 F1000") {
        return fail("Motor smoke test saw an unexpected command string.");
    }

    double value = 12.345;
    if (doubleToString(value) != "12.35") {
        return fail("Mirobot helper formatting check failed.");
    }

    comm.queueResponse("move-ok");
    DeviceSession mirobot_session(comm, 2);
    Mirobot mirobot(mirobot_session);
    mirobot.go_to_axis(1.0, 2.5, -3.0, 4.25, 5.0, 6.0);

    if (comm.writes().size() != static_cast<std::size_t>(2)) {
        return fail("Mirobot smoke test expected a second command write.");
    }

    if (comm.writes()[1] != "@2M21 G90 X1.00 Y2.50 Z-3.00 A4.25 B5.00 C6.00 F2000") {
        return fail("Mirobot smoke test saw an unexpected command string.");
    }

    TcpComm tcp_comm;
    tcp_comm.disconnect();
    if (tcp_comm.connect("not-an-ip", 8000)) {
        return fail("TcpComm should reject an invalid IPv4 address.");
    }

    TcpServerComm tcp_server;
    if (tcp_server.acceptClient(0)) {
        return fail("TcpServerComm should not accept clients before listen().");
    }

    UdpComm udp_comm;
    udp_comm.disconnect();
    if (udp_comm.connect("not-an-ip", 8000)) {
        return fail("UdpComm should reject an invalid IPv4 address.");
    }

    tcp_comm.disconnect();
    std::cout << "Steel SDK smoke test passed." << std::endl;
    return 0;
}
