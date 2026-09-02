#include "io_adapter.h"

#include "serial_comm.h"
#include "tcp_comm.h"

#include <algorithm>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace {

using SteadyClock = std::chrono::steady_clock;

std::uint64_t monotonic_now_ns() noexcept {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(SteadyClock::now().time_since_epoch()).count());
}

IoRequestState state_for_error(IoErrorCode code) noexcept {
    switch (code) {
        case IoErrorCode::InvalidRequest:
            return IoRequestState::InvalidRequest;
        case IoErrorCode::AdapterUnavailable:
            return IoRequestState::AdapterUnavailable;
        case IoErrorCode::Timeout:
            return IoRequestState::Timeout;
        case IoErrorCode::DeadlineMissed:
            return IoRequestState::DeadlineMissed;
        case IoErrorCode::None:
            return IoRequestState::Completed;
        case IoErrorCode::TransportError:
        case IoErrorCode::QueueFull:
        case IoErrorCode::Cancelled:
            return IoRequestState::TransportError;
    }
    return IoRequestState::TransportError;
}

IoError error_for_state(IoRequestState state, std::string message = {}) {
    switch (state) {
        case IoRequestState::Completed:
        case IoRequestState::Accepted:
            return IoError::none();
        case IoRequestState::Timeout:
            return IoError::timeout(message.empty() ? "request timed out" : std::move(message));
        case IoRequestState::DeadlineMissed:
            return IoError::deadline_missed(message.empty() ? "request deadline was missed" : std::move(message));
        case IoRequestState::AdapterUnavailable:
            return IoError::adapter_unavailable(message.empty() ? "adapter is unavailable" : std::move(message));
        case IoRequestState::InvalidRequest:
            return IoError::invalid_request(message.empty() ? "request is invalid" : std::move(message));
        case IoRequestState::TransportError:
            return IoError::transport_error(message.empty() ? "adapter transport failed" : std::move(message));
        case IoRequestState::Unknown:
            return IoError::transport_error(message.empty() ? "adapter returned an unknown state" : std::move(message));
    }
    return IoError::transport_error(message.empty() ? "adapter returned an unknown state" : std::move(message));
}

IoResponse make_response(
    const IoRequest& request,
    IoRequestState state,
    IoError error,
    IoPayload payload = {},
    std::uint64_t timestamp_ns = 0) {
    IoResponse response;
    response.adapter_key = request.adapter_key;
    response.request_id = request.request_id;
    response.state = state;
    response.deadline_ns = request.deadline_ns;
    response.timestamp_ns = timestamp_ns == 0 ? monotonic_now_ns() : timestamp_ns;
    response.payload = std::move(payload);
    response.error = std::move(error);
    return response;
}

std::shared_ptr<CommInterface> borrow_comm(CommInterface& comm) {
    return std::shared_ptr<CommInterface>(&comm, [](CommInterface*) {});
}

std::shared_ptr<CommInterface> make_tcp_comm() {
    return std::make_shared<TcpComm>();
}

std::shared_ptr<CommInterface> make_serial_comm(const std::string& device_path) {
    return std::make_shared<SerialComm>(device_path);
}

} // namespace

std::string_view io_request_state_name(IoRequestState state) {
    switch (state) {
        case IoRequestState::Unknown:
            return "Unknown";
        case IoRequestState::Accepted:
            return "Accepted";
        case IoRequestState::Completed:
            return "Completed";
        case IoRequestState::Timeout:
            return "Timeout";
        case IoRequestState::DeadlineMissed:
            return "DeadlineMissed";
        case IoRequestState::TransportError:
            return "TransportError";
        case IoRequestState::AdapterUnavailable:
            return "AdapterUnavailable";
        case IoRequestState::InvalidRequest:
            return "InvalidRequest";
    }
    return "Unknown";
}

std::string_view io_request_state_detail(IoRequestState state) {
    switch (state) {
        case IoRequestState::Unknown:
            return "unknown";
        case IoRequestState::Accepted:
            return "accepted";
        case IoRequestState::Completed:
            return "completed";
        case IoRequestState::Timeout:
            return "timeout";
        case IoRequestState::DeadlineMissed:
            return "deadline_missed";
        case IoRequestState::TransportError:
            return "transport_error";
        case IoRequestState::AdapterUnavailable:
            return "adapter_unavailable";
        case IoRequestState::InvalidRequest:
            return "invalid_request";
    }
    return "unknown";
}

std::string_view io_error_code_name(IoErrorCode code) {
    switch (code) {
        case IoErrorCode::None:
            return "None";
        case IoErrorCode::InvalidRequest:
            return "InvalidRequest";
        case IoErrorCode::AdapterUnavailable:
            return "AdapterUnavailable";
        case IoErrorCode::TransportError:
            return "TransportError";
        case IoErrorCode::Timeout:
            return "Timeout";
        case IoErrorCode::DeadlineMissed:
            return "DeadlineMissed";
        case IoErrorCode::QueueFull:
            return "QueueFull";
        case IoErrorCode::Cancelled:
            return "Cancelled";
    }
    return "Unknown";
}

IoError IoError::none() {
    return {};
}

IoError IoError::invalid_request(std::string message) {
    return IoError{IoErrorCode::InvalidRequest, std::move(message), 0};
}

IoError IoError::adapter_unavailable(std::string message) {
    return IoError{IoErrorCode::AdapterUnavailable, std::move(message), 0};
}

IoError IoError::transport_error(std::string message, int native_code) {
    return IoError{IoErrorCode::TransportError, std::move(message), native_code};
}

IoError IoError::timeout(std::string message) {
    return IoError{IoErrorCode::Timeout, std::move(message), 0};
}

IoError IoError::deadline_missed(std::string message) {
    return IoError{IoErrorCode::DeadlineMissed, std::move(message), 0};
}

IoPayload IoPayload::copy_of(IoPayloadView view) {
    if (!view.valid()) {
        return {};
    }

    auto bytes = std::make_shared<std::vector<std::uint8_t>>(view.data, view.data + view.size);
    return from_shared(std::move(bytes));
}

IoPayload IoPayload::from_shared(
    std::shared_ptr<const void> owner,
    const std::uint8_t* data,
    std::size_t size) {
    IoPayload payload;
    if (!owner || !data || size == 0) {
        return payload;
    }

    payload.owner_ = std::move(owner);
    payload.data_ = data;
    payload.size_ = size;
    return payload;
}

IoPayload IoPayload::from_shared(std::shared_ptr<const std::vector<std::uint8_t>> bytes) {
    if (!bytes || bytes->empty()) {
        return {};
    }

    const std::uint8_t* data = bytes->data();
    const std::size_t size = bytes->size();
    std::shared_ptr<const void> owner = std::move(bytes);
    return from_shared(std::move(owner), data, size);
}

std::vector<std::uint8_t> IoPayload::to_vector() const {
    if (!valid()) {
        return {};
    }
    return std::vector<std::uint8_t>(data_, data_ + size_);
}

bool IoResponse::terminal() const noexcept {
    return state != IoRequestState::Unknown && state != IoRequestState::Accepted;
}

IoRequestResult::IoRequestResult(IoRequestId request_id, IoRequestState state, IoError error)
    : request_id_(request_id), state_(state), error_(std::move(error)) {
}

struct CommInterfaceIoAdapter::Impl {
    struct QueuedRequest {
        IoRequest request;
        CompletionHandler completion;
    };

    IoAdapterKey adapter_key = 0;
    std::shared_ptr<CommInterface> comm;
    CommInterfaceAdapterOptions options;
    mutable std::mutex mutex;
    std::condition_variable condition;
    std::deque<QueuedRequest> queue;
    std::unordered_set<IoRequestId> cancelled;
    std::thread worker;
    bool started = false;
    bool stopping = false;
    bool delete_after_worker = false;

    Impl(
        IoAdapterKey key,
        std::shared_ptr<CommInterface> communication,
        CommInterfaceAdapterOptions adapter_options)
        : adapter_key(key),
          comm(std::move(communication)),
          options(adapter_options) {
    }
};

namespace {

void emit_backend_response(
    const CommInterfaceIoAdapter::Impl::QueuedRequest& queued,
    IoRequestState state,
    IoError error,
    IoPayload payload = {}) {
    if (!queued.completion) {
        return;
    }
    try {
        queued.completion(make_response(queued.request, state, std::move(error), std::move(payload)));
    } catch (...) {
        // A consumer callback must not terminate the adapter worker. The
        // request has already been settled by the SDK before this callback.
    }
}

bool take_cancelled(CommInterfaceIoAdapter::Impl& impl, IoRequestId request_id) {
    std::lock_guard<std::mutex> lock(impl.mutex);
    const auto it = impl.cancelled.find(request_id);
    if (it == impl.cancelled.end()) {
        return false;
    }
    impl.cancelled.erase(it);
    return true;
}

bool comm_available(const CommInterface& comm) {
    if (const auto* tcp = dynamic_cast<const TcpComm*>(&comm)) {
        return tcp->isConnected();
    }
    if (const auto* serial = dynamic_cast<const SerialComm*>(&comm)) {
        return serial->isOpen();
    }
    return true;
}

void close_comm(CommInterface& comm) noexcept {
    if (auto* tcp = dynamic_cast<TcpComm*>(&comm)) {
        tcp->disconnect();
    } else if (auto* serial = dynamic_cast<SerialComm*>(&comm)) {
        serial->close();
    }
}

void run_comm_worker(CommInterfaceIoAdapter::Impl& impl) {
    for (;;) {
        CommInterfaceIoAdapter::Impl::QueuedRequest queued;
        {
            std::unique_lock<std::mutex> lock(impl.mutex);
            impl.condition.wait(lock, [&]() {
                return impl.stopping || !impl.queue.empty();
            });
            if (impl.stopping && impl.queue.empty()) {
                return;
            }
            queued = std::move(impl.queue.front());
            impl.queue.pop_front();
        }

        if (take_cancelled(impl, queued.request.request_id)) {
            continue;
        }

        if (queued.request.deadline_ns != 0 && monotonic_now_ns() >= queued.request.deadline_ns) {
            emit_backend_response(
                queued,
                IoRequestState::DeadlineMissed,
                IoError::deadline_missed("request was already past its deadline before transport write"));
            continue;
        }

        try {
            const std::string bytes(
                reinterpret_cast<const char*>(queued.request.payload.data()),
                queued.request.payload.size());
            impl.comm->writeData(bytes);

            if (take_cancelled(impl, queued.request.request_id)) {
                continue;
            }
            if (!comm_available(*impl.comm)) {
                emit_backend_response(
                    queued,
                    IoRequestState::TransportError,
                    IoError::transport_error("communication primitive became unavailable after write"));
                continue;
            }

            // CommInterface's existing readLine() blocks in the underlying
            // transport. This worker is the event-driven adapter boundary;
            // the caller thread never turns async into a polling loop.
            const std::string response = impl.comm->readLine();
            if (take_cancelled(impl, queued.request.request_id)) {
                continue;
            }
            if (!comm_available(*impl.comm)) {
                emit_backend_response(
                    queued,
                    IoRequestState::TransportError,
                    IoError::transport_error("communication primitive closed while reading response"));
                continue;
            }
            if (response.empty() && impl.options.empty_response_is_transport_error) {
                emit_backend_response(
                    queued,
                    IoRequestState::TransportError,
                    IoError::transport_error("communication primitive returned an empty response"));
                continue;
            }

            const IoRequestState state =
                queued.request.deadline_ns != 0 && monotonic_now_ns() >= queued.request.deadline_ns
                ? IoRequestState::DeadlineMissed
                : IoRequestState::Completed;
            if (state == IoRequestState::DeadlineMissed) {
                emit_backend_response(
                    queued,
                    state,
                    IoError::deadline_missed("response arrived after the request deadline"));
            } else {
                const IoPayload response_payload = IoPayload::copy_of(IoPayloadView{
                    reinterpret_cast<const std::uint8_t*>(response.data()),
                    response.size()});
                emit_backend_response(queued, state, IoError::none(), response_payload);
            }
        } catch (const std::exception& error) {
            emit_backend_response(
                queued,
                IoRequestState::TransportError,
                IoError::transport_error(error.what()));
        } catch (...) {
            emit_backend_response(
                queued,
                IoRequestState::TransportError,
                IoError::transport_error("communication primitive raised an unknown exception"));
        }
    }
}

} // namespace

CommInterfaceIoAdapter::CommInterfaceIoAdapter(
    IoAdapterKey adapter_key,
    CommInterface& comm,
    CommInterfaceAdapterOptions options)
    : impl_(std::make_unique<Impl>(adapter_key, borrow_comm(comm), options)) {
}

CommInterfaceIoAdapter::CommInterfaceIoAdapter(
    IoAdapterKey adapter_key,
    std::shared_ptr<CommInterface> comm,
    CommInterfaceAdapterOptions options)
    : impl_(std::make_unique<Impl>(adapter_key, std::move(comm), options)) {
}

CommInterfaceIoAdapter::~CommInterfaceIoAdapter() {
    stop();
}

IoAdapterKey CommInterfaceIoAdapter::adapter_key() const noexcept {
    return impl_ ? impl_->adapter_key : 0;
}

IoError CommInterfaceIoAdapter::start() {
    if (!impl_ || !impl_->comm) {
        return IoError::adapter_unavailable("communication primitive is absent");
    }

    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (impl_->started) {
        return IoError::none();
    }
    if (!comm_available(*impl_->comm)) {
        return IoError::adapter_unavailable("communication primitive is not open");
    }

    impl_->stopping = false;
    impl_->started = true;
    impl_->delete_after_worker = false;
    try {
        Impl* worker_impl = impl_.get();
        impl_->worker = std::thread([worker_impl]() {
            run_comm_worker(*worker_impl);
            if (worker_impl->delete_after_worker) {
                delete worker_impl;
            }
        });
    } catch (const std::exception& error) {
        impl_->started = false;
        impl_->stopping = true;
        return IoError::transport_error(error.what());
    } catch (...) {
        impl_->started = false;
        impl_->stopping = true;
        return IoError::transport_error("failed to start adapter worker");
    }
    return IoError::none();
}

bool CommInterfaceIoAdapter::available() const noexcept {
    if (!impl_ || !impl_->comm) {
        return false;
    }

    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->started && !impl_->stopping && comm_available(*impl_->comm);
}

IoError CommInterfaceIoAdapter::submit(const IoRequest& request, CompletionHandler completion) {
    if (!impl_ || request.adapter_key != adapter_key()) {
        return IoError::invalid_request("request adapter_key does not match backend");
    }
    if (request.request_id == 0 || !request.payload.valid()) {
        return IoError::invalid_request("request id and non-empty payload are required");
    }
    if (!completion) {
        return IoError::invalid_request("a completion callback is required");
    }

    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->started || impl_->stopping || !comm_available(*impl_->comm)) {
        return IoError::adapter_unavailable("communication primitive is not available");
    }
    if (impl_->queue.size() >= impl_->options.max_queued_requests) {
        return IoError{IoErrorCode::QueueFull, "adapter request queue is full", 0};
    }

    impl_->queue.push_back(Impl::QueuedRequest{request, std::move(completion)});
    impl_->condition.notify_one();
    return IoError::none();
}

void CommInterfaceIoAdapter::cancel(IoRequestId request_id) noexcept {
    if (!impl_ || request_id == 0) {
        return;
    }

    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->cancelled.insert(request_id);
    impl_->condition.notify_one();
}

void CommInterfaceIoAdapter::stop() noexcept {
    if (!impl_) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        if (!impl_->started && !impl_->worker.joinable()) {
            return;
        }
        impl_->stopping = true;
        for (const auto& queued : impl_->queue) {
            impl_->cancelled.insert(queued.request.request_id);
        }
        impl_->condition.notify_all();
    }

    close_comm(*impl_->comm);
    if (impl_->worker.joinable()) {
        if (impl_->worker.get_id() == std::this_thread::get_id()) {
            impl_->delete_after_worker = true;
            impl_->worker.detach();
            (void)impl_.release();
            return;
        } else {
            impl_->worker.join();
        }
    }

    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->queue.clear();
    impl_->started = false;
}

CommInterface& CommInterfaceIoAdapter::communication() const noexcept {
    return *impl_->comm;
}

TcpClientIoAdapter::TcpClientIoAdapter(
    IoAdapterKey adapter_key,
    std::string target_ip,
    int target_port,
    CommInterfaceAdapterOptions options)
    : CommInterfaceIoAdapter(adapter_key, make_tcp_comm(), options),
      target_ip_(std::move(target_ip)),
      target_port_(target_port) {
}

IoError TcpClientIoAdapter::start() {
    if (target_ip_.empty() || target_port_ <= 0 || target_port_ > 65535) {
        return IoError::invalid_request("TCP adapter requires a valid IPv4 address and port");
    }

    auto* tcp = dynamic_cast<TcpComm*>(&communication());
    if (!tcp) {
        return IoError::transport_error("TCP adapter communication primitive has the wrong type");
    }
    if (!tcp->isConnected() && !tcp->connect(target_ip_, target_port_)) {
        return IoError::adapter_unavailable("TCP client could not connect to the target");
    }
    return CommInterfaceIoAdapter::start();
}

Rs485IoAdapter::Rs485IoAdapter(
    IoAdapterKey adapter_key,
    std::string device_path,
    CommInterfaceAdapterOptions options)
    : CommInterfaceIoAdapter(adapter_key, make_serial_comm(device_path), options),
      device_path_(std::move(device_path)) {
}

IoError Rs485IoAdapter::start() {
    if (device_path_.empty()) {
        return IoError::invalid_request("RS485 adapter requires a device path");
    }

    auto* serial = dynamic_cast<SerialComm*>(&communication());
    if (!serial || !serial->isOpen()) {
        return IoError::adapter_unavailable("RS485 serial device is not open");
    }
    return CommInterfaceIoAdapter::start();
}

struct IoAdapterClient::Impl {
    struct Pending {
        IoRequest request;
        std::shared_ptr<IoAdapterBackend> backend;
        CompletionHandler completion;
        bool accepted = false;
        bool submitting = true;
        bool accepted_delivered = false;
        std::vector<IoResponse> deferred_completions;
    };

    explicit Impl(std::chrono::milliseconds wait_limit)
        : sync_wait_limit(wait_limit > std::chrono::milliseconds::zero() ? wait_limit : std::chrono::milliseconds(1)),
          deadline_thread([this]() { deadline_loop(); }) {
    }

    ~Impl() {
        shutdown();
    }

    std::chrono::milliseconds sync_wait_limit;
    mutable std::mutex mutex;
    std::condition_variable condition;
    std::unordered_map<IoAdapterKey, std::shared_ptr<IoAdapterBackend>> adapters;
    std::unordered_map<IoRequestId, Pending> pending;
    CompletionHandler default_completion;
    IoRequestId next_request_id = 1;
    bool stopping = false;
    std::thread deadline_thread;

    IoRequestId allocate_request_id() {
        std::lock_guard<std::mutex> lock(mutex);
        for (;;) {
            IoRequestId candidate = next_request_id++;
            if (candidate == 0) {
                continue;
            }
            if (pending.find(candidate) == pending.end()) {
                return candidate;
            }
        }
    }

    CompletionHandler handler_or_default(CompletionHandler handler) {
        if (handler) {
            return handler;
        }
        std::lock_guard<std::mutex> lock(mutex);
        return default_completion;
    }

    static void invoke(const CompletionHandler& handler, IoResponse response) noexcept {
        if (!handler) {
            return;
        }
        try {
            handler(std::move(response));
        } catch (...) {
            // User callbacks are outside the request state machine. A thrown
            // callback cannot be allowed to kill the deadline coordinator or
            // a transport worker.
        }
    }

    void settle_immediate(
        const IoRequest& request,
        IoRequestState state,
        IoError error,
        const CompletionHandler& handler) {
        invoke(handler, make_response(request, state, std::move(error)));
    }

    void backend_completed(IoRequestId request_id, IoResponse response) {
        CompletionHandler handler;
        std::optional<IoResponse> settled;
        {
            std::lock_guard<std::mutex> lock(mutex);
            const auto it = pending.find(request_id);
            if (it == pending.end()) {
                return;
            }

            Pending& entry = it->second;
            if (response.request_id != 0 && response.request_id != request_id) {
                // A response for another request must never complete this
                // request. The timer remains authoritative for this entry.
                return;
            }
            if (!response.terminal()) {
                return;
            }
            response.request_id = request_id;
            response.adapter_key = entry.request.adapter_key;
            response.deadline_ns = entry.request.deadline_ns;
            if (response.timestamp_ns == 0) {
                response.timestamp_ns = monotonic_now_ns();
            }
            if (response.state == IoRequestState::Completed &&
                entry.request.deadline_ns != 0 &&
                response.timestamp_ns >= entry.request.deadline_ns) {
                response.state = IoRequestState::DeadlineMissed;
                response.error = error_for_state(
                    IoRequestState::DeadlineMissed,
                    "response arrived after the request deadline");
                response.payload = {};
            } else if (response.error.ok()) {
                response.error = error_for_state(response.state);
            }

            if (!entry.accepted || !entry.accepted_delivered) {
                entry.deferred_completions.push_back(std::move(response));
                return;
            }

            handler = entry.completion;
            settled.emplace(std::move(response));
            pending.erase(it);
            condition.notify_all();
        }

        invoke(handler, std::move(*settled));
    }

    void deadline_loop() {
        for (;;) {
            std::vector<std::pair<CompletionHandler, IoResponse>> expired;
            {
                std::unique_lock<std::mutex> lock(mutex);
                if (stopping) {
                    return;
                }

                IoDeadlineNs nearest = 0;
                for (const auto& item : pending) {
                    if (item.second.submitting || !item.second.accepted_delivered) {
                        continue;
                    }
                    const IoDeadlineNs deadline = item.second.request.deadline_ns;
                    if (deadline == 0 || (nearest != 0 && deadline >= nearest)) {
                        continue;
                    }
                    nearest = deadline;
                }

                if (nearest == 0) {
                    condition.wait(lock, [this]() {
                        if (stopping) {
                            return true;
                        }
                        return std::any_of(pending.begin(), pending.end(), [](const auto& item) {
                            return !item.second.submitting && item.second.accepted_delivered &&
                                   item.second.request.deadline_ns != 0;
                        });
                    });
                    continue;
                }

                const IoDeadlineNs now = monotonic_now_ns();
                if (now < nearest) {
                    condition.wait_for(lock, std::chrono::nanoseconds(nearest - now));
                    continue;
                }

                std::vector<std::pair<std::shared_ptr<IoAdapterBackend>, IoRequestId>> cancellations;
                for (auto it = pending.begin(); it != pending.end();) {
                    const Pending& entry = it->second;
                    if (entry.submitting || !entry.accepted_delivered ||
                        entry.request.deadline_ns == 0 || entry.request.deadline_ns > now) {
                        ++it;
                        continue;
                    }

                    const IoRequest request = entry.request;
                    const bool accepted = entry.accepted;
                    const CompletionHandler handler = entry.completion;
                    const std::shared_ptr<IoAdapterBackend> backend = entry.backend;
                    const IoRequestState state = accepted
                        ? IoRequestState::Timeout
                        : IoRequestState::DeadlineMissed;
                    const IoError error = error_for_state(
                        state,
                        accepted
                            ? "accepted request did not complete before its deadline"
                            : "request was not accepted before its deadline");
                    expired.emplace_back(handler, make_response(request, state, error, {}, now));
                    cancellations.emplace_back(backend, request.request_id);
                    it = pending.erase(it);
                }
                condition.notify_all();
                lock.unlock();
                for (const auto& cancellation : cancellations) {
                    cancellation.first->cancel(cancellation.second);
                }
            }
            for (auto& item : expired) {
                invoke(item.first, std::move(item.second));
            }
        }
    }

    bool cancel_request(IoRequestId request_id) noexcept {
        CompletionHandler handler;
        IoResponse response;
        std::shared_ptr<IoAdapterBackend> backend;
        {
            std::lock_guard<std::mutex> lock(mutex);
            const auto it = pending.find(request_id);
            if (it == pending.end()) {
                return false;
            }
            handler = it->second.completion;
            backend = it->second.backend;
            response = make_response(
                it->second.request,
                IoRequestState::Timeout,
                IoError::timeout("synchronous wrapper reached its bounded wait"));
            pending.erase(it);
            condition.notify_all();
        }
        backend->cancel(request_id);
        invoke(handler, std::move(response));
        return true;
    }

    void shutdown() noexcept {
        std::vector<std::shared_ptr<IoAdapterBackend>> backends;
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (stopping) {
                return;
            }
            stopping = true;
            for (const auto& item : adapters) {
                backends.push_back(item.second);
            }
            adapters.clear();
            pending.clear();
            condition.notify_all();
        }
        for (const auto& backend : backends) {
            backend->stop();
        }
        if (deadline_thread.joinable()) {
            deadline_thread.join();
        }
    }
};

IoAdapterClient::IoAdapterClient(std::chrono::milliseconds sync_wait_limit)
    : impl_(std::make_unique<Impl>(sync_wait_limit)) {
}

IoAdapterClient::~IoAdapterClient() = default;

IoError IoAdapterClient::register_adapter(std::shared_ptr<IoAdapterBackend> adapter) {
    if (!impl_ || !adapter || adapter->adapter_key() == 0) {
        return IoError::invalid_request("a non-null adapter with a non-zero key is required");
    }

    const IoAdapterKey key = adapter->adapter_key();
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        if (impl_->stopping) {
            return IoError::adapter_unavailable("adapter client is stopping");
        }
        if (impl_->adapters.find(key) != impl_->adapters.end()) {
            return IoError::invalid_request("adapter key is already registered");
        }
    }

    IoError start_error;
    try {
        start_error = adapter->start();
    } catch (const std::exception& error) {
        start_error = IoError::transport_error(error.what());
    } catch (...) {
        start_error = IoError::transport_error("adapter start raised an unknown exception");
    }

    bool duplicate_after_start = false;
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        duplicate_after_start = impl_->adapters.find(key) != impl_->adapters.end();
        if (!duplicate_after_start) {
            // Keep a failed-start adapter registered so a request reports the
            // semantic adapter_unavailable state instead of a missing registry
            // entry. The registration error still tells the caller setup failed.
            impl_->adapters.emplace(key, std::move(adapter));
        }
        impl_->condition.notify_all();
    }
    if (duplicate_after_start) {
        adapter->stop();
        return IoError::invalid_request("adapter key is already registered");
    }
    return start_error;
}

IoError IoAdapterClient::unregister_adapter(IoAdapterKey adapter_key) {
    if (!impl_ || adapter_key == 0) {
        return IoError::invalid_request("adapter key must be non-zero");
    }

    std::shared_ptr<IoAdapterBackend> backend;
    std::vector<std::pair<CompletionHandler, IoResponse>> failures;
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        const auto adapter_it = impl_->adapters.find(adapter_key);
        if (adapter_it == impl_->adapters.end()) {
            return IoError::adapter_unavailable("adapter key is not registered");
        }
        backend = adapter_it->second;
        impl_->adapters.erase(adapter_it);

        for (auto it = impl_->pending.begin(); it != impl_->pending.end();) {
            if (it->second.request.adapter_key != adapter_key) {
                ++it;
                continue;
            }
            failures.emplace_back(
                it->second.completion,
                make_response(
                    it->second.request,
                    IoRequestState::AdapterUnavailable,
                    IoError::adapter_unavailable("adapter was unregistered")));
            it = impl_->pending.erase(it);
        }
        impl_->condition.notify_all();
    }

    backend->stop();
    for (auto& failure : failures) {
        Impl::invoke(failure.first, std::move(failure.second));
    }
    return IoError::none();
}

IoRequestResult IoAdapterClient::io_request_async(
    IoAdapterKey adapter_key,
    const IoPayload& payload,
    IoDeadlineNs deadline_ns) {
    return io_request_async(adapter_key, payload, deadline_ns, {});
}

IoRequestResult IoAdapterClient::io_request_async(
    IoAdapterKey adapter_key,
    const IoPayload& payload,
    IoDeadlineNs deadline_ns,
    CompletionHandler completion) {
    if (!impl_) {
        return {};
    }

    const IoRequestId request_id = impl_->allocate_request_id();
    const CompletionHandler handler = impl_->handler_or_default(std::move(completion));
    IoRequest request{adapter_key, request_id, payload, deadline_ns};

    if (adapter_key == 0 || !payload.valid()) {
        const IoError error = IoError::invalid_request("adapter key and non-empty payload are required");
        impl_->settle_immediate(request, IoRequestState::InvalidRequest, error, handler);
        return IoRequestResult(request_id, IoRequestState::InvalidRequest, error);
    }

    const IoDeadlineNs now = monotonic_now_ns();
    if (deadline_ns != 0 && deadline_ns <= now) {
        const IoError error = IoError::deadline_missed("request deadline was already reached at submission");
        impl_->settle_immediate(request, IoRequestState::DeadlineMissed, error, handler);
        return IoRequestResult(request_id, IoRequestState::DeadlineMissed, error);
    }

    std::shared_ptr<IoAdapterBackend> backend;
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        const auto it = impl_->adapters.find(adapter_key);
        if (it != impl_->adapters.end()) {
            backend = it->second;
        }
    }
    if (!backend || !backend->available()) {
        const IoError error = IoError::adapter_unavailable("adapter key is not available");
        impl_->settle_immediate(request, IoRequestState::AdapterUnavailable, error, handler);
        return IoRequestResult(request_id, IoRequestState::AdapterUnavailable, error);
    }

    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        impl_->pending.emplace(
            request_id,
            Impl::Pending{request, backend, handler, false, true, false, {}});
        impl_->condition.notify_all();
    }

    IoError submit_error;
    try {
        submit_error = backend->submit(
            request,
            [client = impl_.get(), request_id](IoResponse response) {
                client->backend_completed(request_id, std::move(response));
            });
    } catch (const std::exception& error) {
        submit_error = IoError::transport_error(error.what());
    } catch (...) {
        submit_error = IoError::transport_error("adapter submit raised an unknown exception");
    }

    if (!submit_error.ok()) {
        {
            std::lock_guard<std::mutex> lock(impl_->mutex);
            impl_->pending.erase(request_id);
            impl_->condition.notify_all();
        }
        const IoRequestState state = state_for_error(submit_error.code);
        impl_->settle_immediate(request, state, submit_error, handler);
        return IoRequestResult(request_id, state, submit_error);
    }

    std::vector<IoResponse> deferred;
    bool missed_while_submitting = false;
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        const auto it = impl_->pending.find(request_id);
        if (it == impl_->pending.end()) {
            // A defensive backend may complete and race with a synchronous
            // cancellation. Its terminal callback has already been emitted.
            return IoRequestResult(request_id, IoRequestState::Accepted, IoError::none());
        }
        it->second.accepted = true;
        it->second.submitting = false;
        missed_while_submitting = it->second.deferred_completions.empty() &&
            request.deadline_ns != 0 && monotonic_now_ns() >= request.deadline_ns;
        if (missed_while_submitting) {
            impl_->pending.erase(it);
        }
        impl_->condition.notify_all();
    }

    if (missed_while_submitting) {
        backend->cancel(request_id);
        const IoError error = IoError::deadline_missed("request was not accepted before its deadline");
        impl_->invoke(
            handler,
            make_response(request, IoRequestState::DeadlineMissed, error));
        return IoRequestResult(request_id, IoRequestState::DeadlineMissed, error);
    }

    impl_->invoke(
        handler,
        make_response(request, IoRequestState::Accepted, IoError::none()));

    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        const auto it = impl_->pending.find(request_id);
        if (it == impl_->pending.end()) {
            return IoRequestResult(request_id, IoRequestState::Accepted, IoError::none());
        }
        it->second.accepted_delivered = true;
        deferred = std::move(it->second.deferred_completions);
        it->second.deferred_completions.clear();
        impl_->condition.notify_all();
    }
    for (auto& response : deferred) {
        impl_->backend_completed(request_id, std::move(response));
    }
    return IoRequestResult(request_id, IoRequestState::Accepted, IoError::none());
}

IoRequestResult IoAdapterClient::io_request_async(
    IoAdapterKey adapter_key,
    IoPayloadView payload,
    IoDeadlineNs deadline_ns,
    CompletionHandler completion) {
    return io_request_async(
        adapter_key,
        IoPayload::copy_of(payload),
        deadline_ns,
        std::move(completion));
}

IoRequestResult IoAdapterClient::io_request_async(
    IoAdapterKey adapter_key,
    const std::vector<std::uint8_t>& payload,
    IoDeadlineNs deadline_ns,
    CompletionHandler completion) {
    return io_request_async(
        adapter_key,
        IoPayloadView{payload.data(), payload.size()},
        deadline_ns,
        std::move(completion));
}

IoRequestResult IoAdapterClient::io_request_async(
    IoAdapterKey adapter_key,
    std::string_view payload,
    IoDeadlineNs deadline_ns,
    CompletionHandler completion) {
    return io_request_async(
        adapter_key,
        IoPayloadView{
            reinterpret_cast<const std::uint8_t*>(payload.data()),
            payload.size()},
        deadline_ns,
        std::move(completion));
}

IoResponse IoAdapterClient::io_request_sync(
    IoAdapterKey adapter_key,
    const IoPayload& payload,
    IoDeadlineNs deadline_ns) {
    struct SyncWait {
        std::mutex mutex;
        std::condition_variable condition;
        std::optional<IoResponse> terminal;
    };
    const auto wait = std::make_shared<SyncWait>();

    const IoRequestResult submission = io_request_async(
        adapter_key,
        payload,
        deadline_ns,
        [wait](IoResponse response) {
            if (!response.terminal()) {
                return;
            }
            std::lock_guard<std::mutex> lock(wait->mutex);
            if (!wait->terminal.has_value()) {
                wait->terminal.emplace(std::move(response));
            }
            wait->condition.notify_all();
        });

    const auto wait_start = SteadyClock::now();
    const auto configured_limit = impl_->sync_wait_limit;
    const auto configured_end = wait_start + configured_limit;
    auto wait_end = configured_end;
    if (deadline_ns != 0) {
        const IoDeadlineNs now = monotonic_now_ns();
        if (deadline_ns <= now) {
            wait_end = wait_start;
        } else {
            const auto deadline_end = wait_start + std::chrono::nanoseconds(deadline_ns - now);
            wait_end = std::min(wait_end, deadline_end);
        }
    }

    {
        std::unique_lock<std::mutex> lock(wait->mutex);
        wait->condition.wait_until(lock, wait_end, [&wait]() {
            return wait->terminal.has_value();
        });
        if (wait->terminal.has_value()) {
            return std::move(*wait->terminal);
        }
    }

    if (submission.request_id() != 0) {
        (void)impl_->cancel_request(submission.request_id());
    }

    {
        std::lock_guard<std::mutex> lock(wait->mutex);
        if (wait->terminal.has_value()) {
            return std::move(*wait->terminal);
        }
    }

    IoRequest request{adapter_key, submission.request_id(), payload, deadline_ns};
    return make_response(
        request,
        IoRequestState::Timeout,
        IoError::timeout("synchronous wrapper reached its bounded wait"));
}

IoResponse IoAdapterClient::io_request_sync(
    IoAdapterKey adapter_key,
    IoPayloadView payload,
    IoDeadlineNs deadline_ns) {
    return io_request_sync(adapter_key, IoPayload::copy_of(payload), deadline_ns);
}

IoResponse IoAdapterClient::io_request_sync(
    IoAdapterKey adapter_key,
    const std::vector<std::uint8_t>& payload,
    IoDeadlineNs deadline_ns) {
    return io_request_sync(
        adapter_key,
        IoPayloadView{payload.data(), payload.size()},
        deadline_ns);
}

IoResponse IoAdapterClient::io_request_sync(
    IoAdapterKey adapter_key,
    std::string_view payload,
    IoDeadlineNs deadline_ns) {
    return io_request_sync(
        adapter_key,
        IoPayloadView{
            reinterpret_cast<const std::uint8_t*>(payload.data()),
            payload.size()},
        deadline_ns);
}

void IoAdapterClient::set_default_completion_handler(CompletionHandler completion) {
    if (!impl_) {
        return;
    }
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->default_completion = std::move(completion);
}

std::size_t IoAdapterClient::pending_request_count() const noexcept {
    if (!impl_) {
        return 0;
    }
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->pending.size();
}
