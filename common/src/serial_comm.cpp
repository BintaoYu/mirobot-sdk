#include "serial_comm.h"
#include <fcntl.h>
#include <termios.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/ioctl.h>

SerialComm::SerialComm(std::string device, int baudRate) {
    port = device;
    fd = open(device.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        std::cerr << "Error opening serial port: " << strerror(errno) << std::endl;
        return;
    }
    configurePort();
}

SerialComm::~SerialComm() {
    close(fd);
}

void SerialComm::init() {
    configurePort();
}

void SerialComm::configurePort() {
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        std::cerr << "Error getting terminal attributes: " << strerror(errno) << std::endl;
        return;
    }

    cfsetospeed(&tty, B38400);
    cfsetispeed(&tty, B38400);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit chars
    tty.c_iflag &= ~IGNBRK;                     // disable break processing
    tty.c_lflag = 0;                            // no signaling chars, no echo, no canonical processing
    tty.c_oflag = 0;                            // no remapping, no delays
    tty.c_cc[VMIN] = 1;                         // read doesn't block
    tty.c_cc[VTIME] = 1;                        // 0.1 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

    tty.c_cflag |= (CLOCAL | CREAD);    // ignore modem controls, enable reading
    tty.c_cflag &= ~(PARENB | PARODD); // shut off parity
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        std::cerr << "Error setting terminal attributes: " << strerror(errno) << std::endl;
    }
}

std::string SerialComm::readLine() {
    std::string result;
    char buf;
    while (true) {
        int n = read(fd, &buf, 1);
        if (n < 0) {
            std::cerr << "Error reading from serial port: " << strerror(errno) << std::endl;
            break;
        }
        if (buf == '\n') {
            break;
        }
        result += buf;
    }
    return result;
}

void SerialComm::writeData(const std::string &data) {
    int n = write(fd, data.c_str(), data.size());
    if (n < 0) {
        std::cerr << "Error writing to serial port: " << strerror(errno) << std::endl;
    }
}

bool SerialComm::dataAvailable() {
    int bytes;
    if (ioctl(fd, FIONREAD, &bytes) == -1) {
        std::cerr << "Error checking data availability: " << strerror(errno) << std::endl;
        return false;
    }
    return bytes > 0;
}

void SerialComm::flush() {
    tcflush(fd, TCIOFLUSH);
}
