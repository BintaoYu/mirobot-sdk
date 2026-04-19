#pragma once

#include "test_support.h"
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace test_support {

#ifdef _WIN32
using NativeSocket = SOCKET;
constexpr NativeSocket kInvalidSocket = INVALID_SOCKET;

class WinSockGuard {
public:
    WinSockGuard() {
        WSADATA data = {};
        const int result = WSAStartup(MAKEWORD(2, 2), &data);
        if (result != 0) {
            throw TestFailure("WSAStartup failed");
        }
    }

    ~WinSockGuard() {
        WSACleanup();
    }
};

inline void closeSocket(NativeSocket socket_value) {
    if (socket_value != kInvalidSocket) {
        closesocket(socket_value);
    }
}

inline int socketLastError() {
    return WSAGetLastError();
}
#else
using NativeSocket = int;
constexpr NativeSocket kInvalidSocket = -1;

inline void closeSocket(NativeSocket socket_value) {
    if (socket_value != kInvalidSocket) {
        close(socket_value);
    }
}

inline int socketLastError() {
    return errno;
}
#endif

inline bool canCreateTcpSockets() {
#ifdef _WIN32
    static WinSockGuard winsock_guard;
#endif

    const NativeSocket socket_value = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_value == kInvalidSocket) {
        return false;
    }

    closeSocket(socket_value);
    return true;
}

class LoopbackTcpServer {
public:
    LoopbackTcpServer() : listen_socket_(kInvalidSocket), client_socket_(kInvalidSocket), port_(0), accept_done_(false) {
#ifdef _WIN32
        static WinSockGuard winsock_guard;
#endif

        listen_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listen_socket_ == kInvalidSocket) {
            throw TestFailure("Failed to create test TCP listen socket");
        }

        sockaddr_in address = {};
        address.sin_family = AF_INET;
        address.sin_port = htons(0);
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        if (bind(listen_socket_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
            closeSocket(listen_socket_);
            throw TestFailure("Failed to bind test TCP listen socket");
        }

        if (listen(listen_socket_, 1) != 0) {
            closeSocket(listen_socket_);
            throw TestFailure("Failed to listen on test TCP socket");
        }

        socklen_t address_length = sizeof(address);
        if (getsockname(listen_socket_, reinterpret_cast<sockaddr*>(&address), &address_length) != 0) {
            closeSocket(listen_socket_);
            throw TestFailure("Failed to query test TCP listen port");
        }

        port_ = ntohs(address.sin_port);
        accept_thread_ = std::thread([this]() { acceptClient(); });
    }

    ~LoopbackTcpServer() {
        closeSocket(client_socket_);
        closeSocket(listen_socket_);

        if (accept_thread_.joinable()) {
            accept_thread_.join();
        }
    }

    uint16_t port() const {
        return port_;
    }

    void waitForClient(std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) {
        std::unique_lock<std::mutex> lock(mutex_);
        const bool ready = cv_.wait_for(lock, timeout, [this]() {
            return client_socket_ != kInvalidSocket || accept_done_;
        });

        if (!ready || client_socket_ == kInvalidSocket) {
            throw TestFailure("Timed out waiting for TCP client connection");
        }
    }

    std::string readLine(std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) {
        waitForClient(timeout);

        std::string result;
        char ch = '\0';

        while (true) {
            waitReadable(timeout);
#ifdef _WIN32
            const int received = recv(client_socket_, &ch, 1, 0);
            if (received == SOCKET_ERROR || received == 0) {
                throw TestFailure("TCP test server failed while reading client data");
            }
#else
            const ssize_t received = recv(client_socket_, &ch, 1, 0);
            if (received <= 0) {
                throw TestFailure("TCP test server failed while reading client data");
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

    void sendRaw(const std::string& payload, std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) {
        waitForClient(timeout);

        const char* cursor = payload.data();
        std::size_t remaining = payload.size();

        while (remaining > 0) {
#ifdef _WIN32
            const int sent = send(client_socket_, cursor, static_cast<int>(remaining), 0);
            if (sent == SOCKET_ERROR) {
                throw TestFailure("TCP test server failed while sending to client");
            }
#else
            const ssize_t sent = send(client_socket_, cursor, remaining, 0);
            if (sent < 0) {
                throw TestFailure("TCP test server failed while sending to client");
            }
#endif
            remaining -= static_cast<std::size_t>(sent);
            cursor += sent;
        }
    }

private:
    void acceptClient() {
        sockaddr_in address = {};
        socklen_t address_length = sizeof(address);
        const NativeSocket accepted = accept(listen_socket_, reinterpret_cast<sockaddr*>(&address), &address_length);

        {
            std::lock_guard<std::mutex> lock(mutex_);
            client_socket_ = accepted;
            accept_done_ = true;
        }

        cv_.notify_all();
    }

    void waitReadable(std::chrono::milliseconds timeout) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(client_socket_, &read_fds);

        timeval tv = {};
        tv.tv_sec = static_cast<long>(timeout.count() / 1000);
        tv.tv_usec = static_cast<long>((timeout.count() % 1000) * 1000);

#ifdef _WIN32
        const int result = select(0, &read_fds, nullptr, nullptr, &tv);
#else
        const int result = select(client_socket_ + 1, &read_fds, nullptr, nullptr, &tv);
#endif
        if (result <= 0) {
            throw TestFailure("Timed out waiting for readable TCP test socket");
        }
    }

    NativeSocket listen_socket_;
    NativeSocket client_socket_;
    uint16_t port_;
    bool accept_done_;
    std::thread accept_thread_;
    std::mutex mutex_;
    std::condition_variable cv_;
};

} // namespace test_support
