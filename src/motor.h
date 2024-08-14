#ifndef MOTOR_H
#define MOTOR_H
#include "serial_comm.h"
#include <string>

using namespace std;

class Motor
{
private:
    bool debug;
    SerialComm serial;
public:
    Motor(int myport, bool mydebug = false, string mydevice = "/dev/ttymxc3");
    void set_speed(int speed);
    void send_msg(string msg, bool wait = false);
    void waitForEnd();
    void dataflush();
};

#endif