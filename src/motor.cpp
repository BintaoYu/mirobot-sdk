#include "motor.h"
#include <string>
#include <iostream>
#include <unistd.h>

void Motor::send_msg(string msg, bool wait) {
    dataflush();
    while (serial.dataAvailable())
        serial.flush();
    serial.writeData(msg);
    if (debug)
        printf("Message sent: %s", msg.c_str());
    if (wait)
    {
        printf("Waiting to receive: %s", msg.c_str());
        waitForEnd();
        cout << read_status() << endl;
    }
}

string Motor::read_status() {
    string read_status = "";
    while (serial.dataAvailable())
    {
        read_status += serial.readLine();
    }
    return read_status;
}


void Motor::waitForEnd() {
    while (!serial.dataAvailable())
        usleep(1000);
}

void Motor::set_speed(int speed) {
    string msg = "G6 F" + to_string(speed);
    send_msg(msg, true);
}

void Motor::load(int speed, int offset) {
    string msg = "$N2=G06 F0" + to_string(offset);
    send_msg(msg, true);
    set_speed(speed);
}


void Motor::dataflush() {
    while (serial.dataAvailable())
    {
        serial.flush();
        sleep(1);
    }
}

Motor::Motor(int myport, bool mydebug, string mydevice) : serial(mydevice, myport) {
    debug = mydebug;
    serial.init();
    // waitForEnd();
    dataflush();
}