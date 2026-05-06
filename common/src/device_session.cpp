#include "device_session.h"
#include <stdexcept>
#include <thread>
#include <utility>

namespace {

constexpr auto kPollInterval = std::chrono::milliseconds(1);

} // namespace

DeviceSession::DeviceSession(
    CommInterface& comm,
    int device_port,
    std::string address_separator,
    std::chrono::milliseconds default_timeout)
    : comm_(&comm),
      device_port_(device_port),
      address_separator_(std::move(address_separator)),
      default_timeout_(default_timeout) {}

void DeviceSession::setDefaultTimeout(std::chrono::milliseconds timeout) {
    default_timeout_ = timeout;
}

std::chrono::milliseconds DeviceSession::defaultTimeout() const {
    return default_timeout_;
}

int DeviceSession::devicePort() const {
    return device_port_;
}

std::string DeviceSession::buildCommand(const std::string& command) const {
    return "@" + std::to_string(device_port_) + address_separator_ + command;
}

void DeviceSession::send(const std::string& command) {
    comm_->writeData(buildCommand(command));
}

void DeviceSession::sendRaw(const std::string& payload) {
    comm_->writeData(payload);
}

bool DeviceSession::waitForData() {
    return waitForData(default_timeout_);
}

bool DeviceSession::waitForData(std::chrono::milliseconds timeout) {
    if (comm_->dataAvailable()) {
        return true;
    }

    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (comm_->dataAvailable()) {
            return true;
        }

        std::this_thread::sleep_for(kPollInterval);
    }

    return comm_->dataAvailable();
}

std::string DeviceSession::readLine() {
    return readLine(default_timeout_);
}

std::string DeviceSession::readLine(std::chrono::milliseconds timeout) {
    ensureReadable(timeout, "line read");
    return comm_->readLine();
}

std::string DeviceSession::readAvailable() {
    std::string status;
    while (comm_->dataAvailable()) {
        status += comm_->readLine();
    }

    return status;
}

std::string DeviceSession::sendAndWait(const std::string& command) {
    return sendAndWait(command, default_timeout_);
}

std::string DeviceSession::sendAndWait(const std::string& command, std::chrono::milliseconds timeout) {
    send(command);
    ensureReadable(timeout, buildCommand(command));
    return readAvailable();
}

std::string DeviceSession::sendRawAndWait(const std::string& payload) {
    return sendRawAndWait(payload, default_timeout_);
}

std::string DeviceSession::sendRawAndWait(const std::string& payload, std::chrono::milliseconds timeout) {
    sendRaw(payload);
    ensureReadable(timeout, payload);
    return readAvailable();
}

void DeviceSession::ensureReadable(std::chrono::milliseconds timeout, const std::string& context) {
    if (waitForData(timeout)) {
        return;
    }

    throw std::runtime_error(
        "Timed out waiting for device response after " + std::to_string(timeout.count()) +
        " ms: " + context);
}
