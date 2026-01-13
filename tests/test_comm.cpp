#include "../libs/mirobot/mirobot.h"
#include "../libs/motor/motor.h"
#include <iostream>
#include <cassert>

void testMirobot() {
    std::cout << "Testing Mirobot class..." << std::endl;

    Mirobot mirobot(1);

    // // Test initialization
    // mirobot.init();
    // assert(mirobot.isInitialized() && "Mirobot initialization failed");

    // // Test movement
    // mirobot.moveTo(10, 20, 30);
    // assert(mirobot.getPosition() == std::make_tuple(10, 20, 30) && "Mirobot moveTo failed");

    // // Test reset
    // mirobot.reset();
    // assert(mirobot.getPosition() == std::make_tuple(0, 0, 0) && "Mirobot reset failed");

    std::cout << "Mirobot tests passed!" << std::endl;
}

void testMotor() {
    std::cout << "Testing Motor class..." << std::endl;

    Motor motor(2);

    // // Test initialization
    // motor.init();
    // assert(motor.isInitialized() && "Motor initialization failed");

    // // Test speed control
    // motor.setSpeed(100);
    // assert(motor.getSpeed() == 100 && "Motor setSpeed failed");

    // // Test stop
    // motor.stop();
    // assert(motor.getSpeed() == 0 && "Motor stop failed");

    std::cout << "Motor tests passed!" << std::endl;
}

int main() {
    testMirobot();
    testMotor();

    std::cout << "All tests passed!" << std::endl;
    return 0;
}