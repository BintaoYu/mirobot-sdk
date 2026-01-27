#include "mirobot.h"
#include "motor.h"
#include "serial_comm.h"
#include "tcp_comm.h"
#include <iostream>
#include <cassert>

int main() {
    CommInterface* comm_tcp = new TcpComm("192.168.31.120", 8000);
    // CommInterface* comm_tcp = new TcpComm("127.0.0.1", 8000);
    // comm_tcp->writeData("@10 G6 F0");
    // comm_tcp->writeData("@2 $H\n");
    Motor motor(comm_tcp, 10, true);
    motor.set_speed(1000);
    sleep(3);
    motor.set_speed(0);
    delete comm_tcp;
    return 0;
}