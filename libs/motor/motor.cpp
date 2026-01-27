#include "motor.h"
#include <string>
#include <iostream>
#include <unistd.h>

Motor::Motor(CommInterface* commPtr, int devicePort, bool debugFlag) : comm(commPtr), device_port(devicePort), debug(debugFlag) {
    if (commPtr == nullptr)
        throw std::invalid_argument("CommInterface pointer cannot be null");
}

void Motor::send_msg(string msg, bool wait) {
    string full_msg = "@" + to_string(device_port) + " " + msg;
    comm->writeData(full_msg);
    if (debug) printf("Message sent: %s\n", full_msg.c_str());
    if (wait) {
        printf("Waiting \n");
        waitForEnd();
        cout << "receive:" << read_status() << endl;
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
    while (!comm->dataAvailable())
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