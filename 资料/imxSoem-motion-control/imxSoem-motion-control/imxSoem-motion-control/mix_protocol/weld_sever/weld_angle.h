#ifndef WELD_ANGLE_H
#define WELD_ANGLE_H

#include "weld_sever/weld_system.h"
#include "weld_sever/weld_motor.h"
typedef struct weld_opt weld_opt_t;
typedef struct weld_angle weld_angle_t;
typedef struct position_control position_control_t;

typedef enum
{
    angle_curise_idle,
    angle_curise_work,
} angle_curise_state_t;

typedef struct
{
    void (*weld_angle_moving)(weld_opt_t *dev);
    angle_curise_state_t state;
    int32_t angle_init_position;
} angle_curise_t;

struct weld_angle
{
    angle_curise_t *curise;
    struct motor_attribute attribute;
    void (*reset)(weld_opt_t *dev);
    void (*estop)(weld_opt_t *dev);
};
extern weld_angle_t weld_angle;
#endif // WELD_WIRE_H
