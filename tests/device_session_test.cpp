#include "device_session.h"
#include "test_support.h"
#include <chrono>
#include <stdexcept>
#include <vector>

using test_support::MockComm;
using test_support::TestCase;

namespace {

void buildCommand_uses_configured_separator() {
    MockComm comm;
    DeviceSession motor_session(comm, 10, " ");
    DeviceSession mirobot_session(comm, 2);

    CHECK_EQ(motor_session.buildCommand("G6 F1000"), std::string("@10 G6 F1000"));
    CHECK_EQ(mirobot_session.buildCommand("F1500"), std::string("@2F1500"));
}

void sendAndWait_drains_all_available_lines() {
    MockComm comm;
    comm.enqueueResponseBatch({"ok", "done"});

    DeviceSession session(comm, 7, " ");
    const std::string response = session.sendAndWait("G6 F1000");

    CHECK_EQ(comm.writes().size(), static_cast<std::size_t>(1));
    CHECK_EQ(comm.writes()[0], std::string("@7 G6 F1000"));
    CHECK_EQ(response, std::string("okdone"));
}

void sendAndWait_throws_when_timeout_expires() {
    MockComm comm;
    DeviceSession session(comm, 5);

    CHECK_THROWS_AS(session.sendAndWait("STATUS?", std::chrono::milliseconds(5)), std::runtime_error);
}

void sendRawAndWait_preserves_unprefixed_payload() {
    MockComm comm;
    comm.enqueueResponseBatch({"camera-ready"});

    DeviceSession session(comm, 2);
    const std::string response = session.sendRawAndWait("STATUS?");

    CHECK_EQ(comm.writes().size(), static_cast<std::size_t>(1));
    CHECK_EQ(comm.writes()[0], std::string("STATUS?"));
    CHECK_EQ(response, std::string("camera-ready"));
}

} // namespace

int main() {
    const std::vector<TestCase> tests = {
        {"DeviceSession builds prefixed commands", buildCommand_uses_configured_separator},
        {"DeviceSession::sendAndWait drains available lines", sendAndWait_drains_all_available_lines},
        {"DeviceSession::sendAndWait times out cleanly", sendAndWait_throws_when_timeout_expires},
        {"DeviceSession::sendRawAndWait keeps payloads unprefixed", sendRawAndWait_preserves_unprefixed_payload},
    };

    return test_support::runTests(tests);
}
