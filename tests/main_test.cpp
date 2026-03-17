#include "mirobot.h"
#include "motor.h"
#include "serial_comm.h"
#include "tcp_comm.h"
#include <iostream>
#include <cassert>



int main() {
    TcpComm& tcp_comm = TcpComm::getInstance();
    // CommInterface* comm_tcp = new TcpComm("127.0.0.1", 8000);
    // comm_tcp->writeData("@10 G6 F0");
    // comm_tcp->writeData("@2 $H\n");
    ////////////////init时需要判断是否连接成功////////////////////////
    // if (!tcp_comm.connectToServer("192.168.31.120", 8000)) {
    //     std::cerr << "TCP 连接失败，程序退出！" << std::endl;
    //     return -1;
    // }
    // std::cout << "TCP 连接成功！" << std::endl;
    ///////////////////////////////////////////////////////////
    Motor motor(&tcp_comm, 10, true);
    motor.set_speed(1000);
    sleep(3);
    motor.set_speed(0);
    return 0;
}