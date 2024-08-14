#ifndef SERIAL_COMM_H
#define SERIAL_COMM_H

#include <string>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#else
#include <unistd.h>
#endif

class SerialComm
{
public:
#ifdef PLATFORM_WINDOWS
    HANDLE fd;
#else
    int fd;
#endif
    SerialComm(std::string device, int myport);
    ~SerialComm();
    void init();
    std::string readLine();
    void writeData(const std::string &data);
    bool dataAvailable();
    void flush();
private:
    std::string port;
    void configurePort();
};

#endif // SERIAL_COMM_H
