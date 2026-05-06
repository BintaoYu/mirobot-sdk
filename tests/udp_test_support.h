#pragma once

#include "test_support.h"
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <string>

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
using UdpNativeSocket = SOCKET;
using UdpSocketLength = int;
constexpr UdpNativeSocket kInvalidUdpSocket = INVALID_SOCKET;

class UdpWinSockGuard {
public:
    UdpWinSockGuard() {
        WSADATA data = {};
        const int result = WSAStartup(MAKEWORD(2, 2), &data);
        if (result != 0) {
            throw TestFailure("WSAStartup failed");
        }
    }

    ~UdpWinSockGuard() {
        WSACleanup();
    }
};

inline void closeUdpSocket(UdpNativeSocket socket_value) {
    if (socket_value != kInvalidUdpSocket) {
        closesocket(socket_value);
    }
}
#else
using UdpNativeSocket = int;
using UdpSocketLength = socklen_t;
constexpr UdpNativeSocket kInvalidUdpSocket = -1;

inline void closeUdpSocket(UdpNativeSocket socket_value) {
    if (socket_value != kInvalidUdpSocket) {
        close(socket_value);
    }
}
#endif

inline bool canCreateUdpSockets() {
#ifdef _WIN32
    static UdpWinSockGuard winsock_guard;
#endif

    const UdpNativeSocket socket_value = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_value == kInvalidUdpSocket) {
        return false;
    }

    closeUdpSocket(socket_value);
    return true;
}

class LoopbackUdpServer {
public:
    LoopbackUdpServer()
        : socket_(kInvalidUdpSocket),
          port_(0),
          has_last_peer_(false),
          last_peer_{} {
#ifdef _WIN32
        static UdpWinSockGuard winsock_guard;
#endif

        socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (socket_ == kInvalidUdpSocket) {
            throw TestFailure("Failed to create test UDP socket");
        }

        sockaddr_in address = {};
        address.sin_family = AF_INET;
        address.sin_port = htons(0);
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        if (bind(socket_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
            closeUdpSocket(socket_);
            throw TestFailure("Failed to bind test UDP socket");
        }

        UdpSocketLength address_length = static_cast<UdpSocketLength>(sizeof(address));
        if (getsockname(socket_, reinterpret_cast<sockaddr*>(&address), &address_length) != 0) {
            closeUdpSocket(socket_);
            throw TestFailure("Failed to query test UDP socket port");
        }

        port_ = ntohs(address.sin_port);
    }

    ~LoopbackUdpServer() {
        closeUdpSocket(socket_);
    }

    uint16_t port() const {
        return port_;
    }

    uint16_t lastPeerPort() const {
        if (!has_last_peer_) {
            throw TestFailure("No UDP peer has been recorded yet");
        }

        return ntohs(last_peer_.sin_port);
    }

    std::string readDatagram(std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) {
        waitReadable(timeout);

        char buffer[4096];
        sockaddr_in peer = {};
        UdpSocketLength peer_length = static_cast<UdpSocketLength>(sizeof(peer));

#ifdef _WIN32
        const int received = recvfrom(
            socket_,
            buffer,
            static_cast<int>(sizeof(buffer)),
            0,
            reinterpret_cast<sockaddr*>(&peer),
            &peer_length);
        if (received == SOCKET_ERROR) {
#else
        const ssize_t received = recvfrom(
            socket_,
            buffer,
            sizeof(buffer),
            0,
            reinterpret_cast<sockaddr*>(&peer),
            &peer_length);
        if (received < 0) {
#endif
            throw TestFailure("Failed to read UDP datagram from test socket");
        }

        last_peer_ = peer;
        has_last_peer_ = true;
        return std::string(buffer, static_cast<std::size_t>(received));
    }

    void sendRawToLastPeer(
        const std::string& payload,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) {
        if (!has_last_peer_) {
            throw TestFailure("Cannot send UDP reply before reading a client datagram");
        }

        waitWritable(timeout);

        const char* cursor = payload.data();
        std::size_t remaining = payload.size();

        while (remaining > 0) {
#ifdef _WIN32
            const int sent = sendto(
                socket_,
                cursor,
                static_cast<int>(remaining),
                0,
                reinterpret_cast<const sockaddr*>(&last_peer_),
                static_cast<int>(sizeof(last_peer_)));
            if (sent == SOCKET_ERROR) {
#else
            const ssize_t sent = sendto(
                socket_,
                cursor,
                remaining,
                0,
                reinterpret_cast<const sockaddr*>(&last_peer_),
                static_cast<UdpSocketLength>(sizeof(last_peer_)));
            if (sent < 0) {
#endif
                throw TestFailure("Failed to send UDP datagram from test socket");
            }

            remaining -= static_cast<std::size_t>(sent);
            cursor += sent;
        }
    }

private:
    void waitReadable(std::chrono::milliseconds timeout) const {
        waitForDirection(timeout, true);
    }

    void waitWritable(std::chrono::milliseconds timeout) const {
        waitForDirection(timeout, false);
    }

    void waitForDirection(std::chrono::milliseconds timeout, bool readable) const {
        fd_set target_fds;
        FD_ZERO(&target_fds);
        FD_SET(socket_, &target_fds);

        timeval tv = {};
        tv.tv_sec = static_cast<long>(timeout.count() / 1000);
        tv.tv_usec = static_cast<long>((timeout.count() % 1000) * 1000);

#ifdef _WIN32
        const int result = readable
            ? select(0, &target_fds, nullptr, nullptr, &tv)
            : select(0, nullptr, &target_fds, nullptr, &tv);
#else
        const int result = readable
            ? select(socket_ + 1, &target_fds, nullptr, nullptr, &tv)
            : select(socket_ + 1, nullptr, &target_fds, nullptr, &tv);
#endif

        if (result <= 0) {
            throw TestFailure(readable
                ? "Timed out waiting for readable UDP test socket"
                : "Timed out waiting for writable UDP test socket");
        }
    }

    UdpNativeSocket socket_;
    uint16_t port_;
    bool has_last_peer_;
    sockaddr_in last_peer_;
};

} // namespace test_support
