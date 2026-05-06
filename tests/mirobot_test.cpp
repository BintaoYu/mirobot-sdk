#include "mirobot.h"
#include "test_support.h"
#include <stdexcept>
#include <vector>

using test_support::MockComm;
using test_support::TestCase;

namespace {

void constructor_accepts_session_object() {
    MockComm comm;
    DeviceSession session(comm, 2);

    Mirobot mirobot(session);
    mirobot.send_msg("M50", false);

    CHECK_EQ(comm.writes().size(), static_cast<std::size_t>(1));
    CHECK_EQ(comm.writes()[0], std::string("@2M50"));
}

void helper_functions_parse_and_format_values() {
    double value = 12.345;
    CHECK_EQ(doubleToString(value), std::string("12.35"));

    double parsed[3] = {};
    CHECK_EQ(translation("X1.25,Y-2.50,Z3.00,", parsed), 3);
    CHECK_EQ(parsed[0], 1.25);
    CHECK_EQ(parsed[1], -2.50);
    CHECK_EQ(parsed[2], 3.00);
}

void set_speed_sends_device_prefixed_command() {
    MockComm comm;
    comm.enqueueResponseBatch({"speed-ok"});

    Mirobot mirobot(comm, 2);
    mirobot.set_speed(1500);

    CHECK_EQ(comm.writes().size(), static_cast<std::size_t>(1));
    CHECK_EQ(comm.writes()[0], std::string("@2F1500"));
}

void go_to_axis_formats_all_coordinates() {
    MockComm comm;
    comm.enqueueResponseBatch({"move-ok"});

    Mirobot mirobot(comm, 2);
    mirobot.go_to_axis(1.0, 2.5, -3.0, 4.25, 5.0, 6.0);

    CHECK_EQ(comm.writes().size(), static_cast<std::size_t>(1));
    CHECK_EQ(
        comm.writes()[0],
        std::string("@2M21 G90 X1.00 Y2.50 Z-3.00 A4.25 B5.00 C6.00 F2000"));
}

void increment_cartesian_lin_uses_nonblocking_message_path() {
    MockComm comm;

    Mirobot mirobot(comm, 9);
    mirobot.increment_cartesian_lin(mirobot.motiontype.MOVL, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 0);

    CHECK_EQ(comm.writes().size(), static_cast<std::size_t>(1));
    CHECK_EQ(
        comm.writes()[0],
        std::string("@9M20 G91 G0 X1.00 Y2.00 Z3.00 A4.00 B5.00 C6.00"));
}

void camera_msg_returns_mock_response() {
    MockComm comm;
    comm.enqueueResponseBatch({"camera-ready"});

    Mirobot mirobot(comm, 1);
    const std::string response = mirobot.camera_msg("STATUS?", true, 0);

    CHECK_EQ(response, std::string("camera-ready"));
    CHECK_EQ(comm.writes().size(), static_cast<std::size_t>(1));
    CHECK_EQ(comm.writes()[0], std::string("STATUS?"));
}

} // namespace

int main() {
    const std::vector<TestCase> tests = {
        {"Mirobot accepts an injected DeviceSession", constructor_accepts_session_object},
        {"Mirobot helper functions parse and format numeric values", helper_functions_parse_and_format_values},
        {"Mirobot::set_speed sends the expected command", set_speed_sends_device_prefixed_command},
        {"Mirobot::go_to_axis formats all six axes", go_to_axis_formats_all_coordinates},
        {"Mirobot::increment_cartesian_lin sends a nonblocking command", increment_cartesian_lin_uses_nonblocking_message_path},
        {"Mirobot::camera_msg returns the queued response", camera_msg_returns_mock_response},
    };

    return test_support::runTests(tests);
}
