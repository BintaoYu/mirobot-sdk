#pragma once
#include <string>

class CommInterface {
public:
    virtual ~CommInterface() {};
    virtual void writeData(const std::string &data) = 0;
    virtual std::string readLine() = 0;
    virtual bool dataAvailable() = 0;
};