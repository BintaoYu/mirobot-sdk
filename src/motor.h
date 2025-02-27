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
    Motor(int myport, bool mydebug = false, string mydevice = "/dev/ttyS2");
    void set_speed(int speed);
    void send_msg(string msg, bool wait = false);
    void waitForEnd();
    void load(int speed, int offset = 0);
    string read_status();
    void dataflush();
};

#endif