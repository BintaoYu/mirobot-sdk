#ifndef SERIAL_COMM_H
#define SERIAL_COMM_H

#include <string>
#include <unistd.h>

class SerialComm
{
public:
    int fd;
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