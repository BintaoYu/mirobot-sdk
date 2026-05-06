#include "motor.h"
#include <chrono>
#include <iostream>
#include <stdexcept>

int main() {
    try {
        const char* ip = "192.168.31.120";
        const int port = 8000;
        const int device_port = 10;
        const int run_speed = 300;
        const auto timeout = std::chrono::milliseconds(1000);

        Motor motor(ip, port, device_port, true, timeout);
        motor.set_speed(0);
        motor.set_speed(run_speed);
        motor.set_speed(0);

        std::cout << "motor status: " << motor.read_status() << std::endl;
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "steel_test failed: " << ex.what() << std::endl;
        return 1;
    }
}
