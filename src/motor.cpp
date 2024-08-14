#include "motor.h"
#include <string>
#include <iostream>
#include <unistd.h>
void Motor::send_msg(string msg, bool wait)
{
    // dataflush();
    // while (serial.dataAvailable())
    //     serial.flush();
    serial.writeData(msg);
    if (debug)
        printf("Message sent: %s", msg.c_str());
    if (wait)
    {
        printf("Waiting to receive: %s", msg.c_str());
        waitForEnd();
        // cout << read_status() << endl;
    }
}

void Motor::waitForEnd()
{
    while (!serial.dataAvailable())
        usleep(1000);
}

void Motor::set_speed(int speed)
{
    string msg = "G6 F" + to_string(speed);
    send_msg(msg, false);
}

void Motor::dataflush()
{
    while (serial.dataAvailable())
    {
        serial.flush();
        sleep(1);
    }
}

Motor::Motor(int myport, bool mydebug, string mydevice) : serial(mydevice, myport)
{
    debug = mydebug;
    serial.init();
    // waitForEnd();
    dataflush();
}