#pragma once

#include "comm_interface.h"
#include <cstdint>
#include <string>

class TcpServerComm : public CommInterface {
public:
    TcpServerComm();
    ~TcpServerComm() override;

    TcpServerComm(const TcpServerComm&) = delete;
    TcpServerComm& operator=(const TcpServerComm&) = delete;
    TcpServerComm(TcpServerComm&&) = delete;
    TcpServerComm& operator=(TcpServerComm&&) = delete;

    bool listen(int listen_port);
    bool listen(const std::string& bind_ip, int listen_port, int backlog = 1);
    bool acceptClient(int timeout_ms = -1);
    void disconnectClient();
    void stop();
    bool isListening() const;
    bool hasClient() const;
    int port() const;

    void writeData(const std::string& data) override;
    std::string readLine() override;
    bool dataAvailable() override;

private:
    std::string bind_ip;
    int listen_port;
    std::intptr_t listen_fd;
    std::intptr_t client_fd;
};
