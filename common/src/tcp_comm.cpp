#include "tcp_comm.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>

// 私有构造函数，仅初始化成员变量
TcpComm::TcpComm() : ip(""), port(0), sock_fd(-1) {}

TcpComm::~TcpComm() {
    disconnect();
}

bool TcpComm::connectToServer(const std::string& target_ip, int target_port) {
    if (sock_fd >= 0) {
        return true; // 已经连接
    }

    this->ip = target_ip;
    this->port = target_port;

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Socket creation failed");
        return false;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);

    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to server failed");
        close(sock_fd);
        sock_fd = -1;
        return false;
    }
    return true;
}

void TcpComm::disconnect() {
    if (sock_fd >= 0) {
        close(sock_fd);
        sock_fd = -1;
    }
}

void TcpComm::writeData(const std::string &data) {
    if (sock_fd < 0) return;

    std::string final_data = data;
    if (final_data.empty() || final_data.back() != '\n') {
        final_data += '\n';
    }

    ssize_t n = send(sock_fd, final_data.c_str(), final_data.size(), MSG_NOSIGNAL);
    if (n < 0) {
        perror("TCP Send Error");
    }
}

std::string TcpComm::readLine() {
    if (sock_fd < 0) return "";
    
    std::string result;
    char c;
    while (recv(sock_fd, &c, 1, 0) > 0) {
        if (c == '\n') break;
        if (c != '\r') result += c; 
    }
    return result;
}

bool TcpComm::dataAvailable() {
    if (sock_fd < 0) return false;
    
    fd_set read_fds;
    struct timeval timeout;
    FD_ZERO(&read_fds);
    FD_SET(sock_fd, &read_fds);
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    
    int ret = select(sock_fd + 1, &read_fds, NULL, NULL, &timeout);
    return (ret > 0 && FD_ISSET(sock_fd, &read_fds));
}