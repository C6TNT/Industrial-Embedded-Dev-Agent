#ifndef WELD_JOG_H
#define WELD_JOG_H

#include "weld_sever/weld_system.h"
#include "weld_sever/weld_motor.h"
typedef struct weld_opt weld_opt_t;
typedef struct weld_jog weld_jog_t;

typedef struct jot_kinematic jot_kinematic_t;

typedef enum
{
    jog_home_idle,
    jog_home_work,
    jog_home_finish,
} jog_home_state_t;

typedef struct
{
    void (*jog_home)(weld_opt_t *dev);
    uint32_t init_tick;
    jog_home_state_t state;

} jog_home_t;

struct weld_jog
{
    void (*axis_init)(struct motor_attribute *attribute);
    void (*move_loop)(weld_opt_t *dev,
                      struct motor_attribute *attribute);
    void (*single_motion_set)(struct motor_attribute *attribute,
                              double target_step_position,
                              double target_vel);
    jog_home_t *jog_home;
};

extern weld_jog_t weld_jog;
#endif // WELD_GAS_H
