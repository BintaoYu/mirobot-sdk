#pragma once

#include "comm_interface.h"
#include <cstdint>
#include <string>

class SerialComm : public CommInterface {
private:
    std::string port;
    void configurePort();
    std::intptr_t handle;
public:
    explicit SerialComm(std::string port);
    ~SerialComm() override;
    bool isOpen() const;
    void close() noexcept;
    void writeData(const std::string &data) override;
    std::string readLine() override;
    bool dataAvailable() override;
    void flush();
};
