#pragma once

#include "comm_interface.h"
#include <cstdint>
#include <string>

class TcpComm : public CommInterface {
public:
    TcpComm();
    ~TcpComm() override;

    TcpComm(const TcpComm&) = delete;
    TcpComm& operator=(const TcpComm&) = delete;
    TcpComm(TcpComm&&) = delete;
    TcpComm& operator=(TcpComm&&) = delete;

    bool connect(const std::string& target_ip, int target_port);
    bool connectToServer(const std::string& target_ip, int target_port);
    void disconnect();
    bool isConnected() const;

    void writeData(const std::string &data) override;
    std::string readLine() override;
    bool dataAvailable() override;

private:
    std::string ip;
    int port;
    std::intptr_t sock_fd;
};
