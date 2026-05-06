#pragma once

#include "comm_interface.h"
#include <cstdint>
#include <string>

class UdpComm : public CommInterface {
public:
    UdpComm();
    ~UdpComm() override;

    UdpComm(const UdpComm&) = delete;
    UdpComm& operator=(const UdpComm&) = delete;
    UdpComm(UdpComm&&) = delete;
    UdpComm& operator=(UdpComm&&) = delete;

    bool connect(const std::string& target_ip, int target_port);
    bool connectToServer(const std::string& target_ip, int target_port);
    void disconnect();
    bool isConnected() const;

    void writeData(const std::string& data) override;
    std::string readLine() override;
    bool dataAvailable() override;

private:
    std::string ip_;
    int port_;
    std::intptr_t sock_fd_;
    std::string read_buffer_;
};
