#ifndef LIBCROBOT_H
#define LIBCROBOT_H

#include "math.h"
#include "libmanager.h"
#include "coordinatetrans.h"
#include <assert.h>
#define CROBOT_AXIS 6
//卡诺普
#define a1 320.0f
#define a2 1111.0f
#define a3 205.0f
#define d4 1232.0f

//新时达
//#define a1 190.0f
//#define a2 765.0f
//#define a3 200.0f
//#define d4 1036.5f

//typedef struct Motor_Parameter_{
//  int direction;
//  int initDec;
//  double transmission;
//  int dectime;
//  int acctime;
//  double maxvelocity;
//}motorparam_;

//未考虑第4与第6关节的耦合
const double PI = 3.141592653589793238;
//const double EPS =1e-5;
const double  MaxStepLength = PI * 1 / 180;   //关节角的最大限幅步长

typedef struct _RobotAngle
{
    double Angle1;
    double Angle2;
    double Angle3;
    double Angle4;
    double Angle5;
    double Angle6;
}RobotAngle;
typedef struct _RobotPosition
{
    //n方向的三个角度
    double nx;
    double ny;
    double nz;

    //a方向的三个角度
    double ax;
    double ay;
    double az;

    //o方向的三个角度
    double ox;
    double oy;
    double oz;

    //位置偏移
    double px;
    double py;
    double pz;
}RobotPosition;
typedef struct _RobotQuationPos
{
    double x;
    double y;
    double z;
    double ox;
    double oy;
    double oz;
    double ow;
}RobotQuationPos;

typedef struct {
    // 欧拉角结构（单位：度）
    double x;
    double y;
    double z;

    double roll;   // 滚转角 R
    double pitch;  // 俯仰角 P
    double yaw;    // 偏航角 Y
} EulerAngles;

typedef struct
{
    double a;           // 长半轴（必须 a > b > 0）
    double b;           // 短半轴（必须 a > b > 0）
    double theta;       // 旋转角度（相对于起点的转角，单位：rad）
    double theta_start; // 起始角度（初始位置，单位：rad）

} EllipseInput;

typedef struct
{
    double x; // 椭圆上点的x坐标
    double y; // 椭圆上点的y坐标
    double R; // 极径（原点到该点的距离）
    int flag; // 0=error ,1=ok
} EllipseOutput;

// 椭圆计算器结构体（封装参数和查找表）
typedef struct
{
    double a;          // 长半轴（确保a >= b）
    double b;          // 短半轴
    double e2;         // 离心率平方（1 - b²/a²，避免重复开方）
    double perimeter;  // 椭圆周长
    size_t sample_cnt; // 查找表采样点数
    double *theta_lut; // 角度查找表（弧度）
    double *s_lut;     // 弧长查找表（mm）
} EllipseCalculator;

extern	"C"
{
    RobotAngle crobotdecToJoint(int currentaxisdec[6],motorparam_ mp[6]);
    void crobotJointToDec(RobotAngle &joint,motorparam_ mp[6],int *calaxisdec);
    void calculateInitdec(int currentdec[6],motorparam_ mp[6],double *initdec);

    RobotPosition QuaternionToRotationMatrix(RobotQuationPos& quapostion);
    RobotQuationPos RotationToQuaternionMatrix(RobotPosition& robotposition);
    EulerAngles matrixToEulerAngles(RobotPosition tM_pos);

    RobotPosition PositiveRobot(RobotAngle& robotangle);
    RobotAngle InverseRobot(const RobotPosition& robotposition, RobotAngle& lastrobotangle);
    int targetdistanceCal(RobotQuationPos startpos, RobotQuationPos targetpos);
    RobotQuationPos nextQuationPostion(RobotQuationPos startpos, RobotQuationPos targetpos, float bili);
    void slerpclaculate(float starting[4], float ending[4], float result[4], float t);
    RobotQuationPos stopPlan(RobotQuationPos nowp, RobotQuationPos endp,double nowvel,double remainingdisance);

    RobotPosition endLinkToSixConverter(RobotPosition endlinkpos, double tool[3]);
    RobotPosition sixToendLinkConverter(RobotPosition six_pos, double tool[3]);

    RobotPosition calculate_vertical_sixbot(RobotPosition point_A,RobotPosition point_B,RobotPosition point_C,double d[3]);

    void cad_to_work1_trans_matrix(RobotPosition p_a0, double* cad_b0, RobotPosition p_a1, double* cad_b1, double* solution);
    void cad_to_work1_coordinate(double* cad_pos, double* trans_matrix, double* deep, double* w1_pos);

    RobotPosition jog_x_released(RobotAngle initial_angle,double tool[3]);
    RobotPosition jog_x_forward(RobotAngle initial_angle,double tool[3]);
    RobotPosition jog_y_released(RobotAngle initial_angle,double tool[3]);
    RobotPosition jog_y_forward(RobotAngle initial_angle,double tool[3]);
    RobotPosition jog_z_released(RobotAngle initial_angle,double tool[3]);
    RobotPosition jog_z_forward(RobotAngle initial_angle,double tool[3]);
    RobotPosition robot_trans_to_workpiece1(RobotPosition& origin_pos, RobotPosition& robot_space_pos);
    RobotPosition workpiece1_trans_to_robot(RobotPosition workpiece_1_origin, RobotPosition workpiece_1_position);
    RobotPosition workpiece1_jog_x_released(RobotAngle initial_angle, double tool[3]);
    RobotPosition workpiece1_jog_y_released(RobotAngle initial_angle, double tool[3]);
    RobotPosition workpiece1_jog_z_released(RobotAngle initial_angle, double tool[3]);
    RobotPosition workpiece1_jog_x_forward(RobotAngle initial_angle, double tool[3]);
    RobotPosition workpiece1_jog_y_forward(RobotAngle initial_angle, double tool[3]);
    RobotPosition workpiece1_jog_z_forward(RobotAngle initial_angle, double tool[3]);

    RobotPosition PositiveRobot_dh(RobotAngle& robotangle,double dhparam[4]);
    RobotAngle InverseRobot_dh(const RobotPosition& robotposition, RobotAngle& lastrobotangle,double dhparam[4]);

    EllipseOutput calc_ellipse_point(EllipseInput input);
    double ellipse_perimeter(double a, double b);
    double fast_ellipse_arc_length(double a, double b, double theta_rad);
    double arc_length_to_angle(double a, double b, double perimeter, double target_arc);

    int EllipseCalculator_Init(EllipseCalculator *ec, double a, double b, size_t sample_cnt);
    void EllipseCalculator_Destroy(EllipseCalculator *ec);
    double EllipseCalculator_ArcLengthToAngle(EllipseCalculator *ec, double s);
}
#endif // LIBMANAGER_H
