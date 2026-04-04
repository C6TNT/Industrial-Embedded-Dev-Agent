#include "weld_sever/weld_rot.h"

/***************暂定所有方法为速度控制模式***************/

static void weld_rot_curise_tube(weld_opt_t *dev)
{
    /*
        状态机:
        0:准备状态  判断当前插补类型(椭圆or长直线)
        1:长直线插补类参数重载状态(计算剩余距离  重载当前速度 当前区间递增) 设置完切换至2
        2:扫描峰基值边沿 扫描区间切换边沿 若触发则切换至1   扫描区间完成标志  若完成则切换至4
        3:椭圆插补类参数重载状态 设置完切换至2
        4:完成
    */
    double now_vel = 0;
    double remain_distance = 0;
    switch (dev->parameter->table->AO_767[2])
    {
    case 0:
    case 2:
        now_vel = dev->parameter->table->interval_peak_rot_vel[dev->parameter->table->section_rt];
        break;
    case 1:
        now_vel = dev->parameter->table->interval_base_rot_vel[dev->parameter->table->section_rt];
        break;
    }
    switch (dev->rot->curise_tube->state)
    {
    case rot_curise_tube_idle:
        switch (dev->parameter->table->rot_interpolation_mode)
        {
        case 0:
            dev->rot->curise_tube->remain_step_position = 0;
            for (int i = 0; i < dev->parameter->table->section_overall; i++)
            {
                dev->rot->curise_tube->remain_step_position += dev->parameter->table->interval_rot_distance[i];
            }

            dev->rot->curise_tube->remain_step_position += dev->parameter->table->rot_down_distance;
            dev->rot->curise_tube->prep_AO3 = dev->parameter->table->AO_767[2];
            dev->rot->curise_tube->start_position = dev->rot->attribute.actual_position_64;
            if ((dev->rot->attribute.direciton == 1 && now_vel >= 0) || (dev->rot->attribute.direciton == -1 && now_vel < 0))
            {
                dev->rot->curise_tube->end_position = dev->rot->attribute.actual_position_64 +
                                                      dev->parameter->table->interval_rot_distance[dev->parameter->table->section_rt] * dev->rot->attribute.f_transper * dev->rot->attribute.direciton;
            }
            else
            {
                dev->rot->curise_tube->end_position = dev->rot->attribute.actual_position_64 -
                                                      dev->parameter->table->interval_rot_distance[dev->parameter->table->section_rt] * dev->rot->attribute.f_transper * dev->rot->attribute.direciton;
            }

            dev->rot->curise_tube->state = rot_curise_tube_parameter_line_set;
            break;
        }
        break;
    case rot_curise_tube_parameter_line_set:

        remain_distance = dev->rot->curise_tube->remain_step_position - SAFE_ABS(dev->rot->attribute.actual_position_64 - dev->rot->curise_tube->start_position) / dev->rot->attribute.f_transper;
        switch (dev->parameter->table->AO_767[2])
        {
        case 0:
        case 2:
            dev->jog->single_motion_set(&dev->rot->attribute,
                                        remain_distance,
                                        dev->parameter->table->interval_peak_rot_vel[dev->parameter->table->section_rt]);
            break;
        case 1:
            dev->jog->single_motion_set(&dev->rot->attribute,
                                        remain_distance,
                                        dev->parameter->table->interval_base_rot_vel[dev->parameter->table->section_rt]);
            break;
        }
        dev->rot->curise_tube->state = rot_curise_tube_parameter_work;
        break;
    case rot_curise_tube_parameter_work:
        /*峰基值检测*/
        switch (dev->parameter->table->AO_767[2])
        {
        case 0:
        case 2:
            if (dev->rot->curise_tube->prep_AO3 == 1)
            {
                dev->rot->curise_tube->state = rot_curise_tube_parameter_line_set;
            }
            break;
        case 1:
            if (dev->rot->curise_tube->prep_AO3 == 0 || dev->rot->curise_tube->prep_AO3 == 2)
            {
                dev->rot->curise_tube->state = rot_curise_tube_parameter_line_set;
            }
            break;
        }
        dev->rot->curise_tube->prep_AO3 = dev->parameter->table->AO_767[2];
        /*区间检测*/
        if ((dev->rot->attribute.direciton == 1 && now_vel >= 0) || (dev->rot->attribute.direciton == -1 && now_vel < 0))
        {
            if (dev->rot->curise_tube->end_position <= dev->rot->attribute.actual_position_64)
            {
                if ((dev->parameter->table->section_rt + 1) == dev->parameter->table->section_overall)
                {
                    /*区间结束 该任务终止*/
                    dev->parameter->table->read_system_state = STATE_DOWN_WELD;
                    return;
                }
                else
                {

                    dev->parameter->table->section_rt++;
                    dev->rot->curise_tube->end_position += dev->parameter->table->interval_rot_distance[dev->parameter->table->section_rt] * dev->rot->attribute.f_transper;
                    dev->rot->curise_tube->state = rot_curise_tube_parameter_line_set;
                }
            }
        }
        else
        {
            if (dev->rot->curise_tube->end_position >= dev->rot->attribute.actual_position_64)
            {
                if ((dev->parameter->table->section_rt + 1) == dev->parameter->table->section_overall)
                {
                    /*区间结束 该任务终止*/
                    dev->parameter->table->read_system_state = STATE_DOWN_WELD;
                    return;
                }
                else
                {

                    dev->parameter->table->section_rt++;
                    dev->rot->curise_tube->end_position -= dev->parameter->table->interval_rot_distance[dev->parameter->table->section_rt] * dev->rot->attribute.f_transper;
                    dev->rot->curise_tube->state = rot_curise_tube_parameter_line_set;
                }
            }
        }

        break;
    case rot_curise_tube_parameter_elipse_set:
        break;
    }
}

static uint8_t weld_rot_down_weld(weld_opt_t *dev)
{
    switch (dev->rot->down->state)
    {
    case rot_down_idle:
        switch (dev->parameter->table->rot_interpolation_mode)
        {
        case 0:
            dev->rot->down->state = rot_down_parameter_line_set;
            break;
        }
        break;
    case rot_down_parameter_line_set:
        dev->jog->single_motion_set(&dev->rot->attribute,
                                    dev->parameter->table->rot_down_distance,
                                    dev->parameter->table->rot_down_vel);
        dev->rot->down->state = rot_down_parameter_work;
        break;
    case rot_down_parameter_work:
        if (dev->rot->attribute.state == move_idle)
        {
            dev->rot->down->state = rot_down_finish;
        }
        break;
    case rot_down_parameter_elipse_set:
        break;
    case rot_down_finish:
        return 1;
        break;
    }
    return 0;
}

static uint8_t weld_rot_transition_weld(weld_opt_t *dev)
{
    switch (dev->rot->transition->state)
    {
    case rot_transition_idle:
        if (dev->parameter->table->interval_rot_direction[dev->parameter->table->section_rt] == 0)
        {
            dev->rot->transition->end_position = dev->rot->attribute.actual_position +
                                                 db_rot_transition_distance;
            dev->rot->attribute.target_vel = db_rot_transition_vel;
            dev->rot->transition->state = rot_transition_work_forward;
        }
        else
        {
            dev->rot->transition->end_position = dev->rot->attribute.actual_position -
                                                 db_rot_transition_distance;
            dev->rot->transition->state = rot_transition_work_backward;
            dev->rot->attribute.target_vel = -1 * db_rot_transition_vel;
        }
        break;
    case rot_transition_work_forward:
        if (dev->rot->attribute.actual_position >= dev->rot->transition->end_position)
        {
            dev->rot->transition->state = rot_transition_finish;
            dev->rot->attribute.target_vel = 0;
        }
        break;
    case rot_transition_work_backward:
        if (dev->rot->attribute.actual_position <= dev->rot->transition->end_position)
        {
            dev->rot->transition->state = rot_transition_finish;
            dev->rot->attribute.target_vel = 0;
        }
        break;
    case rot_transition_finish:
        if (dev->rot->final_interval_flag)
        {
            return 1;
        }
        else
        {
            dev->parameter->table->section_rt++;
            return 2; // 仍有区间需要完成 复位继续
        }
        break;
    }
    return 0;
}

static uint8_t weld_rot_second_start_transition_weld(weld_opt_t *dev)
{
    switch (dev->rot->second_start_transition->state)
    {
    case rot_transition_second_start_idle:
        if (dev->parameter->table->interval_rot_direction[dev->parameter->table->section_rt] == 0)
        {
            /*二次启动时反向行走避开 急停焊点*/
            dev->rot->second_start_transition->end_position = dev->rot->attribute.actual_position -
                                                              db_rot_transition_distance;
            dev->rot->attribute.target_vel = -1 * db_rot_transition_vel;
            dev->rot->second_start_transition->state = rot_transition_second_start_work_backward;
        }
        else
        {
            dev->rot->second_start_transition->end_position = dev->rot->attribute.actual_position +
                                                              db_rot_transition_distance;
            dev->rot->second_start_transition->state = rot_transition_second_start_work_forward;
            dev->rot->attribute.target_vel = db_rot_transition_vel;
        }
        break;
    case rot_transition_second_start_work_forward:
        if (dev->rot->attribute.actual_position >= dev->rot->second_start_transition->end_position)
        {
            dev->rot->second_start_transition->state = rot_transition_second_satrt_finish;
            dev->rot->attribute.target_vel = 0;
        }
        break;
    case rot_transition_second_start_work_backward:
        if (dev->rot->attribute.actual_position <= dev->rot->second_start_transition->end_position)
        {
            dev->rot->second_start_transition->state = rot_transition_second_satrt_finish;
            dev->rot->attribute.target_vel = 0;
        }
        break;
    case rot_transition_second_satrt_finish:
        return 1;
        break;
    }
    return 0;
}

static void weld_rot_go_home_weld(weld_opt_t *dev)
{
    if ((dev->rot->attribute.control_mode == 15) &&
        (dev->rot->attribute.mode_of_operation == 3) &&
        ((dev->rot->attribute.target_vel != 0) ||
         (dev->rot->attribute.f_target_vel != 0.0) ||
         (dev->rot->attribute.acc != 0U) ||
         (dev->rot->attribute.dec != 0U)))
    {
        return;
    }

    switch (dev->rot->gohome->state)
    {
    case rot_go_home_idle:
        dev->rot->gohome->init_position = dev->rot->attribute.actual_position;
        dev->rot->gohome->enable = dev->parameter->table->switch_sinal & bit(14);
        if ((dev->parameter->table->read_system_state & STATE_PRE_GAS_WELD) && dev->rot->gohome->enable)
        {
            dev->rot->gohome->state = rot_go_home_wait;
        }
        break;
    case rot_go_home_wait:
        if (dev->parameter->table->read_system_state == STATE_IDLE_WELD)
        {
            dev->parameter->table->read_system_state = STATE_GO_HOME;
            if (dev->rot->attribute.actual_position < dev->rot->gohome->init_position)
            {
                dev->rot->gohome->state = rot_go_home_forward;
                dev->rot->attribute.target_vel = db_rot_go_home_vel;
            }
            else
            {
                dev->rot->gohome->state = rot_go_home_backward;
                dev->rot->attribute.target_vel = -1 * db_rot_go_home_vel;
            }
        }
        break;
    case rot_go_home_forward:
        if (dev->rot->attribute.actual_position >= dev->rot->gohome->init_position)
        {
            dev->rot->gohome->state = rot_go_home_finish;
        }
        break;
    case rot_go_home_backward:
        if (dev->rot->attribute.actual_position <= dev->rot->gohome->init_position)
        {
            dev->rot->gohome->state = rot_go_home_finish;
        }
        break;
    case rot_go_home_finish:
        dev->parameter->table->read_system_state = STATE_IDLE_WELD;
        dev->rot->gohome->state = rot_go_home_idle;
        dev->rot->attribute.target_vel = 0;
        break;
    }
}

static void weld_rot_encoder_check(weld_opt_t *dev)
{
    switch (dev->rot->check->state)
    {
    case rot_check_safe:
        dev->rot->attribute.actual_position_64 = dev->rot->attribute.actual_position + 4294967295 * dev->rot->check->overflow_count;
        if (dev->rot->attribute.actual_position > 2100000000)
        {
            dev->rot->check->state = rot_check_forward_overflow;
        }
        else if (dev->rot->attribute.actual_position < -2100000000)
        {
            dev->rot->check->state = rot_check_backward_overflow;
        }

        break;
    case rot_check_forward_overflow:
        if (dev->rot->attribute.actual_position >= 0 && dev->rot->attribute.actual_position <= 2100000000)
        {
            dev->rot->check->state = rot_check_safe;
        }
        else if (dev->rot->attribute.actual_position < 0)
        {
            dev->rot->check->overflow_count++;
            dev->rot->attribute.actual_position_64 = dev->rot->attribute.actual_position + 4294967295 * dev->rot->check->overflow_count;
            dev->rot->check->state = rot_check_safe;
        }
        else
        {
            dev->rot->attribute.actual_position_64 = dev->rot->attribute.actual_position + 4294967295 * dev->rot->check->overflow_count;
        }
        break;
    case rot_check_backward_overflow:
        if (dev->rot->attribute.actual_position <= 0 && dev->rot->attribute.actual_position >= -2100000000)
        {
            dev->rot->check->state = rot_check_safe;
        }
        else if (dev->rot->attribute.actual_position > 0)
        {
            dev->rot->check->overflow_count--;
            dev->rot->attribute.actual_position_64 = dev->rot->attribute.actual_position + 4294967295 * dev->rot->check->overflow_count;
            dev->rot->check->state = rot_check_safe;
        }
        else
        {
            dev->rot->attribute.actual_position_64 = dev->rot->attribute.actual_position + 4294967295 * dev->rot->check->overflow_count;
        }
        break;
    }
}

static void rot_reset(weld_opt_t *dev)
{
    dev->rot->attribute.move_mode = 0;
    dev->rot->attribute.target_vel = 0;
    dev->rot->speed_change_enable = 0; // 换向标志
    dev->rot->final_interval_flag = 0; // 结束标志
    dev->rot->down->state = rot_down_idle;
    dev->rot->curise_tube->state = rot_curise_tube_idle;
    dev->rot->transition->state = rot_transition_idle;
    dev->rot->second_start_transition->state = rot_transition_second_start_idle;
}

static void rot_second_start_reset(weld_opt_t *dev)
{
    dev->rot->attribute.move_mode = 0;
    dev->rot->attribute.target_vel = 0;
    // dev->rot->speed_change_enable = 0; // 换向标志 二次启动不可复位
    // dev->rot->final_interval_flag = 0; // 结束标志 二次启动不可复位
    dev->rot->down->state = rot_down_idle;
    // dev->rot->curise_tube->state = rot_curise_tube_idle;  //注意二次启动不可复位该方法
    dev->rot->transition->state = rot_transition_idle;
    dev->rot->second_start_transition->state = rot_transition_second_start_idle;
}

static void rot_estop(weld_opt_t *dev)
{
    dev->rot->attribute.move_mode = 0;
    dev->rot->attribute.target_vel = 0;
}

static rot_transition_second_start_t rot_transition_second_start = {
    .weld_rot_second_start_transition = weld_rot_second_start_transition_weld,
    .state = rot_transition_second_start_idle,
    .end_position = 0,
};

static rot_transition_t rot_transition = {
    .weld_rot_transition = weld_rot_transition_weld,
    .state = rot_transition_idle,
    .end_position = 0,
};

static rot_go_home_t rot_go_home = {

    .weld_rot_go_home = weld_rot_go_home_weld,
    .init_position = 0,
    .state = rot_go_home_idle,
    .enable = 0,
};

static rot_curise_tube_t rot_curise_tube = {
    .weld_rot_moving = weld_rot_curise_tube,
    .state = rot_curise_tube_idle,
    .remain_step_position = 0,
    .end_position = 0,
    .start_position = 0,
    .prep_AO3 = 0,
};

static rot_down_t rot_down = {
    .weld_rot_down = weld_rot_down_weld,
    .state = rot_down_idle,
};

static rot_encoder_check_t rot_check = {
    .state = rot_check_safe,
    .weld_rot_encoder_check = weld_rot_encoder_check,
    .overflow_count = 0,
};

weld_rot_t weld_rot = {

    .gohome = &rot_go_home,
    .attribute = {0},
    .reset = rot_reset,
    .curise_tube = &rot_curise_tube,
    .down = &rot_down,
    .transition = &rot_transition,
    .speed_change_enable = 0,
    .final_interval_flag = 0,
    .estop = rot_estop,
    .second_start_reset = rot_second_start_reset,
    .check = &rot_check,
    .second_start_transition = &rot_transition_second_start,
};
