#include "motor.h"
#include "test_support.h"
#include <chrono>
#include <exception>
#include <stdexcept>
#include <vector>

using test_support::MockComm;
using test_support::TestCase;

namespace {

void constructor_accepts_session_object() {
    MockComm comm;
    DeviceSession session(comm, 10, " ");

    Motor motor(session);
    motor.send_msg("G6 F500", false);

    CHECK_EQ(comm.writes().size(), static_cast<std::size_t>(1));
    CHECK_EQ(comm.writes()[0], std::string("@10 G6 F500"));
}

void set_speed_formats_prefixed_command() {
    MockComm comm;
    comm.enqueueResponseBatch({"ok"});

    Motor motor(comm, 10);
    motor.set_speed(1000);

    CHECK_EQ(comm.writes().size(), static_cast<std::size_t>(1));
    CHECK_EQ(comm.writes()[0], std::string("@10 G6 F1000"));
    CHECK(!comm.dataAvailable());
}

void load_sends_reset_then_speed_command() {
    MockComm comm;
    comm.enqueueResponseBatch({"reset-ok"});
    comm.enqueueResponseBatch({"speed-ok"});

    Motor motor(comm, 7);
    motor.load(250);

    CHECK_EQ(comm.writes().size(), static_cast<std::size_t>(2));
    CHECK_EQ(comm.writes()[0], std::string("@7 $N2=G06 F0"));
    CHECK_EQ(comm.writes()[1], std::string("@7 G6 F250"));
}

void set_speed_timeout_does_not_stop_later_commands() {
    MockComm comm;
    Motor motor(comm, 10, false, std::chrono::milliseconds(1));

    try {
        motor.set_speed(0);
        motor.set_speed(250);
    } catch (const std::exception& error) {
        test_support::fail(
            __FILE__,
            __LINE__,
            std::string("Motor timeout escaped the device library: ") + error.what());
    } catch (...) {
        test_support::fail(__FILE__, __LINE__, "Motor timeout escaped the device library");
    }

    CHECK_EQ(comm.writes().size(), static_cast<std::size_t>(2));
    CHECK_EQ(comm.writes()[0], std::string("@10 G6 F0"));
    CHECK_EQ(comm.writes()[1], std::string("@10 G6 F250"));
}

void read_status_concatenates_all_available_lines() {
    MockComm comm;
    comm.enqueueRead("line-a");
    comm.enqueueRead("line-b");

    Motor motor(comm, 3);
    CHECK_EQ(motor.read_status(), std::string("line-aline-b"));
}

} // namespace

int main() {
    const std::vector<TestCase> tests = {
        {"Motor accepts an injected DeviceSession", constructor_accepts_session_object},
        {"Motor::set_speed formats the expected command", set_speed_formats_prefixed_command},
        {"Motor::load sends reset and speed commands", load_sends_reset_then_speed_command},
        {"Motor timeout does not stop later commands", set_speed_timeout_does_not_stop_later_commands},
        {"Motor::read_status concatenates queued lines", read_status_concatenates_all_available_lines},
    };

    return test_support::runTests(tests);
}
