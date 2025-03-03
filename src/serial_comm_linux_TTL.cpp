#include "serial_comm.h"
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <iostream>
#include <cstring>
#include <cerrno>
#include <sys/ioctl.h>

SerialComm::SerialComm(std::string mydevice, int myport) : port(std::to_string(myport))
{
    fd = open(mydevice.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1)
    {
        perror("open_port: Unable to open serial port");
        throw std::runtime_error("Unable to open serial port");
    }
    fcntl(fd, F_SETFL, 0);
    configurePort();
}

SerialComm::~SerialComm()
{
    close(fd);
}

void SerialComm::configurePort()
{
    struct termios options;
    tcgetattr(fd, &options);

    // Set baud rate to 115200
    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);

    // Configure data bits, sto0p bits, and parity
    options.c_cflag &= ~PARENB;  // No parity
    options.c_cflag &= ~CSTOPB;  // 1 stop bit
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;      // 8 data bits

    // Enable the receiver and set local mode
    options.c_cflag |= (CLOCAL | CREAD);

    // Disable hardware flow control
    options.c_cflag &= ~CRTSCTS;

    // Set raw input and output mode
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // Raw input
    options.c_oflag &= ~OPOST;                         // Raw output
    options.c_cc[VTIME] = 0;
    options.c_cc[VMIN] = 4;
    
    // Apply the configuration immediately
    tcsetattr(fd, TCSANOW, &options);

    // // RS485 configuration
    // struct serial_rs485 rs485conf;
    // memset(&rs485conf, 0, sizeof(rs485conf));

    // rs485conf.flags = SER_RS485_ENABLED | SER_RS485_RTS_ON_SEND;
    // rs485conf.delay_rts_before_send = 4;  // Delay before send (microseconds)
    // rs485conf.delay_rts_after_send = 4;   // Delay after send (microseconds)

    // if (ioctl(fd, TIOCSRS485, &rs485conf) < 0) {
    //     std::cerr << "Error configuring RS485: " << strerror(errno) << std::endl;
    // }
}

void SerialComm::init()
{
    // Any additional initialization can be done here if needed
}


std::string SerialComm::readLine()
{
    char buffer[500] = {0};
    int num = 0;
    char tmp;
    while (true)
    {
        if (dataAvailable())
        {
            int bytesRead = read(fd, &tmp, 1);
            if (bytesRead == -1)
            {
                std::cerr << "Error reading from serial port: " << strerror(errno) << std::endl;
                break;
            }
            else if (bytesRead == 0)
            {
                break;
            }

            if (num == 300)
            {
                std::cerr << "cross the border\n";
                break;
            }
            else if (tmp != '\n')
            {
                buffer[num++] = tmp;
            }
            else
            {
                break;
            }
        }
        else
        {
            usleep(100000); // Sleep for 100 milliseconds
        }
    }
    return std::string(buffer);
}

void SerialComm::writeData(const std::string &data)
{
    std::string msg;
    if (port.length() == 1)
        msg = data + "\r\n"; // Append newline if necessary
    ssize_t bytesWritten = write(fd, msg.c_str(), msg.length());
    if (bytesWritten == -1) {
        std::cerr << "Error writing to serial port: " << strerror(errno) << std::endl;
    } else {
        std::cout << msg << std::endl;
    }
}

bool SerialComm::dataAvailable()
{
    int bytesAvailable;
    if (ioctl(fd, FIONREAD, &bytesAvailable) == -1)
    {
        std::cerr << "Error: ioctl failed: " << strerror(errno) << std::endl;
        return false;
    }
    return bytesAvailable > 0;
}


void SerialComm::flush()
{
    tcflush(fd, TCIOFLUSH);
}

int main()
{
    try
    {
        SerialComm serial("/dev/ttyS0", 1);
        serial.init();
        serial.flush();

        while (true)
        {
            if (serial.dataAvailable())
            {
                std::string line = serial.readLine();
                std::cout << "Received: " << line << std::endl;
            }
            usleep(100000); // Sleep for 100 milliseconds
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
