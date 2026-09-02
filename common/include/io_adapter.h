#pragma once

#include "comm_interface.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

using IoAdapterKey = std::uint32_t;
using IoRequestId = std::uint64_t;
using IoDeadlineNs = std::uint64_t;

enum class IoRequestState : std::uint8_t {
    Unknown = 0,
    Accepted = 1,
    Completed = 2,
    Timeout = 3,
    DeadlineMissed = 4,
    TransportError = 5,
    AdapterUnavailable = 6,
    InvalidRequest = 7,
};

using IoRequestStatus = IoRequestState;

enum class IoErrorCode : std::uint8_t {
    None = 0,
    InvalidRequest = 1,
    AdapterUnavailable = 2,
    TransportError = 3,
    Timeout = 4,
    DeadlineMissed = 5,
    QueueFull = 6,
    Cancelled = 7,
};

std::string_view io_request_state_name(IoRequestState state);
std::string_view io_request_state_detail(IoRequestState state);
std::string_view io_error_code_name(IoErrorCode code);

struct IoError {
    IoErrorCode code = IoErrorCode::None;
    std::string message;
    int native_code = 0;

    bool ok() const noexcept {
        return code == IoErrorCode::None;
    }

    static IoError none();
    static IoError invalid_request(std::string message);
    static IoError adapter_unavailable(std::string message);
    static IoError transport_error(std::string message, int native_code = 0);
    static IoError timeout(std::string message);
    static IoError deadline_missed(std::string message);
};

struct IoPayloadView {
    const std::uint8_t* data = nullptr;
    std::size_t size = 0;

    bool valid() const noexcept {
        return data != nullptr && size != 0;
    }
};

// A payload is a byte view with an optional shared owner. Passing a view to
// io_request_async() makes one explicit SDK-owned copy; passing a payload
// created with from_shared() keeps the caller's storage alive without copying
// its bytes across the SDK boundary.
class IoPayload {
public:
    IoPayload() = default;

    static IoPayload copy_of(IoPayloadView view);
    static IoPayload from_shared(
        std::shared_ptr<const void> owner,
        const std::uint8_t* data,
        std::size_t size);
    static IoPayload from_shared(std::shared_ptr<const std::vector<std::uint8_t>> bytes);

    const std::uint8_t* data() const noexcept {
        return data_;
    }

    std::size_t size() const noexcept {
        return size_;
    }

    bool valid() const noexcept {
        return data_ != nullptr && size_ != 0;
    }

    IoPayloadView view() const noexcept {
        return IoPayloadView{data_, size_};
    }

    std::vector<std::uint8_t> to_vector() const;

private:
    std::shared_ptr<const void> owner_;
    const std::uint8_t* data_ = nullptr;
    std::size_t size_ = 0;
};

struct IoRequest {
    IoAdapterKey adapter_key = 0;
    IoRequestId request_id = 0;
    IoPayload payload;
    IoDeadlineNs deadline_ns = 0;
};

struct IoResponse {
    IoAdapterKey adapter_key = 0;
    IoRequestId request_id = 0;
    IoRequestState state = IoRequestState::Unknown;
    IoDeadlineNs deadline_ns = 0;
    std::uint64_t timestamp_ns = 0;
    IoPayload payload;
    IoError error;

    bool terminal() const noexcept;
    bool ok() const noexcept {
        return state == IoRequestState::Completed && error.ok();
    }

    IoRequestState status() const noexcept {
        return state;
    }
};

class IoRequestResult {
public:
    IoRequestResult() = default;
    IoRequestResult(IoRequestId request_id, IoRequestState state, IoError error);

    bool accepted() const noexcept {
        return state_ == IoRequestState::Accepted;
    }

    bool ok() const noexcept {
        return accepted();
    }

    IoRequestId request_id() const noexcept {
        return request_id_;
    }

    IoRequestState state() const noexcept {
        return state_;
    }

    const IoError& error() const noexcept {
        return error_;
    }

private:
    IoRequestId request_id_ = 0;
    IoRequestState state_ = IoRequestState::Unknown;
    IoError error_;
};

class IoAdapterBackend {
public:
    using CompletionHandler = std::function<void(IoResponse)>;

    virtual ~IoAdapterBackend() = default;

    virtual IoAdapterKey adapter_key() const noexcept = 0;
    virtual IoError start() = 0;
    virtual bool available() const noexcept = 0;
    virtual IoError submit(const IoRequest& request, CompletionHandler completion) = 0;
    virtual void cancel(IoRequestId request_id) noexcept = 0;
    virtual void stop() noexcept = 0;
};

// Adapts the existing synchronous CommInterface implementations to the new
// event-driven request contract. Requests are consumed by a dedicated worker
// and responses are delivered by the worker callback; callers never poll a
// dataAvailable() loop to implement async behaviour.
struct CommInterfaceAdapterOptions {
    std::size_t max_queued_requests = 64;
    bool empty_response_is_transport_error = true;
};

class CommInterfaceIoAdapter : public IoAdapterBackend {
public:
    struct Impl;

    CommInterfaceIoAdapter(
        IoAdapterKey adapter_key,
        CommInterface& comm,
        CommInterfaceAdapterOptions options = {});
    CommInterfaceIoAdapter(
        IoAdapterKey adapter_key,
        std::shared_ptr<CommInterface> comm,
        CommInterfaceAdapterOptions options = {});
    ~CommInterfaceIoAdapter() override;

    IoAdapterKey adapter_key() const noexcept override;
    IoError start() override;
    bool available() const noexcept override;
    IoError submit(const IoRequest& request, CompletionHandler completion) override;
    void cancel(IoRequestId request_id) noexcept override;
    void stop() noexcept override;

protected:
    CommInterface& communication() const noexcept;

private:
    std::unique_ptr<Impl> impl_;
};

class TcpClientIoAdapter final : public CommInterfaceIoAdapter {
public:
    TcpClientIoAdapter(
        IoAdapterKey adapter_key,
        std::string target_ip,
        int target_port,
        CommInterfaceAdapterOptions options = {});

    IoError start() override;

private:
    std::string target_ip_;
    int target_port_ = 0;
};

class Rs485IoAdapter final : public CommInterfaceIoAdapter {
public:
    Rs485IoAdapter(
        IoAdapterKey adapter_key,
        std::string device_path,
        CommInterfaceAdapterOptions options = {});

    IoError start() override;

private:
    std::string device_path_;
};

class IoAdapterClient {
public:
    using CompletionHandler = IoAdapterBackend::CompletionHandler;

    explicit IoAdapterClient(std::chrono::milliseconds sync_wait_limit = std::chrono::milliseconds(1000));
    ~IoAdapterClient();

    IoAdapterClient(const IoAdapterClient&) = delete;
    IoAdapterClient& operator=(const IoAdapterClient&) = delete;
    IoAdapterClient(IoAdapterClient&&) = delete;
    IoAdapterClient& operator=(IoAdapterClient&&) = delete;

    IoError register_adapter(std::shared_ptr<IoAdapterBackend> adapter);
    IoError unregister_adapter(IoAdapterKey adapter_key);

    // Frozen async entry point. A successful submission emits an Accepted
    // response through the handler, followed by exactly one terminal response.
    IoRequestResult io_request_async(
        IoAdapterKey adapter_key,
        const IoPayload& payload,
        IoDeadlineNs deadline_ns);
    IoRequestResult io_request_async(
        IoAdapterKey adapter_key,
        const IoPayload& payload,
        IoDeadlineNs deadline_ns,
        CompletionHandler completion);
    IoRequestResult io_request_async(
        IoAdapterKey adapter_key,
        IoPayloadView payload,
        IoDeadlineNs deadline_ns,
        CompletionHandler completion = {});
    IoRequestResult io_request_async(
        IoAdapterKey adapter_key,
        const std::vector<std::uint8_t>& payload,
        IoDeadlineNs deadline_ns,
        CompletionHandler completion = {});
    IoRequestResult io_request_async(
        IoAdapterKey adapter_key,
        std::string_view payload,
        IoDeadlineNs deadline_ns,
        CompletionHandler completion = {});

    // The synchronous API is a bounded wait over the same async path. Its
    // response always exposes the terminal timeout/deadline/error state.
    IoResponse io_request_sync(
        IoAdapterKey adapter_key,
        const IoPayload& payload,
        IoDeadlineNs deadline_ns);
    IoResponse io_request_sync(
        IoAdapterKey adapter_key,
        IoPayloadView payload,
        IoDeadlineNs deadline_ns);
    IoResponse io_request_sync(
        IoAdapterKey adapter_key,
        const std::vector<std::uint8_t>& payload,
        IoDeadlineNs deadline_ns);
    IoResponse io_request_sync(
        IoAdapterKey adapter_key,
        std::string_view payload,
        IoDeadlineNs deadline_ns);

    void set_default_completion_handler(CompletionHandler completion);
    std::size_t pending_request_count() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
