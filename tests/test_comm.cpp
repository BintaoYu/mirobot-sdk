#include "mirobot.h"
#include "motor.h"
#include "serial_comm.h"
#include "tcp_comm.h"
#include <iostream>
#include <cassert>

void testMirobot() {
    std::cout << "Testing Mirobot class..." << std::endl;
    CommInterface* comm = new SerialComm("/dev/ttyS2");
    CommInterface* comm_tcp = new TcpComm("192.168.31.32", 8080);
    Mirobot mirobot(comm_tcp, 1);
    mirobot.home_simultaneous();
    std::cout << "Mirobot tests passed!" << std::endl;
}

void testMotor() {
    std::cout << "Testing Motor class..." << std::endl;
    CommInterface* comm = new SerialComm("/dev/ttyS3");
    CommInterface* comm_tcp = new TcpComm("192.168.31.32", 8080);
    Motor motor(comm_tcp, 2);
    motor.set_speed(100);
    std::cout << "Motor tests passed!" << std::endl;
}

int main() {
    testMirobot();
    // testMotor();

    std::cout << "All tests passed!" << std::endl;
    return 0;
}