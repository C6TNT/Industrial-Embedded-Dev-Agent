#include "weld_sever/weld_jog.h"

static void axis_position_init(struct motor_attribute *attribute)
{
    attribute->target_position = attribute->actual_position;
    attribute->theory_position = attribute->actual_position;
}

static void move_loop(weld_opt_t *dev, struct motor_attribute *attribute)
{
    /*插补段*/
    switch (attribute->state)
    {
    case move_weld:
        break;
    case move_idle:
        break;
    case move_stop:
    case move_run:
        if (attribute->control_mode != 15)
        {
            attribute->state = move_idle;
            return;
        }
        if (attribute->mode_of_operation == 9 || attribute->mode_of_operation == 3)
        {
            attribute->theory_position = attribute->actual_position;
            if (attribute->move_mode == 2)
            {
                if (attribute->target_vel == 0)
                {
                    attribute->state = move_idle;
                    attribute->move_mode = 0;
                }
            }
            else
            {
                int32_t limitDec = SAFE_ABS(attribute->target_vel) / (1000.0 / (cycleTime / 1000000.0));
                if (SAFE_ABS(attribute->target_position - attribute->actual_position) < limitDec)
                {
                    attribute->target_vel = 0;
                    attribute->state = move_idle;
                }
                else if (attribute->target_vel == 0)
                {
                    attribute->state = move_idle;
                }
            }
        }
        else if (attribute->mode_of_operation == 8)
        {
            attribute->move_result = sinefittingPlan(attribute->step,
                                                     300.0,
                                                     attribute->init_vel,
                                                     (float)cycleTime / 1000.0 / 1000.0 / 1000.0,
                                                     attribute->acc_time,
                                                     attribute->dec_time,
                                                     attribute->f_target_vel,
                                                     attribute->target_step_position,
                                                     &attribute->now_position,
                                                     &attribute->now_vel);
            if (attribute->move_result == 0)
            {
                attribute->step += 1;
                attribute->target_position = attribute->start_position + attribute->now_position;
                attribute->theory_position = attribute->target_position;
            }
            else
            {
                attribute->step = 0;
                attribute->state = move_idle;
            }
        }
        break;
    }
}

static void single_motion_set(struct motor_attribute *attribute, double target_step_position, double target_vel)
{
    // 相对位置  方向在速度上
    /*
    轴方向1  速度+为dec变大的方向
    轴方向1  速度-为dec变小的方向
    轴方向-1 速度+为dec变小的方向
    轴方向-1 速度-为dec变大的方向
    */
    if (attribute->state == move_weld)
    {
        return;
    }
    if (attribute->mode_of_operation == 9 || attribute->mode_of_operation == 3)
    {
        if (SAFE_ABS(target_vel) < 1e-7)
        {
            attribute->target_vel = 0;
            attribute->state = move_idle;
        }
        else
        {
            int8_t cmd_direction = (target_vel > 0) ? 1 : -1;
            attribute->target_position = attribute->actual_position + target_step_position * attribute->f_transper * attribute->direciton * cmd_direction;
            attribute->target_vel = target_vel * attribute->f_transper * attribute->direciton;

            attribute->state = move_run;
        }
    }
    else if (attribute->mode_of_operation == 8)
    {
        attribute->step = 0;
        attribute->start_position = attribute->theory_position;
        attribute->init_vel = attribute->now_vel;
        attribute->target_step_position = target_step_position * attribute->f_transper;
        attribute->f_target_vel = target_vel * attribute->f_transper * attribute->direciton;
        attribute->state = move_run;
    }
}

static void weld_jog_home(weld_opt_t *dev)
{
    switch (dev->jog->jog_home->state)
    {
    case jog_home_idle:
        dev->jog->jog_home->init_tick = dev->system->sys_tick_get();
        dev->jog->jog_home->state = jog_home_work;
        break;
    case jog_home_work:
        if (dev->system->sys_tick_get() < dev->jog->jog_home->init_tick + 100)
        {
            dev->arc->attribute.mode_of_operation = 6;
            dev->yaw->attribute.mode_of_operation = 6;
            dev->angle->attribute.mode_of_operation = 6;
            dev->arc->attribute.control_mode = 6;
            dev->yaw->attribute.control_mode = 6;
            dev->angle->attribute.control_mode = 6;
        }
        else if (dev->system->sys_tick_get() < dev->jog->jog_home->init_tick + 200)
        {
            dev->arc->attribute.control_mode = 7;
            dev->yaw->attribute.control_mode = 7;
            dev->angle->attribute.control_mode = 7;
        }
        else if (dev->system->sys_tick_get() < dev->jog->jog_home->init_tick + 300)
        {
            dev->arc->attribute.control_mode = 15;
            dev->yaw->attribute.control_mode = 15;
            dev->angle->attribute.control_mode = 15;
        }
        else if (dev->system->sys_tick_get() < dev->jog->jog_home->init_tick + 500)
        {
            dev->arc->attribute.control_mode = 31;
            dev->yaw->attribute.control_mode = 31;
            dev->angle->attribute.control_mode = 31;
        }
        else if (dev->system->sys_tick_get() < dev->jog->jog_home->init_tick + 600)
        {
            dev->arc->attribute.control_mode = 15;
            dev->yaw->attribute.control_mode = 15;
            dev->angle->attribute.control_mode = 15;
        }
        else
        {
            dev->ethercat_control->ec_home_set(dev, 1);
            dev->jog->jog_home->state = jog_home_finish;
        }
        break;
    case jog_home_finish:
        break;
    }
}
static jog_home_t jog_home = {
    .state = jog_home_idle,
    .jog_home = weld_jog_home,
    .init_tick = 0,
};

weld_jog_t weld_jog = {
    .move_loop = move_loop,
    .single_motion_set = single_motion_set,
    .axis_init = axis_position_init,
    .jog_home = &jog_home,
};
