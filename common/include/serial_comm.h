#pragma once

#include "comm_interface.h"
#include <cstdint>
#include <string>

class SerialComm : public CommInterface {
private:
    std::string port;
    void configurePort();
    bool isOpen() const;
    std::intptr_t handle;
public:
    explicit SerialComm(std::string port);
    ~SerialComm() override;
    void writeData(const std::string &data) override;
    std::string readLine() override;
    bool dataAvailable() override;
    void flush();
};
