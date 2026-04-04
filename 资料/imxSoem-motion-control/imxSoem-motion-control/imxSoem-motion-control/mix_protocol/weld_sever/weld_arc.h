#ifndef WELD_ARC_H
#define WELD_ARC_H

#include "weld_sever/weld_system.h"
#include "weld_sever/weld_motor.h"
typedef struct weld_opt weld_opt_t;
typedef struct weld_arc weld_arc_t;
typedef struct position_control position_control_t;

#define PID_KP 2409
#define PID_KI 0
#define PID_KD 1

typedef struct
{
    double integral;
    int32_t prev_error;
    uint32_t last_toggle;
} vol_track_pid_t;

typedef enum
{
    ihs_idle,
    ihs_forward,
    ihs_first_end,
    ihs_backward,
    ihs_second_end,
} arc_ihs_state_t;

typedef enum
{
    track_idle,
    track_work,
} arc_track_state_t;

typedef enum
{
    lift_idle,
    lift_work,
    lift_finish,
} arc_lift_state_t;

typedef struct
{
    uint8_t (*weld_ihs_moving)(weld_opt_t *dev);
    arc_ihs_state_t state;
    int32_t ihs_backward_end_position;
    bool enable;
} arc_ihs_t;

typedef struct
{
    bool enable;
    void (*weld_track)(weld_opt_t *dev);
    arc_track_state_t state;

} arc_track_t;

typedef struct
{
    bool enable;
    uint8_t (*weld_lift)(weld_opt_t *dev);
    arc_lift_state_t state;
    int32_t lift_backward_end_position;
} arc_lift_t;
typedef enum
{
    arc_axis_init_idle,
    arc_axis_init_set_sdo,
} arc_axis_init_state_t;

typedef struct
{
    void (*axis_init)(weld_opt_t *dev);
} arc_axis_init_t;

struct weld_arc
{
    struct motor_attribute attribute;
    arc_ihs_t *ihs;
    arc_track_t *track;
    arc_lift_t *lift;
    arc_axis_init_t *init;
    void (*reset)(weld_opt_t *dev);
    void (*estop)(weld_opt_t *dev);
    void (*check)(weld_opt_t *dev);
};
extern weld_arc_t weld_arc;
#endif // WELD_GAS_H
