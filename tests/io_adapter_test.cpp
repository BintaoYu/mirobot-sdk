#include "io_adapter.h"
#include "test_support.h"
#include "tcp_test_support.h"

#include <chrono>
#include <condition_variable>
#include <cerrno>
#include <deque>
#include <string>
#include <map>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

using test_support::TestCase;
using test_support::LoopbackTcpServer;

namespace {

using namespace std::chrono_literals;

std::uint64_t now_ns() {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());
}

IoPayload payload_from(const char* text) {
    const auto* bytes = reinterpret_cast<const std::uint8_t*>(text);
    return IoPayload::copy_of(IoPayloadView{bytes, std::char_traits<char>::length(text)});
}

class FakeEventDrivenAdapter final : public IoAdapterBackend {
public:
    explicit FakeEventDrivenAdapter(IoAdapterKey key) : key_(key) {}

    IoAdapterKey adapter_key() const noexcept override {
        return key_;
    }

    IoError start() override {
        started_ = true;
        return start_error_;
    }

    bool available() const noexcept override {
        return started_ && available_;
    }

    IoError submit(const IoRequest& request, CompletionHandler completion) override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!available()) {
            return IoError::adapter_unavailable("fake adapter is unavailable");
        }
        if (!submit_error_.ok()) {
            return submit_error_;
        }
        requests_[request.request_id] = request;
        completions_[request.request_id] = std::move(completion);
        return IoError::none();
    }

    void cancel(IoRequestId request_id) noexcept override {
        std::lock_guard<std::mutex> lock(mutex_);
        cancelled_.push_back(request_id);
    }

    void stop() noexcept override {
        std::lock_guard<std::mutex> lock(mutex_);
        started_ = false;
    }

    void set_available(bool available) {
        available_ = available;
    }

    void set_submit_error(IoError error) {
        submit_error_ = std::move(error);
    }

    void complete(IoRequestId request_id, IoRequestState state, IoPayload payload = {}) {
        CompletionHandler completion;
        IoRequest request;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            const auto request_it = requests_.find(request_id);
            if (request_it == requests_.end()) {
                return;
            }
            request = request_it->second;
            const auto completion_it = completions_.find(request_id);
            if (completion_it == completions_.end()) {
                return;
            }
            completion = completion_it->second;
        }

        IoResponse response;
        response.adapter_key = request.adapter_key;
        response.request_id = request.request_id;
        response.state = state;
        response.deadline_ns = request.deadline_ns;
        response.payload = std::move(payload);
        response.timestamp_ns = now_ns();
        response.error = state == IoRequestState::Completed
            ? IoError::none()
            : IoError::transport_error("fake terminal state");
        completion(std::move(response));
    }

    IoRequest request(IoRequestId request_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return requests_.at(request_id);
    }

private:
    IoAdapterKey key_;
    mutable std::mutex mutex_;
    std::unordered_map<IoRequestId, IoRequest> requests_;
    std::unordered_map<IoRequestId, CompletionHandler> completions_;
    std::vector<IoRequestId> cancelled_;
    bool started_ = false;
    bool available_ = true;
    IoError start_error_;
    IoError submit_error_;
};

struct ResponseCollector {
    std::mutex mutex;
    std::condition_variable condition;
    std::vector<IoResponse> responses;

    void add(IoResponse response) {
        std::lock_guard<std::mutex> lock(mutex);
        responses.push_back(std::move(response));
        condition.notify_all();
    }

    bool wait_for(std::size_t count, std::chrono::milliseconds timeout = 500ms) {
        std::unique_lock<std::mutex> lock(mutex);
        return condition.wait_for(lock, timeout, [&]() {
            return responses.size() >= count;
        });
    }
};

class EventDrivenComm final : public CommInterface {
public:
    void writeData(const std::string& data) override {
        std::lock_guard<std::mutex> lock(mutex_);
        writes_.push_back(data);
        responses_.push_back("ACK:" + data);
        condition_.notify_all();
    }

    std::string readLine() override {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [&]() {
            return !responses_.empty() || !available_;
        });
        if (responses_.empty()) {
            return {};
        }
        std::string response = std::move(responses_.front());
        responses_.pop_front();
        return response;
    }

    bool dataAvailable() override {
        std::lock_guard<std::mutex> lock(mutex_);
        return !responses_.empty();
    }

    bool isAvailable() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return available_;
    }

    void close() noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        available_ = false;
        condition_.notify_all();
    }

    const std::vector<std::string>& writes() const {
        return writes_;
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::deque<std::string> responses_;
    std::vector<std::string> writes_;
    bool available_ = true;
};

void async_request_emits_accepted_and_completed() {
    auto backend = std::make_shared<FakeEventDrivenAdapter>(41);
    IoAdapterClient client;
    CHECK(client.register_adapter(backend).ok());

    ResponseCollector collector;
    const IoPayload request_payload = payload_from("PING");
    const IoDeadlineNs deadline = now_ns() + (500ms).count() * 1000ULL * 1000ULL;
    const IoRequestResult submission = client.io_request_async(
        41,
        request_payload,
        deadline,
        [&collector](IoResponse response) { collector.add(std::move(response)); });

    CHECK(submission.ok());
    CHECK(submission.request_id() != 0);
    CHECK(backend->request(submission.request_id()).deadline_ns == deadline);
    CHECK(collector.wait_for(1));
    CHECK(collector.responses[0].state == IoRequestState::Accepted);
    CHECK_EQ(collector.responses[0].request_id, submission.request_id());

    backend->complete(submission.request_id(), IoRequestState::Completed, payload_from("PONG"));
    CHECK(collector.wait_for(2));
    CHECK(collector.responses[1].state == IoRequestState::Completed);
    CHECK_EQ(collector.responses[1].request_id, submission.request_id());
    CHECK(collector.responses[1].deadline_ns == deadline);
    CHECK(collector.responses[1].payload.to_vector() == payload_from("PONG").to_vector());
}

void three_argument_async_uses_default_completion_handler() {
    auto backend = std::make_shared<FakeEventDrivenAdapter>(40);
    IoAdapterClient client;
    CHECK(client.register_adapter(backend).ok());

    ResponseCollector collector;
    client.set_default_completion_handler(
        [&collector](IoResponse response) { collector.add(std::move(response)); });
    const IoRequestResult submission = client.io_request_async(
        40,
        std::string_view("DEFAULT"),
        now_ns() + (500ms).count() * 1000ULL * 1000ULL);
    CHECK(submission.ok());
    CHECK(collector.wait_for(1));
    backend->complete(submission.request_id(), IoRequestState::Completed, payload_from("DEFAULT-OK"));
    CHECK(collector.wait_for(2));
    CHECK(collector.responses[0].state == IoRequestState::Accepted);
    CHECK(collector.responses[1].state == IoRequestState::Completed);
}

void multiple_request_ids_are_matched_by_id() {
    auto backend = std::make_shared<FakeEventDrivenAdapter>(42);
    IoAdapterClient client;
    CHECK(client.register_adapter(backend).ok());

    ResponseCollector collector;
    const IoPayload payload = payload_from("REQUEST");
    const IoRequestResult first = client.io_request_async(
        42, payload, now_ns() + (1s).count() * 1000ULL * 1000ULL * 1000ULL,
        [&collector](IoResponse response) { collector.add(std::move(response)); });
    const IoRequestResult second = client.io_request_async(
        42, payload, now_ns() + (1s).count() * 1000ULL * 1000ULL * 1000ULL,
        [&collector](IoResponse response) { collector.add(std::move(response)); });

    CHECK(first.request_id() != second.request_id());
    CHECK(collector.wait_for(2));
    backend->complete(second.request_id(), IoRequestState::Completed, payload_from("SECOND"));
    backend->complete(first.request_id(), IoRequestState::Completed, payload_from("FIRST"));
    CHECK(collector.wait_for(4));

    std::map<IoRequestId, std::vector<std::uint8_t>> terminal_payloads;
    for (const auto& response : collector.responses) {
        if (response.terminal()) {
            terminal_payloads[response.request_id] = response.payload.to_vector();
        }
    }
    CHECK(terminal_payloads[first.request_id()] == payload_from("FIRST").to_vector());
    CHECK(terminal_payloads[second.request_id()] == payload_from("SECOND").to_vector());
}

void timeout_is_delivered_by_deadline_coordinator() {
    auto backend = std::make_shared<FakeEventDrivenAdapter>(43);
    IoAdapterClient client;
    CHECK(client.register_adapter(backend).ok());

    ResponseCollector collector;
    const IoRequestResult submission = client.io_request_async(
        43,
        payload_from("NO-RESPONSE"),
        now_ns() + (35ms).count() * 1000ULL * 1000ULL,
        [&collector](IoResponse response) { collector.add(std::move(response)); });
    CHECK(submission.ok());
    CHECK(collector.wait_for(2, 500ms));
    CHECK(collector.responses.back().state == IoRequestState::Timeout);
    CHECK(collector.responses.back().error.code == IoErrorCode::Timeout);
}

void expired_request_is_deadline_missed_before_submit() {
    auto backend = std::make_shared<FakeEventDrivenAdapter>(44);
    IoAdapterClient client;
    CHECK(client.register_adapter(backend).ok());

    ResponseCollector collector;
    const IoRequestResult submission = client.io_request_async(
        44,
        payload_from("LATE"),
        now_ns() - 1,
        [&collector](IoResponse response) { collector.add(std::move(response)); });
    CHECK(!submission.ok());
    CHECK(submission.state() == IoRequestState::DeadlineMissed);
    CHECK(collector.wait_for(1));
    CHECK(collector.responses[0].state == IoRequestState::DeadlineMissed);
    CHECK(client.pending_request_count() == static_cast<std::size_t>(0));
}

void transport_failure_is_not_reported_as_timeout() {
    auto backend = std::make_shared<FakeEventDrivenAdapter>(45);
    backend->set_submit_error(IoError::transport_error("write failed",  EIO));
    IoAdapterClient client;
    CHECK(client.register_adapter(backend).ok());

    ResponseCollector collector;
    const IoRequestResult submission = client.io_request_async(
        45,
        payload_from("FAIL"),
        now_ns() + (1s).count() * 1000ULL * 1000ULL * 1000ULL,
        [&collector](IoResponse response) { collector.add(std::move(response)); });
    CHECK(!submission.ok());
    CHECK(submission.state() == IoRequestState::TransportError);
    CHECK(collector.wait_for(1));
    CHECK(collector.responses[0].state == IoRequestState::TransportError);
    CHECK(collector.responses[0].error.native_code == EIO);
}

void unavailable_and_invalid_requests_are_typed() {
    auto unavailable_backend = std::make_shared<FakeEventDrivenAdapter>(46);
    unavailable_backend->set_available(false);
    IoAdapterClient client;
    CHECK(client.register_adapter(unavailable_backend).ok());

    ResponseCollector collector;
    const IoRequestResult unavailable = client.io_request_async(
        46,
        payload_from("DOWN"),
        0,
        [&collector](IoResponse response) { collector.add(std::move(response)); });
    CHECK(!unavailable.ok());
    CHECK(unavailable.state() == IoRequestState::AdapterUnavailable);
    CHECK(collector.wait_for(1));

    const IoRequestResult invalid = client.io_request_async(
        46,
        IoPayload{},
        0,
        [&collector](IoResponse response) { collector.add(std::move(response)); });
    CHECK(!invalid.ok());
    CHECK(invalid.state() == IoRequestState::InvalidRequest);
    CHECK(collector.wait_for(2));
    CHECK(collector.responses[0].state == IoRequestState::AdapterUnavailable);
    CHECK(collector.responses[1].state == IoRequestState::InvalidRequest);
}

void synchronous_wrapper_returns_terminal_response() {
    auto backend = std::make_shared<FakeEventDrivenAdapter>(47);
    IoAdapterClient client(500ms);
    CHECK(client.register_adapter(backend).ok());

    IoResponse response;
    std::thread completer([&]() {
        while (client.pending_request_count() == 0) {
            std::this_thread::yield();
        }
        const IoRequestId request_id = backend->request(1).request_id;
        backend->complete(request_id, IoRequestState::Completed, payload_from("SYNC-OK"));
    });
    response = client.io_request_sync(
        47,
        payload_from("SYNC"),
        now_ns() + (1s).count() * 1000ULL * 1000ULL * 1000ULL);
    completer.join();

    CHECK(response.state == IoRequestState::Completed);
    CHECK(response.ok());
    CHECK(response.payload.to_vector() == payload_from("SYNC-OK").to_vector());
}

void synchronous_wrapper_exposes_its_bounded_timeout() {
    auto backend = std::make_shared<FakeEventDrivenAdapter>(39);
    IoAdapterClient client(30ms);
    CHECK(client.register_adapter(backend).ok());

    const IoResponse response = client.io_request_sync(39, payload_from("NO-RESPONSE"), 0);
    CHECK(response.state == IoRequestState::Timeout);
    CHECK(response.error.code == IoErrorCode::Timeout);
    CHECK(!response.ok());
}

void shared_payload_keeps_handle_without_copying_bytes() {
    auto backend = std::make_shared<FakeEventDrivenAdapter>(48);
    IoAdapterClient client;
    CHECK(client.register_adapter(backend).ok());

    auto bytes = std::make_shared<const std::vector<std::uint8_t>>(
        std::initializer_list<std::uint8_t>{1, 2, 3, 4});
    const IoPayload payload = IoPayload::from_shared(bytes);
    const IoRequestResult submission = client.io_request_async(48, payload, 0);
    CHECK(submission.ok());
    const IoRequest captured = backend->request(submission.request_id());
    CHECK(captured.payload.data() == bytes->data());
    CHECK_EQ(captured.payload.size(), bytes->size());
}

void comm_interface_adapter_delivers_event_driven_response() {
    EventDrivenComm comm;
    auto adapter = std::make_shared<CommInterfaceIoAdapter>(49, comm);
    IoAdapterClient client;
    CHECK(client.register_adapter(adapter).ok());

    ResponseCollector collector;
    const IoRequestResult submission = client.io_request_async(
        49,
        std::string_view("TX"),
        now_ns() + (500ms).count() * 1000ULL * 1000ULL,
        [&collector](IoResponse response) { collector.add(std::move(response)); });
    CHECK(submission.ok());
    CHECK(collector.wait_for(2));
    CHECK(collector.responses[0].state == IoRequestState::Accepted);
    CHECK(collector.responses[1].state == IoRequestState::Completed);
    CHECK(collector.responses[1].payload.to_vector() == payload_from("ACK:TX").to_vector());
    CHECK(comm.writes().size() == static_cast<std::size_t>(1));
    CHECK(comm.writes()[0] == "TX");
}

void tcp_client_adapter_reuses_tcp_comm() {
    if (!test_support::canCreateTcpSockets()) {
        return;
    }

    LoopbackTcpServer server;
    std::string server_error;
    std::thread server_thread([&]() {
        try {
            if (server.readLine() != "PING") {
                server_error = "TCP adapter sent an unexpected request";
                return;
            }
            server.sendRaw("PONG\n");
        } catch (const std::exception& error) {
            server_error = error.what();
        }
    });

    auto adapter = std::make_shared<TcpClientIoAdapter>(50, "127.0.0.1", server.port());
    IoAdapterClient client;
    const IoError registration = client.register_adapter(adapter);
    if (!registration.ok()) {
        server_thread.join();
        CHECK(false);
        return;
    }

    ResponseCollector collector;
    const IoRequestResult submission = client.io_request_async(
        50,
        std::string_view("PING"),
        now_ns() + (1s).count() * 1000ULL * 1000ULL * 1000ULL,
        [&collector](IoResponse response) { collector.add(std::move(response)); });
    if (!submission.ok()) {
        server_thread.join();
        CHECK(false);
        return;
    }
    const bool response_seen = collector.wait_for(2);

    server_thread.join();
    CHECK(response_seen);
    CHECK(!collector.responses.empty());
    CHECK(collector.responses.back().state == IoRequestState::Completed);
    CHECK(collector.responses.back().payload.to_vector() == payload_from("PONG").to_vector());
    CHECK(server_error.empty());
}

} // namespace

int main() {
    const std::vector<TestCase> tests = {
        {"I/O async emits accepted and completed", async_request_emits_accepted_and_completed},
        {"I/O three-argument async uses default handler", three_argument_async_uses_default_completion_handler},
        {"I/O request IDs match out-of-order responses", multiple_request_ids_are_matched_by_id},
        {"I/O deadline coordinator emits timeout", timeout_is_delivered_by_deadline_coordinator},
        {"I/O expired request is deadline missed", expired_request_is_deadline_missed_before_submit},
        {"I/O transport failure is typed", transport_failure_is_not_reported_as_timeout},
        {"I/O unavailable and invalid requests are typed", unavailable_and_invalid_requests_are_typed},
        {"I/O sync wrapper returns terminal response", synchronous_wrapper_returns_terminal_response},
        {"I/O sync wrapper exposes bounded timeout", synchronous_wrapper_exposes_its_bounded_timeout},
        {"I/O shared payload preserves its byte handle", shared_payload_keeps_handle_without_copying_bytes},
        {"I/O CommInterface adapter is event driven", comm_interface_adapter_delivers_event_driven_response},
        {"I/O TCP adapter reuses TcpComm", tcp_client_adapter_reuses_tcp_comm},
    };
    return test_support::runTests(tests);
}
