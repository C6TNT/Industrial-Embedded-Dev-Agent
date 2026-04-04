#ifndef WELD_MOTOR_H
#define WELD_MOTOR_H

typedef enum
{
    move_idle,
    move_run,
    move_stop,
    move_weld,
} jog_state_t;

struct motor_attribute
{
    int32_t dec_encoder_overflow; // 无限回转边界条件
    int8_t direciton;
    uint16_t control_mode;
    uint32_t acc;
    uint32_t dec;
    int8_t mode_of_operation;
    uint8_t move_mode; // 为1时该轴无法被操作
    int64_t actual_position_64;
    int32_t actual_position;
    int32_t target_position; // 实际目标dec
    // position_control_t position_block;
    int32_t target_vel;
    int32_t drive_target_vel;
    int8_t drive_mode_of_operation;
    int8_t drive_mode_display;
    uint16_t drive_control_word;
    uint16_t drive_status_word;
    int32_t actual_vel;
    uint16_t error_code;
    uint16_t status_code;
    double acc_time;
    double dec_time;
    /*浮点型命令*/
    double f_target_step_position;
    double f_target_vel;
    double f_transper; // 传动比
    /*算法需要*/
    jog_state_t state;
    int now_vel;
    int now_position;
    int32_t init_vel;
    int32_t step;
    int8_t move_result;
    double target_step_position; // 相对位置
    int32_t init_position;       // 轴初始值
    int32_t start_position;      // 起始位置

    int32_t theory_position; // 理论位置
};

#endif // WELD_GAS_H
