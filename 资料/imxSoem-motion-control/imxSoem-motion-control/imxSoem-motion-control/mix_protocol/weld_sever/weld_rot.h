#ifndef WELD_ROT_H
#define WELD_ROT_H

#include "weld_sever/weld_system.h"
#include "weld_sever/weld_motor.h"
typedef struct weld_opt weld_opt_t;
typedef struct weld_rot weld_rot_t;

typedef enum
{
    rot_transition_idle,
    rot_transition_work_forward,
    rot_transition_work_backward,
    rot_transition_finish,
} rot_transition_state_t;

typedef enum
{
    rot_transition_second_start_idle,
    rot_transition_second_start_work_forward,
    rot_transition_second_start_work_backward,
    rot_transition_second_satrt_finish,
} rot_transition_second_start_state_t;

typedef enum
{
    rot_curise_tube_idle,
    rot_curise_tube_parameter_line_set,
    rot_curise_tube_parameter_work,
    rot_curise_tube_parameter_elipse_set,
} rot_curise_tube_state_t;

typedef enum
{
    rot_down_idle,
    rot_down_parameter_line_set,
    rot_down_parameter_work,
    rot_down_parameter_elipse_set,
    rot_down_finish,
} rot_down_state_t;

typedef enum
{
    rot_go_home_idle,
    rot_go_home_wait,
    rot_go_home_forward,
    rot_go_home_backward,
    rot_go_home_finish,
} rot_go_home_state_t;

typedef struct
{
    rot_transition_state_t state;
    uint8_t (*weld_rot_transition)(weld_opt_t *dev);
    int32_t end_position;
} rot_transition_t;

typedef struct
{
    rot_transition_second_start_state_t state;
    uint8_t (*weld_rot_second_start_transition)(weld_opt_t *dev);
    int32_t end_position;
} rot_transition_second_start_t;

typedef struct
{
    rot_curise_tube_state_t state;
    void (*weld_rot_moving)(weld_opt_t *dev);
    double remain_step_position;
    double start_position;
    double end_position; // 每个区间的终止位置
    uint16_t prep_AO3;
} rot_curise_tube_t;

typedef enum
{
    rot_check_safe,
    rot_check_forward_overflow,
    rot_check_backward_overflow,
} rot_encoder_check_state_t;

typedef struct
{
    rot_encoder_check_state_t state;
    void (*weld_rot_encoder_check)(weld_opt_t *dev);
    int32_t overflow_count;
} rot_encoder_check_t;

typedef struct
{
    rot_down_state_t state;
    uint8_t (*weld_rot_down)(weld_opt_t *dev);
} rot_down_t;

typedef struct
{
    void (*weld_rot_go_home)(weld_opt_t *dev);
    rot_go_home_state_t state;
    int32_t init_position;
    bool enable;
} rot_go_home_t;

struct weld_rot
{
    struct motor_attribute attribute;
    rot_curise_tube_t *curise_tube;
    rot_down_t *down;
    rot_transition_t *transition;
    rot_transition_second_start_t *second_start_transition;
    rot_go_home_t *gohome;
    rot_encoder_check_t *check;
    bool speed_change_enable;
    bool final_interval_flag;
    void (*reset)(weld_opt_t *dev);
    void (*estop)(weld_opt_t *dev);
    void (*second_start_reset)(weld_opt_t *dev);
};

extern weld_rot_t weld_rot;
#endif // WELD_ROT_H
