#include "serial_comm.h"
#include <iostream>
#include <string>


SerialComm::SerialComm(std::string mydevice, int myport) : port(std::to_string(myport))
{
    fd = CreateFile(mydevice.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (fd == INVALID_HANDLE_VALUE)
    {
        std::cerr << "open_port: Unable to open serial port" << std::endl;
        throw std::runtime_error("Unable to open serial port");
    }
    configurePort();
}

SerialComm::~SerialComm()
{
    CloseHandle(fd);
}

void SerialComm::configurePort()
{
    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(fd, &dcbSerialParams))
    {
        std::cerr << "Error getting state" << std::endl;
        return;
    }
    dcbSerialParams.BaudRate = CBR_38400;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    if (!SetCommState(fd, &dcbSerialParams))
    {
        std::cerr << "Error setting state" << std::endl;
        return;
    }

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    SetCommTimeouts(fd, &timeouts);
}

void SerialComm::init()
{
    // Any additional initialization can be done here if needed
}

std::string SerialComm::readLine()
{
    char buffer[500] = {0};
    DWORD bytesRead;
    char tmp;
    int num = 0;

    while (true)
    {
        if (dataAvailable())
        {
            if (!ReadFile(fd, &tmp, 1, &bytesRead, NULL))
            {
                std::cerr << "Error reading from serial port" << std::endl;
                break;
            }
            if (bytesRead == 0)
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
            Sleep(100); // Sleep for 100 milliseconds
        }
    }
    return std::string(buffer);
}

void SerialComm::writeData(const std::string &data)
{
    std::string msg;
    if (port.length() == 1)
        msg = "@0" + port + data + "\r\n"; // Append newline if necessary
    else 
        msg = "@" + port + data + "\r\n"; // Append newline if necessary
    
    DWORD bytesWritten;
    if (!WriteFile(fd, msg.c_str(), msg.length(), &bytesWritten, NULL))
    {
        std::cerr << "Error writing to serial port" << std::endl;
    }
    else
    {
        std::cout << msg << std::endl;
    }
}

bool SerialComm::dataAvailable()
{
    COMSTAT comStat;
    DWORD errors;

    if (!ClearCommError(fd, &errors, &comStat))
    {
        std::cerr << "Error: ClearCommError failed" << std::endl;
        return false;
    }
    return comStat.cbInQue > 0;
}

void SerialComm::flush()
{
    PurgeComm(fd, PURGE_RXCLEAR | PURGE_TXCLEAR);
}

int main()
{
    try
    {
        SerialComm serial("COM1", 1);
        serial.init();
        serial.flush();

        while (true)
        {
            if (serial.dataAvailable())
            {
                std::string line = serial.readLine();
                std::cout << "Received: " << line << std::endl;
            }
            Sleep(100); // Sleep for 100 milliseconds
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
