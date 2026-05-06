#pragma once

#include "comm_interface.h"
#include <chrono>
#include <string>

class DeviceSession {
public:
    DeviceSession(
        CommInterface& comm,
        int device_port,
        std::string address_separator = "",
        std::chrono::milliseconds default_timeout = std::chrono::milliseconds(1000));

    void setDefaultTimeout(std::chrono::milliseconds timeout);
    std::chrono::milliseconds defaultTimeout() const;

    int devicePort() const;
    std::string buildCommand(const std::string& command) const;

    void send(const std::string& command);
    void sendRaw(const std::string& payload);

    bool waitForData();
    bool waitForData(std::chrono::milliseconds timeout);

    std::string readLine();
    std::string readLine(std::chrono::milliseconds timeout);
    std::string readAvailable();

    std::string sendAndWait(const std::string& command);
    std::string sendAndWait(const std::string& command, std::chrono::milliseconds timeout);
    std::string sendRawAndWait(const std::string& payload);
    std::string sendRawAndWait(const std::string& payload, std::chrono::milliseconds timeout);

private:
    void ensureReadable(std::chrono::milliseconds timeout, const std::string& context);

    CommInterface* comm_;
    int device_port_;
    std::string address_separator_;
    std::chrono::milliseconds default_timeout_;
};
