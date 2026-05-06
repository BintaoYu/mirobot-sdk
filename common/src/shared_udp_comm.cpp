#include "shared_udp_comm.h"
#include <mutex>
#include <stdexcept>

namespace {

std::mutex g_shared_udp_mutex;
std::weak_ptr<UdpComm> g_shared_udp_comm;
std::string g_shared_udp_ip;
int g_shared_udp_port = 0;

} // namespace

std::shared_ptr<UdpComm> acquireSharedUdpComm(const std::string& target_ip, int target_port) {
    std::lock_guard<std::mutex> lock(g_shared_udp_mutex);

    if (target_port <= 0 || target_port > 65535) {
        throw std::runtime_error("Invalid shared UDP port: " + std::to_string(target_port));
    }

    if (auto shared_comm = g_shared_udp_comm.lock()) {
        if (g_shared_udp_ip != target_ip || g_shared_udp_port != target_port) {
            throw std::runtime_error(
                "Shared UDP transport already bound to " + g_shared_udp_ip + ":" +
                std::to_string(g_shared_udp_port) + ", cannot switch to " + target_ip + ":" +
                std::to_string(target_port));
        }

        if (!shared_comm->isConnected() && !shared_comm->connect(target_ip, target_port)) {
            throw std::runtime_error(
                "Failed to reconnect shared UDP transport to " + target_ip + ":" +
                std::to_string(target_port));
        }

        return shared_comm;
    }

    auto shared_comm = std::make_shared<UdpComm>();
    if (!shared_comm->connect(target_ip, target_port)) {
        throw std::runtime_error(
            "Failed to prepare shared UDP transport at " + target_ip + ":" +
            std::to_string(target_port));
    }

    g_shared_udp_ip = target_ip;
    g_shared_udp_port = target_port;
    g_shared_udp_comm = shared_comm;
    return shared_comm;
}
