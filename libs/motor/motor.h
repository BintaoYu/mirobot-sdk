#ifndef MOTOR_H
#define MOTOR_H

#include "device_session.h"
#include <chrono>
#include <string>

class Motor {
private:
    bool debug;
    DeviceSession session_;

public:
    explicit Motor(DeviceSession session, bool debugFlag = false);
    Motor(
        CommInterface& comm,
        int devicePort,
        bool debugFlag = false,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(1000));

    void set_speed(int speed);
    void send_msg(const std::string& msg, bool wait = false);
    bool waitForEnd(std::chrono::milliseconds timeout = std::chrono::milliseconds(1000));
    void load(int speed, int offset = 0);
    std::string read_status();
};

#endif
