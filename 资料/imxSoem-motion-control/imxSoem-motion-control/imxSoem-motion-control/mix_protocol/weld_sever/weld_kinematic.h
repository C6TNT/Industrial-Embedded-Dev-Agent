#ifndef WELD_KINEMATIC_H
#define WELD_KINEMATIC_H
#include "weld_sever/weld_system.h"
#include "libtpr20pro/libcrobot.h"
typedef struct weld_opt weld_opt_t;
typedef struct weld_kinematic weld_kinematic_t;

typedef enum
{
    kinematic_type_none,
    kinematic_type_tube,
    kinematic_type_CNC,
} kinematic_type_t;

typedef enum
{
    kinematic_loop_idle,
    kinematic_loop_enable,
} kinematic_loop_state_t;
typedef struct
{
    double x;
} kinematic_world_axis_t;
typedef struct
{
    double dhparam[4];
    // 卡诺普
    // dhparam[0] = a1 320.0f
    // dhparam[1] = a2 1111.0f
    // dhparam[2] = a3 205.0f
    // dhparam[3] = d4 1232.0f
    double tool[3];
    // tool[0]=px;
    // tool[1]=py;
    // tool[2]=pz;
} kinematic_model_param;
typedef enum
{
    moveL_idle,
    moveL_run,
    moveL_stop,
} moveL_state_t;

typedef struct
{

    void (*loop)(weld_opt_t *dev);
    void (*moveL_set)(weld_opt_t *dev, RobotQuationPos endPosition, double targetVel);
    moveL_state_t state;
    int32_t step;
    int8_t moveL_result; // moveL返回值
    double now_position;
    double now_vel;
    double init_vel;
    double acc_time;
    double dec_time;
    float persent;
    RobotAngle lastrobotangle; // 关节角理论值
    /*命令下发端*/
    RobotQuationPos startPosition;
    RobotQuationPos endPosition;
    int targetDistance;
    double f_target_vel;
} kinematic_moveL_t;

typedef struct
{
    void (*moveJ_set)(weld_opt_t *dev, RobotAngle angle_endPos, double targetVel);
    int32_t step;
    int now_position;
    int now_vel;
    int init_vel;
    double acc_time;
    double dec_time;
    float persent;
    RobotAngle lastrobotangle; // 关节角理论值
    /*命令下发端*/
    RobotAngle angle_position;
    int targetDistance;
    double f_movj_target_vel;
} kinematic_moveJ_t;

struct weld_kinematic
{
    RobotAngle robotAngle;
    RobotPosition robotPositive;
    RobotQuationPos robotQuationPos;
    kinematic_world_axis_t world_axis;
    kinematic_model_param model_param;
    kinematic_loop_state_t loop_state;
    void (*init)(weld_opt_t *dev);
    void (*loop)(weld_opt_t *dev);
    kinematic_moveL_t *moveL;
    kinematic_moveJ_t *moveJ;
};
extern weld_kinematic_t weld_kinematic;
#endif // WELD_GAS_H
