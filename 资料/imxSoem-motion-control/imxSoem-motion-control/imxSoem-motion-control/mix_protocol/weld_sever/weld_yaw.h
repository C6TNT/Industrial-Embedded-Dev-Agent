#ifndef WELD_YAW_H
#define WELD_YAW_H

#include "weld_sever/weld_system.h"
#include "weld_sever/weld_motor.h"
typedef struct weld_opt weld_opt_t;
typedef struct weld_yaw weld_yaw_t;

typedef enum
{
    yaw_curise_idle,
    yaw_curise_work,
} yaw_curise_state_t;

typedef enum
{
    yaw_down_idle,
    yaw_down_work,
    yaw_down_finish,
} yaw_down_state_t;

typedef enum
{
    yaw_change_idle,
    yaw_change_forward,
    yaw_change_backward,
    yaw_change_forward_wait,
    yaw_change_backward_wait,
} yaw_change_state_t;

typedef enum
{
    yaw_ahead_idle,
    yaw_ahead_work,
} yaw_ahead_state_t;

typedef struct
{
    yaw_curise_state_t state;
    uint8_t (*weld_yaw_moving)(weld_opt_t *dev);
} yaw_curise_t;

typedef struct
{
    yaw_down_state_t state;
    uint8_t (*weld_yaw_down)(weld_opt_t *dev);
    int32_t rot_init_position;
} yaw_down_t;

typedef struct
{
    yaw_change_state_t state;
    void (*weld_yaw_change)(weld_opt_t *dev);
    int32_t edge_position;
    int32_t center_position;
    uint32_t stop_tick;
} yaw_change_t;

typedef struct
{
    void (*weld_yaw_ahead)(weld_opt_t *dev);
    yaw_ahead_state_t state;
    uint32_t stop_tick;
} yaw_ahead_t;

struct weld_yaw
{
    struct motor_attribute attribute;
    yaw_curise_t *curise;
    yaw_change_t *change;
    yaw_down_t *down;
    yaw_ahead_t *ahead;
    bool enable;
    bool edge_lock_enable;
    void (*reset)(weld_opt_t *dev);
    void (*estop)(weld_opt_t *dev);
};

extern weld_yaw_t weld_yaw;
#endif // WELD_WIRE_H
