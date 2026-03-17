#pragma once

#include "comm_interface.h"
#include <string>

class TcpComm : public CommInterface {
public:
    // 1. 获取全局唯一单例 (C++11 局部静态变量，初始化是安全的)
    static TcpComm& getInstance() {
        static TcpComm instance;
        return instance;
    }

    // 2. 禁用拷贝构造函数和赋值操作符，防止单例被意外复制
    TcpComm(const TcpComm&) = delete;
    TcpComm& operator=(const TcpComm&) = delete;

    // 3. 网络连接控制方法
    bool connectToServer(const std::string& target_ip, int target_port);
    void disconnect();

    // 4. 实现 CommInterface 接口
    void writeData(const std::string &data) override;
    std::string readLine() override;
    bool dataAvailable() override;

private:
    // 5. 构造和析构函数私有化
    TcpComm();
    ~TcpComm();

    std::string ip;
    int port;
    int sock_fd;
};