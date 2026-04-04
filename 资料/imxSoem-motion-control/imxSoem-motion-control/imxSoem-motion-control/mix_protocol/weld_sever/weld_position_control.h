#ifndef _WELD_POSITION_CONTROL_H_
#define _WELD_POSITION_CONTROL_H_
#include "weld_sever/weld_system.h"
typedef struct weld_joint_axis weld_joint_axis_t;
typedef struct weld_position_control weld_position_control_t;

#define AXIS_MAX_COUNT 6 // 虚拟轴总数量

#define AXIS_0 0 // 虚拟端的轴号
#define AXIS_1 1 // 虚拟端的轴号
#define AXIS_2 2 // 虚拟端的轴号
#define AXIS_3 3 // 虚拟端的轴号
#define AXIS_4 4 // 虚拟端的轴号
#define AXIS_5 5 // 虚拟端的轴号
#define MAX_SEGMENT_LEN 10
typedef enum
{
    motion_type_Idle,
    motion_type_Single_Move,
    motion_type_Single_MoveAbs,
    motion_type_Single_VMove,
    motion_type_Move,
    motion_type_MoveAbs,
} motion_type_t;

typedef enum
{
    cancel_current_motion,
} motion_cancel_t;

typedef enum
{
    segment_plan_rise_down,
    segment_plan_rise,
    segment_plan_curise,
    segment_plan_down,
    segment_plan_down_estop,
} segment_plan_t;

typedef enum
{
    jog_segment_idle,
    jog_segment_rise_down,
    jog_segment_rise,
    jog_segment_curise,
    jog_segment_down,
} jog_segment_status_t;

typedef enum
{
    position_mode_idle,
    position_mode_work,
} position_mode_state_t;

struct plan_block_t
{

    /*如下变量为用户下发*/

    int32_t target_position;
    int32_t target_vel;
    uint32_t acc;
    uint32_t dec;
    int32_t origin_position;

    /*如下变量为用于计算*/
    float entry_vel_mm;
    float exit_vel_mm;
    double target_position_mm;
    double origin_position_mm;
    float target_vel_mm;
    double accelerate_until;
    double decelerate_after;
    float acc_mm;
    float dec_mm;
    double remain_millimeters;
    float uint_vector;
};

/*每个轴都有这样一个规划器
  该规划器输入后会在计算线程持续计算，计算后的值放入环形缓冲区等待主线程获取*/
struct segment_t
{
    int32_t target_position;
};

struct jog_block_t
{
    jog_segment_status_t status; // 点动规划器状态
    int32_t current_direction;   // 当前点动方向
    float target_vel;            // 点动目标速度
    float acc;
    float dec;
    int32_t target_direction; // 目标点动方向
};

struct pos_block_t
{
    motion_type_t type;
    int32_t transper;             // 传动比 仅有正值  方向在外部处理
    segment_plan_t status;        // 梯形规划状态
    position_mode_state_t state;  // 指令块执行状态
    float current_vel;            // 当前规划速度
    int32_t last_target_position; // 保存上次目标位置
    struct plan_block_t plan[1];
    uint8_t plan_head;
    uint8_t plan_tail;
    struct segment_t segment[1];
    uint16_t segment_head;
    uint16_t segment_next_head;
    uint16_t segment_tail;
    struct jog_block_t jog;
    /*jog*/
};

typedef enum
{
    axis_idle,
    axis_run,
    axis_stop,
} axis_control_state_t;

typedef struct
{
    int8_t direciton;
    uint16_t control_mode;
    uint32_t acc;
    uint32_t dec;
    int8_t mode_of_operation;
    int32_t actual_position;
    // position_control_t position_block;
    int32_t target_vel;
    int32_t drive_target_vel;
    int8_t drive_mode_of_operation;
    int8_t drive_mode_display;
    uint16_t drive_control_word;
    uint16_t drive_status_word;

    int32_t target_position; // 绝对位置

    int32_t actual_vel;
    uint16_t error_code;
    uint16_t status_code;
    /*新增适配严浩算法部分参数*/
    axis_control_state_t state;
    int32_t step;
    double target_step_position; // 相对位置
    int32_t theory_position;     // 理论位置
    int32_t start_position;      // 起始位置

    int now_vel;          // 当前速度
    double init_vel;      // 初始速度
    int now_position;     // 当前位置
    int8_t single_result; // 单轴返回值
    double acc_time;
    double dec_time;
    int32_t init_position; // 轴初始值
    /*浮点型命令*/
    double f_target_step_position;
    double f_target_vel;
    double f_transper; // 传动比
    /*插补点缓冲区*/
    struct segment_t segment[MAX_SEGMENT_LEN];
    uint8_t segment_head;
    uint8_t segment_next_head;
    uint8_t segment_tail;
    /*运动指令缓冲区*/

} joint_axis_t;

struct weld_joint_axis
{
    joint_axis_t attribute;
};

struct weld_position_control
{

    void (*segment_write)(joint_axis_t *attribute, int32_t now_p);
    int32_t (*segment_read)(joint_axis_t *attribute);
    void (*single_motion_set)(joint_axis_t *attribute, double target_step_position, double target_vel);
    void (*single_motion)(joint_axis_t *attribute);
    void (*axis_init)(joint_axis_t *attribute);
};

extern weld_position_control_t position_control;
extern weld_joint_axis_t weld_joint[AXIS_MAX_COUNT];
#endif
