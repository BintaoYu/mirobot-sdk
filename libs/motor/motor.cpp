#include "motor.h"
#include "shared_udp_comm.h"
#include <chrono>
#include <cstdio>
#include <exception>
#include <iostream>
#include <string>
#include <thread>
#include <utility>

namespace {

void report_command_failure(const std::string& command, const std::exception& error) {
    std::cerr << "Motor command failed: " << command << ": " << error.what() << std::endl;
}

void report_unknown_command_failure(const std::string& command) {
    std::cerr << "Motor command failed: " << command << ": unknown error" << std::endl;
}

} // namespace

Motor::Motor(DeviceSession session, bool debugFlag)
    : debug(debugFlag), owned_comm_(), session_(std::move(session)) {
}

Motor::Motor(CommInterface& comm, int devicePort, bool debugFlag, std::chrono::milliseconds timeout)
    : debug(debugFlag), owned_comm_(), session_(comm, devicePort, " ", timeout) {
}

Motor::Motor(
    const std::string& targetIp,
    int targetPort,
    int devicePort,
    bool debugFlag,
    std::chrono::milliseconds timeout)
    : debug(debugFlag),
      owned_comm_(acquireSharedUdpComm(targetIp, targetPort)),
      session_(*owned_comm_, devicePort, " ", timeout) {
}

void Motor::send_msg(const std::string& msg, bool wait) {
    const std::string full_msg = session_.buildCommand(msg);
    if (debug) {
        std::printf("Message sent: %s\n", full_msg.c_str());
    }

    try {
        if (wait) {
            const std::string response = session_.sendAndWait(msg);
            std::cout << "receive:" << response << std::endl;
            return;
        }

        session_.send(msg);
    } catch (const std::exception& error) {
        report_command_failure(full_msg, error);
    } catch (...) {
        report_unknown_command_failure(full_msg);
    }
}

std::string Motor::read_status() {
    return session_.readAvailable();
}

bool Motor::waitForEnd(std::chrono::milliseconds timeout) {
    return session_.waitForData(timeout);
}

void Motor::set_speed(int speed) {
    const std::string msg = "G6 F" + std::to_string(speed);
    send_msg(msg, true);
}

void Motor::load(int speed, int offset) {
    (void)offset;
    const std::string msg = "$N2=G06 F0";
    send_msg(msg, true);
    set_speed(speed);
}
