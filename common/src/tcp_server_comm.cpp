#include "tcp_server_comm.h"
#include <cerrno>
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
using SocketLength = int;

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

bool setReuseAddress(NativeSocket socket_handle) {
    const int reuse_address = 1;
    return setsockopt(
        socket_handle,
        SOL_SOCKET,
        SO_REUSEADDR,
        reinterpret_cast<const char*>(&reuse_address),
        static_cast<int>(sizeof(reuse_address))) == 0;
}
#else
constexpr std::intptr_t kInvalidSocketValue = -1;
using NativeSocket = int;
using SocketLength = socklen_t;

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

bool setReuseAddress(NativeSocket socket_handle) {
    const int reuse_address = 1;
    return setsockopt(
        socket_handle,
        SOL_SOCKET,
        SO_REUSEADDR,
        &reuse_address,
        sizeof(reuse_address)) == 0;
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

bool waitReadable(std::intptr_t socket_value, int timeout_ms) {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(nativeSocket(socket_value), &read_fds);

    timeval timeout = {};
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

#ifdef _WIN32
    const int result = select(0, &read_fds, nullptr, nullptr, &timeout);
#else
    const int result = select(nativeSocket(socket_value) + 1, &read_fds, nullptr, nullptr, &timeout);
#endif
    return result > 0 && FD_ISSET(nativeSocket(socket_value), &read_fds);
}

} // namespace

TcpServerComm::TcpServerComm()
    : bind_ip("0.0.0.0"),
      listen_port(0),
      listen_fd(kInvalidSocketValue),
      client_fd(kInvalidSocketValue) {}

TcpServerComm::~TcpServerComm() {
    stop();
}

bool TcpServerComm::listen(int listen_port) {
    return listen("0.0.0.0", listen_port, 1);
}

bool TcpServerComm::listen(const std::string& bind_ip, int listen_port, int backlog) {
    if (listen_port < 0 || listen_port > 65535) {
        std::cerr << "Invalid TCP listen port: " << listen_port << std::endl;
        return false;
    }

    if (backlog <= 0) {
        std::cerr << "Invalid TCP backlog: " << backlog << std::endl;
        return false;
    }

    stop();

#ifdef _WIN32
    static WinSockRuntime win_sock_runtime;
    if (!win_sock_runtime.ready()) {
        return false;
    }
#endif

    const NativeSocket socket_handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#ifdef _WIN32
    if (socket_handle == INVALID_SOCKET) {
#else
    if (socket_handle < 0) {
#endif
        std::cerr << "TCP server socket creation failed: " << lastSocketError() << std::endl;
        return false;
    }
    listen_fd = static_cast<std::intptr_t>(socket_handle);

    if (!setReuseAddress(socket_handle)) {
        std::cerr << "TCP server setsockopt(SO_REUSEADDR) failed: " << lastSocketError() << std::endl;
        stop();
        return false;
    }

    sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(static_cast<uint16_t>(listen_port));
    if (!parseIpv4Address(bind_ip, server_addr.sin_addr)) {
        std::cerr << "Invalid TCP server bind address: " << bind_ip << std::endl;
        stop();
        return false;
    }

    if (bind(nativeSocket(listen_fd), reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) != 0) {
        std::cerr << "TCP server bind failed: " << lastSocketError() << std::endl;
        stop();
        return false;
    }

    if (::listen(nativeSocket(listen_fd), backlog) != 0) {
        std::cerr << "TCP server listen failed: " << lastSocketError() << std::endl;
        stop();
        return false;
    }

    sockaddr_in local_addr = {};
    SocketLength local_addr_len = static_cast<SocketLength>(sizeof(local_addr));
    if (getsockname(nativeSocket(listen_fd), reinterpret_cast<sockaddr*>(&local_addr), &local_addr_len) != 0) {
        std::cerr << "TCP server getsockname failed: " << lastSocketError() << std::endl;
        stop();
        return false;
    }

    this->bind_ip = bind_ip;
    this->listen_port = ntohs(local_addr.sin_port);
    return true;
}

bool TcpServerComm::acceptClient(int timeout_ms) {
    if (!isListening()) {
        return false;
    }

    if (hasClient()) {
        return true;
    }

    if (timeout_ms >= 0 && !waitReadable(listen_fd, timeout_ms)) {
        return false;
    }

    sockaddr_in client_addr = {};
    SocketLength client_addr_len = static_cast<SocketLength>(sizeof(client_addr));
    const NativeSocket accepted_socket = accept(
        nativeSocket(listen_fd),
        reinterpret_cast<sockaddr*>(&client_addr),
        &client_addr_len);

#ifdef _WIN32
    if (accepted_socket == INVALID_SOCKET) {
#else
    if (accepted_socket < 0) {
#endif
        std::cerr << "TCP server accept failed: " << lastSocketError() << std::endl;
        return false;
    }

    client_fd = static_cast<std::intptr_t>(accepted_socket);
    return true;
}

void TcpServerComm::disconnectClient() {
    closeSocket(client_fd);
}

void TcpServerComm::stop() {
    disconnectClient();
    closeSocket(listen_fd);
    listen_port = 0;
}

bool TcpServerComm::isListening() const {
    return listen_fd != kInvalidSocketValue;
}

bool TcpServerComm::hasClient() const {
    return client_fd != kInvalidSocketValue;
}

int TcpServerComm::port() const {
    return listen_port;
}

void TcpServerComm::writeData(const std::string& data) {
    if (!hasClient()) {
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
        const int sent = send(nativeSocket(client_fd), cursor, static_cast<int>(remaining), sendFlags());
        if (sent == SOCKET_ERROR) {
#else
        const ssize_t sent = send(nativeSocket(client_fd), cursor, remaining, sendFlags());
        if (sent < 0) {
#endif
            std::cerr << "TCP server send error: " << lastSocketError() << std::endl;
            disconnectClient();
            return;
        }

        if (sent == 0) {
            std::cerr << "TCP server send returned 0 bytes" << std::endl;
            disconnectClient();
            return;
        }

        remaining -= static_cast<std::size_t>(sent);
        cursor += sent;
    }
}

std::string TcpServerComm::readLine() {
    if (!hasClient()) {
        return "";
    }

    std::string result;
    char ch = '\0';

    while (true) {
#ifdef _WIN32
        const int received = recv(nativeSocket(client_fd), &ch, 1, 0);
        if (received == SOCKET_ERROR || received == 0) {
            disconnectClient();
            break;
        }
#else
        const ssize_t received = recv(nativeSocket(client_fd), &ch, 1, 0);
        if (received <= 0) {
            disconnectClient();
            break;
        }
#endif
        if (ch == '\n') {
            break;
        }
        if (ch != '\r') {
            result.push_back(ch);
        }
    }

    return result;
}

bool TcpServerComm::dataAvailable() {
    if (!hasClient()) {
        return false;
    }

    return waitReadable(client_fd, 0);
}
