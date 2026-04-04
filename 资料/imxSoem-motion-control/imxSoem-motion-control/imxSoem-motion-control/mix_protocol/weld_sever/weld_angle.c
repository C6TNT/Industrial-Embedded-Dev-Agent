#include "weld_sever/weld_angle.h"

/***************暂定所有方法为速度控制模式***************/

static void weld_angle_curise(weld_opt_t *dev)
{
    switch (dev->angle->curise->state)
    {
    case angle_curise_idle:
        if (dev->parameter->table->read_system_state == STATE_RUN_WELD)
        {
            dev->angle->curise->state = angle_curise_work;
            dev->angle->curise->angle_init_position = dev->parameter->table->angle_zero_position;
        }
        else
        {
            dev->angle->curise->state = angle_curise_idle;
        }
        break;
    case angle_curise_work:
        if (SAFE_ABS(dev->angle->attribute.actual_position -
                     dev->parameter->table->interval_angle_position[dev->parameter->table->section_rt]) < POSITION_ACCURACY)
        {
            dev->angle->attribute.target_vel = 0;
            dev->angle->curise->angle_init_position = dev->angle->attribute.actual_position;
        }
        else if (dev->parameter->table->read_system_state == STATE_RUN_WELD)
        {
            if (dev->parameter->table->interval_rot_distance[dev->parameter->table->section_rt] == 0 ||
                dev->parameter->table->interval_peak_rot_vel[dev->parameter->table->section_rt] == 0)
            {
                return;
            }
            switch (dev->parameter->table->AO_767[2])
            {
            case 0:
            case 2:
                dev->angle->attribute.target_vel = (dev->parameter->table->interval_angle_position[dev->parameter->table->section_rt] - dev->angle->curise->angle_init_position) /
                                                   ((double)dev->parameter->table->interval_rot_distance[dev->parameter->table->section_rt] /
                                                    (double)dev->parameter->table->interval_peak_rot_vel[dev->parameter->table->section_rt]);
                break;
            case 1:
                dev->angle->attribute.target_vel = (dev->parameter->table->interval_angle_position[dev->parameter->table->section_rt] - dev->angle->curise->angle_init_position) /
                                                   ((double)dev->parameter->table->interval_rot_distance[dev->parameter->table->section_rt] /
                                                    (double)dev->parameter->table->interval_base_rot_vel[dev->parameter->table->section_rt]);
                break;
            }
        }
        else
        {
            dev->angle->attribute.target_vel = 0;
        }
        break;
    }
}

static void angle_reset(weld_opt_t *dev)
{
    dev->angle->attribute.target_vel = 0;
    dev->angle->curise->state = angle_curise_idle;
}

static void angle_estop(weld_opt_t *dev)
{
    dev->angle->attribute.target_vel = 0;
}

static angle_curise_t angle_curise = {
    .weld_angle_moving = weld_angle_curise,
    .state = angle_curise_idle,
    .angle_init_position = 0,
};

weld_angle_t weld_angle = {
    .attribute = {0},
    .curise = &angle_curise,
    .reset = angle_reset,
    .estop = angle_estop,
};
