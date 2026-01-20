#ifndef MOTOR_H
#define MOTOR_H

#include "comm_interface.h"
#include <string>

using namespace std;

class Motor
{
private:
    bool debug;
    CommInterface* comm;
public:
    Motor(CommInterface* commPtr, int devicePort, bool debugFlag = false);
    void set_speed(int speed);
    void send_msg(string msg, bool wait = false);
    void waitForEnd();
    void load(int speed, int offset = 0);
    string read_status();
};

#endif