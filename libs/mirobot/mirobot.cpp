#include "mirobot.h"
#include "shared_udp_comm.h"
#include <chrono>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <regex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

// 将双精度浮点数转换为字符串
std::string doubleToString(double dbNum)
{
    char chCode[25];
    std::snprintf(chCode, sizeof(chCode), "%.2lf", dbNum);
    return std::string(chCode);
}


// 从字符串中提取浮点数并存储到数组中
void getMatch(const std::string& instr, double data[])
{
    std::regex target_float("-?[0-9]+\\.[0-9]{3}");
    auto words_begin = std::sregex_iterator(instr.begin(), instr.end(), target_float);
    auto words_end = std::sregex_iterator();
    int num = 0;
    for (auto i = words_begin; i != words_end; ++i)
    {
        std::smatch match = *i;
        std::string match_str = match.str();
        double match_double = std::stod(match_str);
        data[num++] = match_double;
    }
}

// 重置Mirobot机器人
void mirobot_reset(Mirobot &m)
{
    const std::string msg = "$H";
    m.send_msg(msg, true);
    std::this_thread::sleep_for(std::chrono::seconds(13));
}

// 从字符串中提取带逗号的浮点数并存储到数组中
int translation(const std::string& instr, double data[])
{
    std::regex target_float("-?[0-9]+(\\.[0-9]+)?,");
    auto words_begin = std::sregex_iterator(instr.begin(), instr.end(), target_float);
    auto words_end = std::sregex_iterator();
    int num = 0;
    for (auto i = words_begin; i != words_end; ++i)
    {
        std::smatch match = *i;
        std::string match_str = match.str();
        double match_double = std::stod(match_str);
        data[num++] = match_double;
    }
    return num;
}

// Mirobot构造函数
Mirobot::Mirobot(DeviceSession session, bool debugFlag)
    : debug(debugFlag), owned_comm_(), session_(std::move(session)), time_temp{} {
}

Mirobot::Mirobot(CommInterface& comm, int devicePort, bool debugFlag, std::chrono::milliseconds timeout)
    : debug(debugFlag), owned_comm_(), session_(comm, devicePort, "", timeout), time_temp{} {
}

Mirobot::Mirobot(
    const std::string& targetIp,
    int targetPort,
    int devicePort,
    bool debugFlag,
    std::chrono::milliseconds timeout)
    : debug(debugFlag),
      owned_comm_(acquireSharedUdpComm(targetIp, targetPort)),
      session_(*owned_comm_, devicePort, "", timeout),
      time_temp{} {
}

// 读取Mirobot状态
std::string Mirobot::read_status()
{
    return session_.readAvailable();
}

// 读取一行数据
std::string Mirobot::readline()
{
    return session_.readLine();
}

// 获取当前时间并格式化
void Mirobot::get_time()
{
    time_t t = time(0);
    tm time_info = {};
#ifdef _WIN32
    localtime_s(&time_info, &t);
#else
    tm* local_time = localtime(&t);
    if (local_time != nullptr) {
        time_info = *local_time;
    }
#endif
    strftime(time_temp, sizeof(time_temp), "%Y-%m-%d %H:%M:%S", &time_info);
    status.time = time_temp;
}

// 发送消息到Mirobot
void Mirobot::send_msg(const std::string& msg, bool wait, int delay)
{
    const std::string full_msg = session_.buildCommand(msg);
    if (debug) {
        std::printf("message sent: %s\n", full_msg.c_str());
    }

    if (wait)
    {
        std::cout << "receive:" << session_.sendAndWait(msg) << std::endl;
        return;
    }

    session_.send(msg);
    if (delay > 0) {
        std::this_thread::sleep_for(std::chrono::seconds(delay));
    }
}

std::string Mirobot::camera_msg(const std::string& msg, bool wait, int delay) {
    if (debug) {
        std::printf("message sent: %s\n", msg.c_str());
    }

    if (wait)
    {
        return session_.sendRawAndWait(msg);
    }

    session_.sendRaw(msg);
    if (delay > 0) {
        std::this_thread::sleep_for(std::chrono::seconds(delay));
    }

    return "";
}

// 等待串口结束
bool Mirobot::waitForEnd(std::chrono::milliseconds timeout)
{
    return session_.waitForData(timeout);
}

// 复位每个轴
void Mirobot::home_individual()
{
    const std::string msg = "$HH";
    send_msg(msg, true);
}

// 同时复位所有轴
void Mirobot::home_simultaneous()
{
    const std::string msg = "$H";
    send_msg(msg, true);
}

// 解锁轴
void Mirobot::unlock_shaft()
{
    const std::string msg = "M50";
    send_msg(msg, true);
}

// 设定运动速度
void Mirobot::set_speed(int speed)
{
    const std::string msg = "F" + std::to_string(speed);
    send_msg(msg, true);
}

// 关节旋转到指定位置
void Mirobot::go_to_axis(double a1, double a2, double a3, double a4, double a5, double a6)
{
    std::string msg = "M21 G90";
    msg += " X" + doubleToString(a1);
    msg += " Y" + doubleToString(a2);
    msg += " Z" + doubleToString(a3);
    msg += " A" + doubleToString(a4);
    msg += " B" + doubleToString(a5);
    msg += " C" + doubleToString(a6);
    msg += " F2000";
    send_msg(msg, true);
}

// 回归零位
void Mirobot::go_to_zero(int delay)
{
    go_to_axis(0, 0, 0, 0, 0, 0);
    std::this_thread::sleep_for(std::chrono::seconds(delay));
}

// G1参数速度
// G0最大速度
// 增量移动各轴
void Mirobot::increment_axis(double a1, double a2, double a3, double a4, double a5, double a6)
{
    std::string msg = "M21 G91 G1";
    msg += " X" + doubleToString(a1);
    msg += " Y" + doubleToString(a2);
    msg += " Z" + doubleToString(a3);
    msg += " A" + doubleToString(a4);
    msg += " B" + doubleToString(a5);
    msg += " C" + doubleToString(a6);
    send_msg(msg, true);
}

// 移动到坐标位置
void Mirobot::go_to_cartesian_lin(int m, double x, double y, double z, double a, double b, double c, int delay)
{
    std::string msg = "M20 G90";
    if (m == motiontype.MOVJ)
        msg += " G01";
    else if (m == motiontype.MOVL)
        msg += " G00";
    else if (m == motiontype.JUMP)
        msg += " G05";
    else
    {
        std::printf("m: %d: ", m);
        return;
    }
    msg += " X" + doubleToString(x);
    msg += " Y" + doubleToString(y);
    msg += " Z" + doubleToString(z);
    msg += " A" + doubleToString(a);
    msg += " B" + doubleToString(b);
    msg += " C" + doubleToString(c);
    send_msg(msg, true, delay);
}

// 笛卡尔空间中的线性增量
void Mirobot::increment_cartesian_lin(int m, double x, double y, double z, double a, double b, double c, int delay)
{
    std::string msg = "M20 G91";
    if (m == motiontype.MOVJ)
        msg += " G1";
    else if (m == motiontype.MOVL)
        msg += " G0";
    else if (m == motiontype.JUMP)
        msg += " G5";
    else
        return;
    msg += " X" + doubleToString(x);
    msg += " Y" + doubleToString(y);
    msg += " Z" + doubleToString(z);
    msg += " A" + doubleToString(a);
    msg += " B" + doubleToString(b);
    msg += " C" + doubleToString(c);
    send_msg(msg, false, delay);
}

// 吸盘开
void Mirobot::suction_cup_on(int delay)
{
    send_msg("M3S1000", false, delay);
}

// 吸盘关
void Mirobot::suction_cup_off(int delay)
{
    send_msg("M3S0", false, delay);
}

// 吸盘吹
void Mirobot::suction_cup_blow(int delay)
{
    send_msg("M3S500", false, delay);
}

// 夹具开
void Mirobot::gripper_on()
{
    send_msg("M3S40");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    send_msg("M4E45");
}

// 夹具关
void Mirobot::gripper_off()
{
    send_msg("M3S60");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    send_msg("M4E65");
}

// 设置空气泵的PWM
void Mirobot::set_air_pump(int pwm)
{
    const std::string msg = "M3S" + std::to_string(pwm);
    send_msg(msg);
}

// 设置夹具的PWM
void Mirobot::set_gripper(int pwm)
{
    const std::string msg = "M4E" + std::to_string(pwm);
    send_msg(msg);
}

// 延时执行秒
void Mirobot::set_delay_time(double t)
{
    const std::string msg = "G4 P" + doubleToString(t);
    send_msg(msg);
}

// 滑轨移动到
void Mirobot::slider_move_to(double x, int speed)
{
    std::string msg = "G90 G01";
    msg += " D" + doubleToString(x) + " F" + std::to_string(speed);
    send_msg(msg);
}

// 传送带移动到
void Mirobot::conveyor_move_to(int c, double n, int speed)
{
    std::string msg = "";
    if (movemode.relative == c)
        msg += "G91 G01";
    else if (movemode.absolute == c)
        msg += "G90 G01";
    else
        return;
    msg += " D" + doubleToString(n) + " F" + std::to_string(speed);
    send_msg(msg);
}

// 关节移动
void Mirobot::move_to_axis(int joint, int revolve, double n)
{
    std::string msg = "M21 G91 G01 ";
    if (revolvedirection.ccw == revolve)
        n = -n;
    if (mirobotjoint.Joint1 == joint)
        msg += "X" + doubleToString(n);
    else if (mirobotjoint.Joint2 == joint)
        msg += "Y" + doubleToString(n);
    else if (mirobotjoint.Joint3 == joint)
        msg += "Z" + doubleToString(n);
    else if (mirobotjoint.Joint4 == joint)
        msg += "A" + doubleToString(n);
    else if (mirobotjoint.Joint5 == joint)
        msg += "B" + doubleToString(n);
    else if (mirobotjoint.Joint6 == joint)
        msg += "C" + doubleToString(n);
    else
        return;
    send_msg(msg);
}

// 定向移动
void Mirobot::direction_mobility(int d, double n, int delay)
{
    std::string msg = "M20 G91 G01 ";
    double x = -n;
    if (movedirection.dforward == d)
        msg += "Y" + doubleToString(n);
    else if (movedirection.dbackward == d)
        msg += "Y" + doubleToString(x);
    else if (movedirection.dup == d)
        msg += "Z" + doubleToString(n);
    else if (movedirection.ddown == d)
        msg += "Z" + doubleToString(x);
    else if (movedirection.dright == d)
        msg += "X" + doubleToString(n);
    else if (movedirection.dleft == d)
        msg += "X" + doubleToString(x);
    else
        return;
    send_msg(msg, true, delay);
}

// 门型轨迹移动，只用于笛卡尔模式
void Mirobot::jump_move(int m, double x, double y, double z, double a, double b, double c)
{
    std::string msg = "M20 ";
    if (movemode.relative == m)
        msg += "G91 G05";
    else if (movemode.absolute == m)
        msg += "G90 G05";
    else
        return;
    msg += " X" + doubleToString(x) + " Y" + doubleToString(y) + " Z" + doubleToString(z) + " A" + doubleToString(a) + " B" + doubleToString(b) + " C" + doubleToString(c);
    send_msg(msg);
}

void Mirobot::set_arc_move(int mode, int revolve, double x, double y, double z, double r)
{
    std::string msg = "M20 ";
    if (movemode.relative == mode)
        msg += "G91 ";
    else if (movemode.absolute == mode)
        msg += "G90 ";
    else
        return;
    if (revolvedirection.cw == revolve)
        msg += "G2 ";
    else if (revolvedirection.ccw == revolve)
        msg += "G3 ";
    else
        return;
    msg += "X" + doubleToString(x) + " Y" + doubleToString(y) + " Z" + doubleToString(z) + " R" + doubleToString(r);
    send_msg(msg);
}

void Mirobot::get_status()
{
    const std::string readstr = session_.sendAndWait("?");
    if (readstr.empty() || readstr.front() != '<')
    {
        throw std::runtime_error("Unexpected Mirobot status payload: " + readstr);
    }

    std::cout << "feedback : " << readstr << std::endl;
    status.moving = (readstr[1] != 'I');

    double state[13] = {};
    getMatch(readstr, state);
    status.A = state[0];
    status.B = state[1];
    status.C = state[2];
    status.D = state[3];
    status.X = state[4];
    status.Y = state[5];
    status.Z = state[6];
    status.CCX = state[7];
    status.CCY = state[8];
    status.CCZ = state[9];
    status.CCRX = state[10];
    status.CCRY = state[11];
    status.CCRZ = state[12];

    const std::size_t num = readstr.find('>');
    if (num != std::string::npos && num >= 27) {
        status.PumpPWM = static_cast<int>(readstr[num - 27]);
    }
    if (num != std::string::npos && num >= 15) {
        status.ValvePWM = static_cast<int>(readstr[num - 15]);
    }

    get_time();
}

Mirobot::~Mirobot()
{
    std::cout << std::endl;
    std::printf("program over\n");
}
