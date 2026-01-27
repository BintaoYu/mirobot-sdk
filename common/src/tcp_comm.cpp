#include "tcp_comm.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

TcpComm::TcpComm(std::string ip, int port) : ip(ip), port(port), sock_fd(-1) {
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Socket creation failed");
        return;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);

    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to server failed");
        close(sock_fd);
        sock_fd = -1;
    }
}

TcpComm::~TcpComm() {
    if (sock_fd >= 0)
        close(sock_fd);
}

void TcpComm::writeData(const std::string &data) {
    if (sock_fd < 0) return;

    std::string final_data = data;
    
    // 检查字符串是否为空，且最后一个字符是否为换行符
    if (final_data.empty() || final_data.back() != '\n') {
        final_data += '\n';
    }

    // 发送处理后的完整数据
    ssize_t n = send(sock_fd, final_data.c_str(), final_data.size(), MSG_NOSIGNAL);
    
    if (n < 0) {
        perror("TCP Send Error");
    }
}

std::string TcpComm::readLine() {
    if (sock_fd < 0) return "";
    std::string result;
    char c;
    // TCP 是流，必须逐字节判断直到换行符，才能保证“行”的语义
    while (recv(sock_fd, &c, 1, 0) > 0) {
        if (c == '\n') break;
        if (c != '\r') result += c; // 兼容 Windows 的 \r\n
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
