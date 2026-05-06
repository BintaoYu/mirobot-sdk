#ifndef MIROBOT_H
#define MIROBOT_H

#include "device_session.h"
#include <chrono>
#include <memory>
#include <string>

std::string doubleToString(double dbNum);
void getMatch(const std::string& instr, double data[]);

struct MirobotJoint {
    int Joint1 = 1;
    int Joint2 = 2;
    int Joint3 = 3;
    int Joint4 = 4;
    int Joint5 = 5;
    int Joint6 = 6;
};

struct RevolveDirection {
    int cw = 1;
    int ccw = 2;
};

struct MoveDirection {
    int dforward = 1;
    int dbackward = 2;
    int dup = 3;
    int ddown = 4;
    int dright = 5;
    int dleft = 6;
};

struct MoveMode {
    int relative = 1; //相对位置G91
    int absolute = 2; //绝对位置G90
};

struct Motion {
    int MOVJ = 1; //关节运动
    int MOVL = 2; //直线运动
    int JUMP = 3; //门型轨迹运动
};

struct Status {
    bool moving = false; //机械臂空闲状态
    //机械臂各轴角度值
    double A = 0.0;
    double B = 0.0;
    double C = 0.0;
    double D = 0.0;
    double X = 0.0;
    double Y = 0.0;
    double Z = 0.0;
    //机械臂末端的位置和姿态
    double CCX = 0.0;
    double CCY = 0.0;
    double CCZ = 0.0;
    double CCRX = 0.0;
    double CCRY = 0.0;
    double CCRZ = 0.0;
    const char* time = nullptr;
    //两组PWM的值
    int PumpPWM = 0;
    int ValvePWM = 0;
};

class Mirobot {
private:
    bool debug;
    std::shared_ptr<CommInterface> owned_comm_;
    DeviceSession session_;
    MoveMode movemode;
    RevolveDirection revolvedirection;
    MirobotJoint mirobotjoint;
    Status status;
    char time_temp[32];

public:
    Motion motiontype;
    MoveDirection movedirection;

    explicit Mirobot(DeviceSession session, bool debugFlag = false);
    Mirobot(
        CommInterface& comm,
        int devicePort,
        bool debugFlag = false,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(1000));
    Mirobot(
        const std::string& targetIp,
        int targetPort,
        int devicePort,
        bool debugFlag = false,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(1000));

    std::string readline();
    std::string read_status();
    void get_time();
    void send_msg(const std::string& msg, bool wait = false, int delay = 0);
    std::string camera_msg(const std::string& msg, bool wait = true, int delay = 0);
    bool waitForEnd(std::chrono::milliseconds timeout = std::chrono::milliseconds(1000));

    //每个轴复位
    //home each axis individually
    void home_individual();

    //复位
    //home all axes simultaneously
    void home_simultaneous();

    //解锁
    //unlock the shaft
    void unlock_shaft();

    //设定运动速度
    //set speed
    void set_speed(int speed);

    //关节旋转到
    //send all axes to a specific position
    void go_to_axis(double a1, double a2, double a3, double a4, double a5, double a6);

    //回归零位
    //send all axes to their respective zero positions
    void go_to_zero(int delay = 0);

    //G1参数速度
    //G0最大速度
    //increment all axes a specified amount
    void increment_axis(double a1, double a2, double a3, double a4, double a5, double a6);

    //移动到坐标位置
    //linear move to a cartesian position
    void go_to_cartesian_lin(int m, double x, double y, double z, double a, double b, double c, int delay = 0);

    //笛卡尔空间中的线性增量
    //linear increment in cartesian space
    void increment_cartesian_lin(int m, double x, double y, double z, double a, double b, double c, int delay = 0);

    //吸盘开
    void suction_cup_on(int delay = 1);

    //吸盘关
    void suction_cup_off(int delay = 1);

    //吸盘吹
    void suction_cup_blow(int delay = 1);

    //夹具开
    void gripper_on();

    //夹具关
    void gripper_off();

    //set the pwm of the air pump
    void set_air_pump(int pwm);

    // set the pwm of the gripper
    void set_gripper(int pwm);

    //延时执行 秒
    //set the pwm of the gripper
    void set_delay_time(double t);

    //滑轨移动到
    void slider_move_to(double x, int speed);

    //传送带移动到
    void conveyor_move_to(int c, double n, int speed);

    //关节移动
    void move_to_axis(int joint, int revolve, double n);

    //定向移动
    void direction_mobility(int d, double n, int delay = 0);

    //门型轨迹移动 只用于笛卡尔模式
    void jump_move(int m, double x, double y, double z, double a, double b, double c);

    void set_arc_move(int mode, int revolve, double x, double y, double z, double r);

    void get_status();

    ~Mirobot();
};

int translation(const std::string& instr, double data[]);
void mirobot_reset(Mirobot &m);

#endif // MIROBOT_H
