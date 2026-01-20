#pragma once

#include "comm_interface.h"
#include <string>

class SerialComm : public CommInterface {
private:
    std::string port;
    void configurePort();
    int fd;
public:
    SerialComm(std::string port);
    ~SerialComm();
    void writeData(const std::string &data) override;
    std::string readLine() override;
    bool dataAvailable() override;
    void flush();
};