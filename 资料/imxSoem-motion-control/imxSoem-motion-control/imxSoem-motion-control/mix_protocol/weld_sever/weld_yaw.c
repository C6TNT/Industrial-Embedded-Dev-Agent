#include "weld_sever/weld_yaw.h"

static void yaw_change_edge_position(weld_opt_t *dev)
{
    if (dev->yaw->down->state == yaw_down_work && dev->parameter->table->read_system_state == STATE_DOWN_WELD)
    {
        if (dev->parameter->table->interval_rot_direction[dev->parameter->table->section_rt] == 0)
        {

            if (dev->yaw->change->state == yaw_change_forward_wait)
            {
                dev->yaw->change->edge_position =
                    dev->yaw->change->center_position -
                    YawlinearFunction(dev->yaw->down->rot_init_position,
                                      dev->parameter->table->interval_yaw_distance[dev->parameter->table->section_rt],
                                      dev->yaw->down->rot_init_position + (dev->parameter->table->down_time / 1000.0f) * dev->parameter->table->rot_down_vel,
                                      0,
                                      dev->rot->attribute.actual_position);
            }
            else if (dev->yaw->change->state == yaw_change_backward_wait)
            {
                dev->yaw->change->edge_position =
                    dev->yaw->change->center_position +
                    YawlinearFunction(dev->yaw->down->rot_init_position,
                                      dev->parameter->table->interval_yaw_distance[dev->parameter->table->section_rt],
                                      dev->yaw->down->rot_init_position + (dev->parameter->table->down_time / 1000.0f) * dev->parameter->table->rot_down_vel,
                                      0,
                                      dev->rot->attribute.actual_position);
            }
        }
        else
        {
            if (dev->yaw->change->state == yaw_change_forward_wait)
            {
                dev->yaw->change->edge_position =
                    dev->yaw->change->center_position -
                    YawlinearFunction_back(dev->yaw->down->rot_init_position,
                                           dev->parameter->table->interval_yaw_distance[dev->parameter->table->section_rt],
                                           dev->yaw->down->rot_init_position - (dev->parameter->table->down_time / 1000.0f) * dev->parameter->table->rot_down_vel,
                                           0,
                                           dev->rot->attribute.actual_position);
            }
            else if (dev->yaw->change->state == yaw_change_backward_wait)
            {
                dev->yaw->change->edge_position =
                    dev->yaw->change->center_position +
                    YawlinearFunction_back(dev->yaw->down->rot_init_position,
                                           dev->parameter->table->interval_yaw_distance[dev->parameter->table->section_rt],
                                           dev->yaw->down->rot_init_position - (dev->parameter->table->down_time / 1000.0f) * dev->parameter->table->rot_down_vel,
                                           0,
                                           dev->rot->attribute.actual_position);
            }
        }
    }
    else if (dev->yaw->ahead->state == yaw_ahead_work && (dev->parameter->table->read_system_state & (STATE_PRE_WELD |
                                                                                                      STATE_RISE_WELD)))
    {
        if (dev->yaw->change->state == yaw_change_forward_wait)
        {
            dev->yaw->change->edge_position = dev->yaw->change->center_position - dev->parameter->table->yaw_ahead_distance;
        }
        else if (dev->yaw->change->state == yaw_change_backward_wait)
        {
            dev->yaw->change->edge_position = dev->yaw->change->center_position + dev->parameter->table->yaw_ahead_distance;
        }
    }
    else
    {
        if (dev->yaw->change->state == yaw_change_forward_wait)
        {
            dev->yaw->change->edge_position = dev->yaw->change->center_position - dev->parameter->table->interval_yaw_distance[dev->parameter->table->section_rt];
        }
        else if (dev->yaw->change->state == yaw_change_backward_wait)
        {
            dev->yaw->change->edge_position = dev->yaw->change->center_position + dev->parameter->table->interval_yaw_distance[dev->parameter->table->section_rt];
        }
    }
}
/***************横摆的切换用于管控横摆主状态机***************/
static void weld_yaw_change_position(weld_opt_t *dev)
{
    dev->yaw->enable = dev->parameter->table->switch_sinal & bit(5);
    dev->yaw->edge_lock_enable = dev->parameter->table->switch_sinal & bit(4);
    dev->yaw->change->center_position = dev->parameter->table->yaw_center_positiopn;
    if (!dev->yaw->enable ||
        ((dev->parameter->table->read_system_state & (STATE_RISE_WELD |
                                                      STATE_PRE_WELD |
                                                      STATE_RUN_WELD |
                                                      STATE_DOWN_WELD)) == 0))
    {
        dev->yaw->change->state = yaw_change_idle;
        return;
    }
    else
    {
        yaw_change_edge_position(dev);
        switch (dev->yaw->change->state)
        {
        case yaw_change_idle:
            // dev->yaw->change->center_position = dev->yaw->attribute.actual_position;
            dev->yaw->change->edge_position = dev->yaw->change->center_position + dev->parameter->table->yaw_ahead_distance;
            dev->yaw->change->state = yaw_change_forward;
            break;
        case yaw_change_forward:
            if (dev->yaw->attribute.actual_position > (dev->yaw->change->edge_position - POSITION_ACCURACY))
            {
                /*******已到达边缘***********/
                dev->yaw->attribute.target_vel = 0;
                dev->yaw->change->state = yaw_change_forward_wait;
                dev->yaw->change->stop_tick = dev->system->sys_tick_get() + getCycle(dev->parameter->table->interval_yaw_forward_time[dev->parameter->table->section_rt]);
            }
            break;
        case yaw_change_forward_wait:
            if (dev->parameter->table->read_system_state & (STATE_DOWN_WELD))
            {
                dev->yaw->change->state = yaw_change_backward;
            }
            else
            {
                if (dev->system->sys_tick_get() >= dev->yaw->change->stop_tick)
                {
                    dev->yaw->change->state = yaw_change_backward;
                }
                else
                {
                    if (dev->yaw->edge_lock_enable)
                    {
                        dev->current->peak_base_change->state = peak_state;
                        dev->current->peak_base_change->stop_tick = dev->system->sys_tick_get() + 2;
                    }
                }
            }
            break;
        case yaw_change_backward:
            if (dev->yaw->attribute.actual_position < (dev->yaw->change->edge_position + POSITION_ACCURACY))
            {
                /*******已到达边缘***********/

                dev->yaw->attribute.target_vel = 0;
                dev->yaw->change->state = yaw_change_backward_wait;
                dev->yaw->change->stop_tick = dev->system->sys_tick_get() + getCycle(dev->parameter->table->interval_yaw_backward_time[dev->parameter->table->section_rt]);
            }
            break;
        case yaw_change_backward_wait:
            if (dev->parameter->table->read_system_state & (STATE_DOWN_WELD))
            {
                dev->yaw->change->state = yaw_change_forward;
            }
            else
            {
                if (dev->system->sys_tick_get() >= dev->yaw->change->stop_tick)
                {
                    dev->yaw->change->state = yaw_change_forward;
                }
                else
                {
                    if (dev->yaw->edge_lock_enable)
                    {
                        dev->current->peak_base_change->state = peak_state;
                        dev->current->peak_base_change->stop_tick = dev->system->sys_tick_get() + 2;
                    }
                }
            }
            break;
        }
    }
}

static uint8_t weld_yaw_curise(weld_opt_t *dev)
{
    switch (dev->yaw->curise->state)
    {
    case yaw_curise_idle:
        if (dev->yaw->enable &&
            (dev->parameter->table->read_system_state == STATE_RUN_WELD))
        {
            dev->yaw->curise->state = yaw_curise_work;
        }
        break;
    case yaw_curise_work:
        if (dev->yaw->enable)
        {
            switch (dev->yaw->change->state)
            {
            case yaw_change_forward:
                dev->yaw->attribute.target_vel = dev->parameter->table->interval_yaw_vel[dev->parameter->table->section_rt];
                break;
            case yaw_change_backward:
                dev->yaw->attribute.target_vel = -1 * dev->parameter->table->interval_yaw_vel[dev->parameter->table->section_rt];
                break;
            case yaw_change_forward_wait:
                dev->yaw->attribute.target_vel = 0;
                break;
            case yaw_change_backward_wait:
                dev->yaw->attribute.target_vel = 0;
                break;
            default:
                dev->yaw->attribute.target_vel = 0;
                break;
            }
        }
        else
        {
            dev->yaw->attribute.target_vel = 0;
            dev->yaw->curise->state = yaw_curise_idle;
        }

        break;
    }
    return 0;
}

static uint8_t weld_yaw_down_weld(weld_opt_t *dev)
{
    switch (dev->yaw->down->state)
    {
    case yaw_down_idle:
        if (dev->yaw->enable)
        {
            dev->yaw->down->state = yaw_down_work;
            dev->yaw->down->rot_init_position = dev->rot->attribute.actual_position;
        }
        else
        {
            dev->yaw->down->state = yaw_down_finish;
        }
        break;
    case yaw_down_work:
        if (dev->yaw->change->edge_position == dev->yaw->change->center_position)
        {
            if (SAFE_ABS(dev->yaw->attribute.actual_position - dev->yaw->change->edge_position) < POSITION_ACCURACY)
            {
                dev->yaw->down->state = yaw_down_finish;
                dev->yaw->attribute.target_vel = 0;
            }
        }
        switch (dev->yaw->change->state)
        {
        case yaw_change_forward:
            dev->yaw->attribute.target_vel = dev->parameter->table->interval_yaw_vel[dev->parameter->table->section_rt];
            break;
        case yaw_change_backward:
            dev->yaw->attribute.target_vel = -1 * dev->parameter->table->interval_yaw_vel[dev->parameter->table->section_rt];
            break;
        case yaw_change_forward_wait:
            dev->yaw->attribute.target_vel = 0;
            break;
        case yaw_change_backward_wait:
            dev->yaw->attribute.target_vel = 0;
            break;
        default:
            dev->yaw->attribute.target_vel = 0;
            break;
        }
        break;
    case yaw_down_finish:
        return 1;
        break;
    }
    return 0;
}

// static uint8_t weld_yaw_ahead_move(weld_opt_t *dev)
// {
//     switch (dev->yaw->ahead->state)
//     {
//     case yaw_ahead_idle:
//         if (dev->yaw->enable && dev->parameter->table->read_system_state == STATE_PRE_WELD)
//         {
//             if (dev->parameter->table->interval_rot_direction[dev->parameter->table->section_rt] == 0)
//             {
//                 dev->yaw->ahead->state = yaw_ahead_work_forward_wait;
//                 dev->yaw->ahead->start_rot_position = dev->rot->attribute.actual_position +
//                                                       dev->parameter->table->yaw_ahead_start_position + POSITION_ACCURACY;
//                 // PRINTF("x\r\n");
//             }
//             else
//             {
//                 dev->yaw->ahead->state = yaw_ahead_work_backward_wait;
//                 dev->yaw->ahead->start_rot_position = dev->rot->attribute.actual_position -
//                                                       dev->parameter->table->yaw_ahead_start_position - POSITION_ACCURACY;
//                 // PRINTF("m\r\n");
//             }
//         }
//         else
//         {
//             dev->yaw->ahead->state = yaw_ahead_finish;
//         }
//         break;
//     case yaw_ahead_work_forward_wait:
//         if (dev->rot->attribute.actual_position < dev->yaw->ahead->start_rot_position)
//         {
//             switch (dev->yaw->change->state)
//             {
//             case yaw_change_forward:
//                 dev->yaw->attribute.target_vel = dev->parameter->table->yaw_ahead_vel;
//                 break;
//             case yaw_change_backward:
//                 dev->yaw->attribute.target_vel = -1 * dev->parameter->table->yaw_ahead_vel;
//                 break;
//             case yaw_change_forward_wait:
//                 dev->yaw->attribute.target_vel = 0;
//                 break;
//             case yaw_change_backward_wait:
//                 dev->yaw->attribute.target_vel = 0;
//                 break;
//             default:
//                 dev->yaw->attribute.target_vel = 0;
//                 break;
//             }
//         }
//         else
//         {
//             dev->yaw->ahead->state = yaw_ahead_finish;
//         }

//         break;
//     case yaw_ahead_work_backward_wait:
//         if (dev->rot->attribute.actual_position > dev->yaw->ahead->start_rot_position)
//         {
//             switch (dev->yaw->change->state)
//             {
//             case yaw_change_forward:
//                 dev->yaw->attribute.target_vel = dev->parameter->table->yaw_ahead_vel;
//                 break;
//             case yaw_change_backward:
//                 dev->yaw->attribute.target_vel = -1 * dev->parameter->table->yaw_ahead_vel;
//                 break;
//             case yaw_change_forward_wait:
//                 dev->yaw->attribute.target_vel = 0;
//                 break;
//             case yaw_change_backward_wait:
//                 dev->yaw->attribute.target_vel = 0;
//                 break;
//             default:
//                 dev->yaw->attribute.target_vel = 0;
//                 break;
//             }
//         }
//         else
//         {
//             dev->yaw->ahead->state = yaw_ahead_finish;
//         }

//         break;
//     case yaw_ahead_finish:
//         return 1;
//         break;
//     }
//     return 0;
// }

static void weld_yaw_ahead_move_2(weld_opt_t *dev)
{
    switch (dev->yaw->ahead->state)
    {
    case yaw_ahead_idle:
        if (dev->yaw->enable)
        {
            if (dev->current->enable)
            {
                dev->yaw->ahead->stop_tick = dev->system->sys_tick_get() +
                                             getCycle(dev->parameter->table->yaw_ahead_time);
                dev->yaw->ahead->state = yaw_ahead_work;
            }
            else
            {
                dev->yaw->ahead->state = yaw_ahead_idle;
            }
        }
        break;
    case yaw_ahead_work:
        if (dev->yaw->enable)
        {
            if (dev->system->sys_tick_get() > dev->yaw->ahead->stop_tick)
            {
                switch (dev->yaw->change->state)
                {
                case yaw_change_forward:
                    dev->yaw->attribute.target_vel = dev->parameter->table->yaw_ahead_vel;
                    break;
                case yaw_change_backward:
                    dev->yaw->attribute.target_vel = -1 * dev->parameter->table->yaw_ahead_vel;
                    break;
                case yaw_change_forward_wait:
                    dev->yaw->attribute.target_vel = 0;
                    break;
                case yaw_change_backward_wait:
                    dev->yaw->attribute.target_vel = 0;
                    break;
                default:
                    dev->yaw->attribute.target_vel = 0;
                    break;
                }
            }
        }
        else
        {
            dev->yaw->ahead->state = yaw_ahead_idle;
        }
        break;
    }
}

static void yaw_reset(weld_opt_t *dev)
{
    dev->yaw->attribute.move_mode = 0;
    dev->yaw->attribute.target_vel = 0;
    dev->yaw->curise->state = yaw_curise_idle;
    dev->yaw->change->state = yaw_change_idle;
    dev->yaw->down->state = yaw_down_idle;
    dev->yaw->ahead->state = yaw_ahead_idle;
}

static void yaw_estop(weld_opt_t *dev)
{
    dev->yaw->attribute.move_mode = 0;
    dev->yaw->attribute.target_vel = 0;
}

static yaw_change_t yaw_change = {
    .weld_yaw_change = weld_yaw_change_position,
    .state = yaw_change_idle,
    .edge_position = 0,
    .center_position = 0,
    .stop_tick = 0,
};

static yaw_curise_t yaw_curise = {
    .weld_yaw_moving = weld_yaw_curise,
    .state = yaw_curise_idle,
};

static yaw_ahead_t yaw_ahead = {
    .state = yaw_ahead_idle,
    .weld_yaw_ahead = weld_yaw_ahead_move_2,
    .stop_tick = 0,
};

static yaw_down_t yaw_down = {
    .weld_yaw_down = weld_yaw_down_weld,
    .state = yaw_down_idle,
    .rot_init_position = 0,
};

weld_yaw_t weld_yaw = {
    .attribute = {0},
    .reset = yaw_reset,
    .curise = &yaw_curise,
    .change = &yaw_change,
    .down = &yaw_down,
    .ahead = &yaw_ahead,
    .edge_lock_enable = 0,
    .enable = 0,
    .estop = yaw_estop,
};
