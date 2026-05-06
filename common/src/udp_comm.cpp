#include "udp_comm.h"
#include <cstring>
#include <iostream>
#include <utility>

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

std::string trimTrailingLineEndings(std::string text) {
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) {
        text.pop_back();
    }

    return text;
}

bool extractBufferedLine(std::string& buffer, std::string& line) {
    const std::size_t newline_pos = buffer.find('\n');
    if (newline_pos == std::string::npos) {
        return false;
    }

    line = buffer.substr(0, newline_pos);
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }

    buffer.erase(0, newline_pos + 1);
    return true;
}

bool isValidPort(int port) {
    return port > 0 && port <= 65535;
}

} // namespace

UdpComm::UdpComm() : ip_(), port_(0), sock_fd_(kInvalidSocketValue), read_buffer_() {}

UdpComm::~UdpComm() {
    disconnect();
}

bool UdpComm::isConnected() const {
    return sock_fd_ != kInvalidSocketValue;
}

bool UdpComm::connect(const std::string& target_ip, int target_port) {
    if (!isValidPort(target_port)) {
        std::cerr << "Invalid UDP port: " << target_port << std::endl;
        return false;
    }

    if (isConnected()) {
        return true;
    }

#ifdef _WIN32
    static WinSockRuntime win_sock_runtime;
    if (!win_sock_runtime.ready()) {
        return false;
    }
#endif

    ip_ = target_ip;
    port_ = target_port;

    const NativeSocket socket_handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#ifdef _WIN32
    if (socket_handle == INVALID_SOCKET) {
#else
    if (socket_handle < 0) {
#endif
        std::cerr << "UDP socket creation failed: " << lastSocketError() << std::endl;
        return false;
    }
    sock_fd_ = static_cast<std::intptr_t>(socket_handle);

    sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(static_cast<uint16_t>(port_));
    if (!parseIpv4Address(ip_, server_addr.sin_addr)) {
        std::cerr << "Invalid IPv4 address: " << ip_ << std::endl;
        closeSocket(sock_fd_);
        return false;
    }

    if (::connect(nativeSocket(sock_fd_), reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) != 0) {
        std::cerr << "UDP connect failed: " << lastSocketError() << std::endl;
        closeSocket(sock_fd_);
        return false;
    }

    return true;
}

bool UdpComm::connectToServer(const std::string& target_ip, int target_port) {
    return connect(target_ip, target_port);
}

void UdpComm::disconnect() {
    closeSocket(sock_fd_);
    read_buffer_.clear();
}

void UdpComm::writeData(const std::string& data) {
    if (!isConnected()) {
        return;
    }

    std::string final_data = data;
    if (final_data.empty() || final_data.back() != '\n') {
        final_data += '\n';
    }

#ifdef _WIN32
    const int sent = send(
        nativeSocket(sock_fd_),
        final_data.data(),
        static_cast<int>(final_data.size()),
        sendFlags());
    if (sent == SOCKET_ERROR) {
#else
    const ssize_t sent = send(
        nativeSocket(sock_fd_),
        final_data.data(),
        final_data.size(),
        sendFlags());
    if (sent < 0) {
#endif
        std::cerr << "UDP send error: " << lastSocketError() << std::endl;
        closeSocket(sock_fd_);
    }
}

std::string UdpComm::readLine() {
    if (!isConnected()) {
        return "";
    }

    std::string line;
    if (extractBufferedLine(read_buffer_, line)) {
        return line;
    }
    if (!read_buffer_.empty()) {
        line = trimTrailingLineEndings(std::move(read_buffer_));
        read_buffer_.clear();
        return line;
    }

    char buffer[4096];
#ifdef _WIN32
    const int received = recv(nativeSocket(sock_fd_), buffer, static_cast<int>(sizeof(buffer)), 0);
    if (received == SOCKET_ERROR) {
#else
    const ssize_t received = recv(nativeSocket(sock_fd_), buffer, sizeof(buffer), 0);
    if (received < 0) {
#endif
        std::cerr << "UDP receive error: " << lastSocketError() << std::endl;
        closeSocket(sock_fd_);
        read_buffer_.clear();
        return "";
    }

    read_buffer_.append(buffer, static_cast<std::size_t>(received));
    if (extractBufferedLine(read_buffer_, line)) {
        return line;
    }

    line = trimTrailingLineEndings(std::move(read_buffer_));
    read_buffer_.clear();
    return line;
}

bool UdpComm::dataAvailable() {
    if (!read_buffer_.empty()) {
        return true;
    }

    if (!isConnected()) {
        return false;
    }

    fd_set read_fds;
    timeval timeout = {};
    FD_ZERO(&read_fds);
    FD_SET(nativeSocket(sock_fd_), &read_fds);

#ifdef _WIN32
    const int ret = select(0, &read_fds, nullptr, nullptr, &timeout);
#else
    const int ret = select(nativeSocket(sock_fd_) + 1, &read_fds, nullptr, nullptr, &timeout);
#endif

    return ret > 0 && FD_ISSET(nativeSocket(sock_fd_), &read_fds);
}
