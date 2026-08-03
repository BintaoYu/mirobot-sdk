#include "mirobot.h"
#include <iostream>
#include <string>

int main() {
    Mirobot mirobot1("192.168.31.44", 8000, 1, true);
    Mirobot mirobot2("192.168.31.44", 8000, 2, true);
    Mirobot mirobot3("192.168.31.44", 8000, 3, true);
    mirobot1.home_simultaneous();
    mirobot2.home_simultaneous();
    mirobot3.home_simultaneous();
    return 0;
}
