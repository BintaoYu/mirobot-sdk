#include "motor.h"
#include <string>
#include <iostream>
#include <unistd.h>

Motor::Motor(CommInterface* commPtr, int devicePort, bool debugFlag) : comm(commPtr), debug(debugFlag) {
    if (commPtr == nullptr)
        throw std::invalid_argument("CommInterface pointer cannot be null");
}

void Motor::send_msg(string msg, bool wait) {
    comm->writeData(msg);
    if (debug) printf("Message sent: %s", msg.c_str());
    if (wait) {
        printf("Waiting to receive: %s", msg.c_str());
        waitForEnd();
        cout << read_status() << endl;
    }
}

string Motor::read_status() {
    string status = "";
    while (comm->dataAvailable()){
        status += comm->readLine();
    }
    return status;
}


void Motor::waitForEnd() {
    int timeout = 2000; // 2000 ms timeout
    while (!comm->dataAvailable() && timeout-- > 0)
        usleep(1000);
}

void Motor::set_speed(int speed) {
    string msg = "G6 F" + to_string(speed);
    send_msg(msg, true);
}

void Motor::load(int speed, int offset) {
    string msg = "$N2=G06 F0";
    send_msg(msg, true);
    set_speed(speed);
}