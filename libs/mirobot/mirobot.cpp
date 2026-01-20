#include "mirobot.h"
#include <iostream>
#include <regex>
#include <string>
#include <ctime>
#include <unistd.h>

// 将双精度浮点数转换为字符串
string doubleToString(double &dbNum)
{
    char chCode[25];
    sprintf(chCode, "%.2lf", dbNum);
    return string(chCode);
}


// 从字符串中提取浮点数并存储到数组中
void getMatch(std::string instr, double data[])
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
    string msg = "$H";
    m.send_msg(msg, true);
    sleep(13);
}

// 从字符串中提取带逗号的浮点数并存储到数组中
int translation(std::string instr, double data[])
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
Mirobot::Mirobot(CommInterface* commPtr, int devicePort, bool debugFlag) : comm(commPtr), debug(debugFlag) {
    if (commPtr == nullptr)
        throw std::invalid_argument("CommInterface pointer cannot be null");
}

// 读取Mirobot状态
string Mirobot::read_status()
{
    string read_status = "";
    while (comm->dataAvailable())
    {
        read_status += comm->readLine();
    }
    return read_status;
}

// 读取一行数据
string Mirobot::readline()
{
    return comm->readLine();
}

// 获取当前时间并格式化
void Mirobot::get_time()
{
    time_t t = time(0);
    strftime(time_temp, sizeof(time_temp), "%Y-%m-%d %H:%M:%S", localtime(&t));
    status.time = time_temp;
}

// 发送消息到Mirobot
void Mirobot::send_msg(string msg, bool wait, int delay)
{
    comm->writeData(msg);
    if (debug)
        printf("message sent: %s\n", msg.c_str());
    if (wait)
    {
        cout << "waiting" << endl;
        waitForEnd();
        cout << read_status() << endl;
    }
    sleep(delay);
}

string Mirobot::camera_msg(string msg, bool wait, int delay) {
    comm->writeData(msg);
    if (debug)
        printf("message sent: %s\n", msg.c_str());
    if (wait)
    {
        cout << "waiting" << endl;
        waitForEnd();
        return read_status();
    }
    sleep(delay);
}

// 等待串口结束
void Mirobot::waitForEnd()
{
    while (!comm->dataAvailable())
    {
        usleep(1000000);
        cout << "waiting" << endl;
    }
}

// 复位每个轴
void Mirobot::home_individual()
{
    string msg = "$HH";
    send_msg(msg, true);
}

// 同时复位所有轴
void Mirobot::home_simultaneous()
{
    string msg = "$H";
    send_msg(msg, true);
}

// 解锁轴
void Mirobot::unlock_shaft()
{
    string msg = "M50";
    send_msg(msg, true);
}

// 设定运动速度
void Mirobot::set_speed(int speed)
{
    string msg = "F" + to_string(speed);
    send_msg(msg, true);
}

// 关节旋转到指定位置
void Mirobot::go_to_axis(double a1, double a2, double a3, double a4, double a5, double a6)
{
    string msg = "M21 G90";
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
    sleep(delay);
}

// G1参数速度
// G0最大速度
// 增量移动各轴
void Mirobot::increment_axis(double a1, double a2, double a3, double a4, double a5, double a6)
{
    string msg = "M21 G91 G1";
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
    string msg = "M20 G90";
    if (m == motiontype.MOVJ)
        msg += " G01";
    else if (m == motiontype.MOVL)
        msg += " G00";
    else if (m == motiontype.JUMP)
        msg += " G05";
    else
    {
        printf("m: %d: ", m);
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
    string msg = "M20 G91";
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
    sleep(1);
    send_msg("M4E45");
}

// 夹具关
void Mirobot::gripper_off()
{
    send_msg("M3S60");
    sleep(1);
    send_msg("M4E65");
}

// 设置空气泵的PWM
void Mirobot::set_air_pump(int pwm)
{
    string msg = "M3S" + to_string(pwm);
    send_msg(msg);
}

// 设置夹具的PWM
void Mirobot::set_gripper(int pwm)
{
    string msg = "M4E" + to_string(pwm);
    send_msg(msg);
}

// 延时执行秒
void Mirobot::set_delay_time(double t)
{
    string msg = "G4 P" + doubleToString(t);
    send_msg(msg);
}

// 滑轨移动到
void Mirobot::slider_move_to(double x, int speed)
{
    string msg = "G90 G01";
    msg += " D" + doubleToString(x) + " F" + to_string(speed);
    send_msg(msg);
}

// 传送带移动到
void Mirobot::conveyor_move_to(int c, double n, int speed)
{
    string msg = "";
    if (movemode.relative == c)
        msg += "G91 G01";
    else if (movemode.absolute == c)
        msg += "G90 G01";
    else
        return;
    msg += " D" + doubleToString(n) + " F" + to_string(speed);
    send_msg(msg);
}

// 关节移动
void Mirobot::move_to_axis(int joint, int revolve, double n)
{
    string msg = "M21 G91 G01 ";
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
    string msg = "M20 G91 G01 ";
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
    string msg = "M20 ";
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
    string msg = "M20 ";
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
    send_msg("?");
    waitForEnd();
    string readstr = read_status();
    string showstr = "feedback : " + readstr;
    if (readstr[0] != '<')
    {
        get_status();
        return;
    }
    cout << showstr << endl;
    char tmp = readstr[1];
    status.moving = ((tmp == 'I') ? false : true);
    double state[13];
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
    int num = read_status().find(">");
    status.PumpPWM = int(readstr[num - 27]);
    status.ValvePWM = int(readstr[num - 15]);
    get_time();
}

Mirobot::~Mirobot()
{
    cout << endl;
    printf("program over\n");
}
