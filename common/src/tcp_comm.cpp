#include "tcp_comm.h"
#include <cstring>
#include <iostream>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace {

#ifdef _WIN32
constexpr std::intptr_t kInvalidSocketValue = static_cast<std::intptr_t>(INVALID_SOCKET);
using NativeSocket = SOCKET;

NativeSocket nativeSocket(std::intptr_t socket_value) {
    return static_cast<NativeSocket>(socket_value);
}

class WinSockRuntime {
public:
    WinSockRuntime() : initialized_(false) {
        WSADATA data = {};
        const int result = WSAStartup(MAKEWORD(2, 2), &data);
        if (result == 0) {
            initialized_ = true;
        } else {
            std::cerr << "WSAStartup failed with code " << result << std::endl;
        }
    }

    ~WinSockRuntime() {
        if (initialized_) {
            WSACleanup();
        }
    }

    bool ready() const {
        return initialized_;
    }

private:
    bool initialized_;
};

int lastSocketError() {
    return WSAGetLastError();
}

void closeSocket(std::intptr_t& socket_value) {
    if (socket_value == kInvalidSocketValue) {
        return;
    }

    closesocket(nativeSocket(socket_value));
    socket_value = kInvalidSocketValue;
}
#else
constexpr std::intptr_t kInvalidSocketValue = -1;
using NativeSocket = int;

NativeSocket nativeSocket(std::intptr_t socket_value) {
    return static_cast<NativeSocket>(socket_value);
}

int lastSocketError() {
    return errno;
}

void closeSocket(std::intptr_t& socket_value) {
    if (socket_value == kInvalidSocketValue) {
        return;
    }

    close(nativeSocket(socket_value));
    socket_value = kInvalidSocketValue;
}
#endif

int sendFlags() {
#ifdef _WIN32
    return 0;
#else
    return MSG_NOSIGNAL;
#endif
}

bool parseIpv4Address(const std::string& ip, in_addr& out_address) {
#ifdef _WIN32
#if defined(__MINGW32__)
    const unsigned long address_value = inet_addr(ip.c_str());
    if (address_value == INADDR_NONE && ip != "255.255.255.255") {
        return false;
    }
    out_address.s_addr = address_value;
    return true;
#else
    return InetPtonA(AF_INET, ip.c_str(), &out_address) == 1;
#endif
#else
    return inet_pton(AF_INET, ip.c_str(), &out_address) == 1;
#endif
}

} // namespace

TcpComm::TcpComm() : ip(""), port(0), sock_fd(kInvalidSocketValue) {}

TcpComm::~TcpComm() {
    disconnect();
}

bool TcpComm::isConnected() const {
    return sock_fd != kInvalidSocketValue;
}

bool TcpComm::connect(const std::string& target_ip, int target_port) {
    if (isConnected()) {
        return true;
    }

#ifdef _WIN32
    static WinSockRuntime win_sock_runtime;
    if (!win_sock_runtime.ready()) {
        return false;
    }
#endif

    this->ip = target_ip;
    this->port = target_port;

    const NativeSocket socket_handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#ifdef _WIN32
    if (socket_handle == INVALID_SOCKET) {
#else
    if (socket_handle < 0) {
#endif
        std::cerr << "Socket creation failed: " << lastSocketError() << std::endl;
        return false;
    }
    sock_fd = static_cast<std::intptr_t>(socket_handle);

    sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(static_cast<uint16_t>(port));
    if (!parseIpv4Address(ip, server_addr.sin_addr)) {
        std::cerr << "Invalid IPv4 address: " << ip << std::endl;
        closeSocket(sock_fd);
        return false;
    }

    if (::connect(nativeSocket(sock_fd), reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) != 0) {
        std::cerr << "Connection to server failed: " << lastSocketError() << std::endl;
        closeSocket(sock_fd);
        return false;
    }
    return true;
}

bool TcpComm::connectToServer(const std::string& target_ip, int target_port) {
    return connect(target_ip, target_port);
}

void TcpComm::disconnect() {
    closeSocket(sock_fd);
}

void TcpComm::writeData(const std::string &data) {
    if (!isConnected()) {
        return;
    }

    std::string final_data = data;
    if (final_data.empty() || final_data.back() != '\n') {
        final_data += '\n';
    }

    const char* cursor = final_data.c_str();
    std::size_t remaining = final_data.size();

    while (remaining > 0) {
#ifdef _WIN32
        const int sent = send(nativeSocket(sock_fd), cursor, static_cast<int>(remaining), sendFlags());
        if (sent == SOCKET_ERROR) {
#else
        const ssize_t sent = send(nativeSocket(sock_fd), cursor, remaining, sendFlags());
        if (sent < 0) {
#endif
            std::cerr << "TCP send error: " << lastSocketError() << std::endl;
            closeSocket(sock_fd);
            return;
        }
        if (sent == 0) {
            std::cerr << "TCP send returned 0 bytes" << std::endl;
            closeSocket(sock_fd);
            return;
        }

        remaining -= static_cast<std::size_t>(sent);
        cursor += sent;
    }
}

std::string TcpComm::readLine() {
    if (!isConnected()) {
        return "";
    }
    
    std::string result;
    char c;

    while (true) {
#ifdef _WIN32
        const int received = recv(nativeSocket(sock_fd), &c, 1, 0);
        if (received == SOCKET_ERROR || received == 0) {
            closeSocket(sock_fd);
            break;
        }
#else
        const ssize_t received = recv(nativeSocket(sock_fd), &c, 1, 0);
        if (received <= 0) {
            closeSocket(sock_fd);
            break;
        }
#endif
        if (c == '\n') break;
        if (c != '\r') result += c; 
    }
    return result;
}

bool TcpComm::dataAvailable() {
    if (!isConnected()) {
        return false;
    }
    
    fd_set read_fds;
    timeval timeout;
    FD_ZERO(&read_fds);
    FD_SET(nativeSocket(sock_fd), &read_fds);
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

#ifdef _WIN32
    const int ret = select(0, &read_fds, nullptr, nullptr, &timeout);
#else
    const int ret = select(nativeSocket(sock_fd) + 1, &read_fds, nullptr, nullptr, &timeout);
#endif

    return (ret > 0 && FD_ISSET(nativeSocket(sock_fd), &read_fds));
}
