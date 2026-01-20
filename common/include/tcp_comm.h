#pragma once

#include "comm_interface.h"
#include <string>

class TcpComm : public CommInterface {
private:
    std::string ip;
    int port;
    int sock_fd;
public:
    TcpComm(std::string ip, int port);
    ~TcpComm();
    void writeData(const std::string &data) override;
    std::string readLine() override;
    bool dataAvailable() override;
};