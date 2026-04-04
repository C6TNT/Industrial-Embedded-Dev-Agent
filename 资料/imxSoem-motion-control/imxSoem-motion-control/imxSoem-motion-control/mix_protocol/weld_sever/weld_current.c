#include "weld_sever/weld_current.h"

static void weld_current_peak_base_change_weld(weld_opt_t *dev)
{
    dev->current->peak_base_change->enable = dev->parameter->table->switch_sinal & bit(3);
    dev->current->enable = dev->parameter->table->switch_sinal & bit(10);
    dev->parameter->table->AO_767[1] = dev->current->peak_base_change->enable;
    // dev->current->peak_base_change->enable = 1;//峰基切换使能持续检测
    if (!dev->current->peak_base_change->enable ||
        ((dev->parameter->table->read_system_state & (STATE_RISE_WELD |
                                                      STATE_RUN_WELD |
                                                      STATE_DOWN_WELD)) == 0))
    {
        dev->parameter->table->AO_767[2] = 0;
        return;
    }

    switch (dev->current->peak_base_change->state)
    {
    case peak_base_idle:
        dev->parameter->table->AO_767[2] = 0;
        dev->current->peak_base_change->stop_tick =
            dev->system->sys_tick_get() +
            getCycle(dev->parameter->table->interval_peak_pluse_time[dev->parameter->table->section_rt]);
        dev->current->peak_base_change->state = peak_pulse_state;
        break;

    case peak_pulse_state:
        if (dev->system->sys_tick_get() < dev->current->peak_base_change->stop_tick)
        {
            dev->parameter->table->AO_767[2] = 2;
            dev->current->peak_base_change->state = peak_pulse_state;
        }
        else
        {
            dev->current->peak_base_change->stop_tick =
                dev->system->sys_tick_get() +
                getCycle(dev->parameter->table->interval_peak_time[dev->parameter->table->section_rt]);
            dev->parameter->table->AO_767[2] = 0;
            dev->current->peak_base_change->state = peak_state;
        }
        break;
    case peak_state:
        if (dev->system->sys_tick_get() < dev->current->peak_base_change->stop_tick)
        {
            dev->current->peak_base_change->state = peak_state;
            dev->parameter->table->AO_767[2] = 0;
        }
        else
        {
            dev->current->peak_base_change->stop_tick =
                dev->system->sys_tick_get() +
                getCycle(dev->parameter->table->interval_base_time[dev->parameter->table->section_rt]);
            dev->current->peak_base_change->state = base_state;
            dev->parameter->table->AO_767[2] = 1;
        }
        break;
    case base_state:
        if (dev->system->sys_tick_get() < dev->current->peak_base_change->stop_tick)
        {
            dev->current->peak_base_change->state = base_state;
            dev->parameter->table->AO_767[2] = 1;
        }
        else
        {
            dev->current->peak_base_change->stop_tick =
                dev->system->sys_tick_get() +
                getCycle(dev->parameter->table->interval_peak_time[dev->parameter->table->section_rt]);
            dev->current->peak_base_change->state = peak_base_idle;
            dev->parameter->table->AO_767[2] = 0;
        }
        break;
    }
}

static uint8_t weld_current_pre_weld(weld_opt_t *dev)
{
    switch (dev->current->pre->state)
    {
    case pre_weld_idle:
        if (dev->current->enable)
        {
            dev->current->pre->stop_tick =
                dev->system->sys_tick_get() +
                getCycle(dev->parameter->table->current_pre_time);
            dev->current->pre->state = pre_weld_work;
            break;
        }
        else
        {
            dev->current->pre->state = pre_weld_finsh;
        }

    case pre_weld_work:
        if (dev->system->sys_tick_get() < dev->current->pre->stop_tick)
        {
            dev->parameter->table->AO_767[0] = dev->parameter->table->current_pre;
        }
        else
        {
            dev->current->pre->state = pre_weld_finsh;
        }
        break;
    case pre_weld_finsh:
        return 1; // 任务结束
        break;
    }
    return 0;
}

static uint8_t weld_current_rise_weld(weld_opt_t *dev)
{
    switch (dev->current->rise->state)
    {
    case rise_weld_idle:
        if (dev->current->enable)
        {
            dev->current->rise->stop_tick =
                dev->system->sys_tick_get() +
                getCycle(dev->parameter->table->current_rise_time);
            dev->current->rise->state = rise_weld_work;
        }
        else
        {
            dev->current->rise->state = rise_weld_finsh;
        }

        break;
    case rise_weld_work:
        if (dev->system->sys_tick_get() < dev->current->rise->stop_tick)
        {
            uint32_t final_current;
            if (dev->parameter->table->AO_767[2] == 0)
            {
                final_current = dev->parameter->table->interval_peak_current[0];
            }
            else if (dev->parameter->table->AO_767[2] == 1)
            {
                final_current = dev->parameter->table->interval_base_current[0];
            }
            else if (dev->parameter->table->AO_767[2] == 2)
            {
                final_current = dev->parameter->table->interval_peak_pluse_current[0];
            }
            else
            {
                return 1;
            }
            dev->parameter->table->AO_767[0] =
                linearFunction(
                    dev->current->rise->stop_tick - getCycle(dev->parameter->table->current_rise_time),
                    dev->parameter->table->current_pre,
                    dev->current->rise->stop_tick,
                    final_current,
                    dev->system->sys_tick_get());
            // printf("rise %d  %d\r\n",dev->parameter->table->IO_rt_current,dev->parameter->table->read_system_state);
        }
        else
        {
            dev->current->rise->state = rise_weld_finsh;
        }
        break;
    case rise_weld_finsh:
        return 1; // 任务结束
        break;
    }
    return 0;
}

static uint8_t weld_current_down_weld(weld_opt_t *dev)
{
    switch (dev->current->down->state)
    {
    case down_weld_idle:
        if (dev->current->enable)
        {
            dev->current->down->stop_tick =
                dev->system->sys_tick_get() +
                getCycle(dev->parameter->table->down_time);
            dev->current->down->state = down_weld_work;
        }
        else
        {
            dev->current->down->state = down_weld_finsh;
        }

        break;
    case down_weld_work:
        if (dev->system->sys_tick_get() < dev->current->down->stop_tick)
        {
            uint32_t start_current;
            if (dev->parameter->table->AO_767[2] == 0)
            {
                start_current = dev->parameter->table->interval_peak_current[dev->parameter->table->section_rt];
            }
            else if (dev->parameter->table->AO_767[2] == 1)
            {
                start_current = dev->parameter->table->interval_base_current[dev->parameter->table->section_rt];
            }
            else if (dev->parameter->table->AO_767[2] == 2)
            {
                start_current = dev->parameter->table->interval_peak_pluse_current[dev->parameter->table->section_rt];
            }
            else
            {
                return 1;
            }
            uint32_t down_time = dev->parameter->table->rot_down_distance / dev->parameter->table->rot_down_vel;
            dev->parameter->table->AO_767[0] =
                linearFunction(
                    dev->current->down->stop_tick - getCycle(2000),
                    start_current,
                    dev->current->down->stop_tick,
                    db_current_final,
                    dev->system->sys_tick_get());
        }
        else
        {
            dev->current->down->state = down_weld_finsh;
            SET_BIT_FALSE(dev->parameter->table->DO_767[0], 0);
            SET_BIT_FALSE(dev->parameter->table->DO_767[0], 7);
            dev->parameter->table->AO_767[0] = 0;
        }
        break;
    case down_weld_finsh:
        return 1; // 任务结束
        break;
    }
    return 0;
}

static uint8_t weld_current_curise_weld(weld_opt_t *dev)
{
    switch (dev->current->curise->state)
    {
    case curise_weld_idle:
        if (dev->current->enable)
        {
            dev->current->curise->state = curise_weld_work;
        }
        break;
    case curise_weld_work:
        switch (dev->parameter->table->AO_767[2])
        {
        case 0:
            dev->parameter->table->AO_767[0] =
                dev->parameter->table->interval_peak_current[dev->parameter->table->section_rt];
            break;
        case 1:
            dev->parameter->table->AO_767[0] =
                dev->parameter->table->interval_base_current[dev->parameter->table->section_rt];
            break;
        case 2:
            dev->parameter->table->AO_767[0] =
                dev->parameter->table->interval_peak_pluse_current[dev->parameter->table->section_rt];
            break;
        }
        // printf("curise %d  %d\r\n",dev->parameter->table->IO_rt_current,dev->parameter->table->read_system_state);
        break;
    }
    return 0;
}

static uint8_t weld_current_high_freq_weld(weld_opt_t *dev)
{
    switch (dev->current->high_freq->state)
    {
    case high_freq_weld_idle:
        if (dev->current->enable)
        {
            dev->current->high_freq->stop_tick = dev->system->sys_tick_get() + getCycle(100);
            dev->current->high_freq->state = high_freq_weld_work;
        }
        else
        {
            dev->current->high_freq->state = high_freq_weld_finsh;
        }
        break;
    case high_freq_weld_work:
        if (dev->system->sys_tick_get() <= dev->current->high_freq->stop_tick)
        {
            SET_BIT_TURE(dev->parameter->table->DO_767[0], 0);
        }
        else
        {
            dev->parameter->table->AO_767[0] = dev->parameter->table->current_arc;
            SET_BIT_TURE(dev->parameter->table->DO_767[0], 7);
            dev->current->high_freq->alarm_tick_out = dev->system->sys_tick_get() + getCycle(3000);
            dev->current->high_freq->state = high_freq_weld_check;
        }
        break;
    case high_freq_weld_check:
        if (dev->system->sys_tick_get() <= dev->current->high_freq->alarm_tick_out)
        {
            if (!CHECK_BIT_TURE(dev->parameter->table->DI_767[0], 14))
            {
                SET_BIT_FALSE(dev->parameter->table->DO_767[0], 7);
                dev->current->high_freq->state = high_freq_weld_finsh;
            }
        }
        else
        {
            SET_BIT_FALSE(dev->parameter->table->DO_767[0], 0);
            SET_BIT_FALSE(dev->parameter->table->DO_767[0], 7);
            dev->parameter->table->AO_767[0] = 0;
            dev->parameter->table->read_system_state = STATE_ALARM;
            dev->parameter->table->alarm_state = ALARM_FAIL_ARC;
            return 0;
        }
        break;
    case high_freq_weld_finsh:
        return 1;
        break;
    }
    return 0;
}

static uint8_t weld_current_lift_arc_weld(weld_opt_t *dev)
{
    switch (dev->current->lift_arc->state)
    {
    case lift_arc_weld_idle:
        if (dev->current->enable)
        {
            dev->arc->attribute.target_vel = dev->parameter->table->arc_forward_speed;
            dev->current->lift_arc->state = lift_arc_weld_forward;
        }
        else
        {
            dev->current->lift_arc->state = lift_arc_weld_second_end;
        }
        break;
    case lift_arc_weld_forward:
        if (!CHECK_BIT_TURE(dev->parameter->table->DI_767[0], 6))
        {
            dev->current->lift_arc->state = lift_arc_weld_first_end;
            dev->arc->attribute.target_vel = 0;
        }
        break;
    case lift_arc_weld_first_end:
        SET_BIT_TURE(dev->parameter->table->DO_767[0], 0);
        dev->parameter->table->AO_767[0] = dev->parameter->table->current_arc;
        dev->arc->attribute.target_vel = -1 * dev->parameter->table->arc_backward_speed;
        dev->current->lift_arc->arc_backward_end_position = dev->arc->attribute.actual_position -
                                                            dev->parameter->table->arc_backward_distance;
        dev->current->lift_arc->state = lift_arc_weld_backward;
        break;
    case lift_arc_weld_backward:
        if (SAFE_ABS(dev->arc->attribute.actual_position - dev->current->lift_arc->arc_backward_end_position) < POSITION_ACCURACY)
        {
            dev->arc->attribute.target_vel = 0;
            dev->current->lift_arc->state = lift_arc_weld_check;
        }
        break;
    case lift_arc_weld_check:
        if (!CHECK_BIT_TURE(dev->parameter->table->DI_767[0], 14))
        {
            dev->current->lift_arc->state = lift_arc_weld_second_end;
        }
        else
        {
            SET_BIT_FALSE(dev->parameter->table->DO_767[0], 0);
            dev->parameter->table->AO_767[0] = 0;
            dev->parameter->table->read_system_state = STATE_ALARM;
            dev->parameter->table->alarm_state = ALARM_FAIL_ARC;
        }
        break;
    case lift_arc_weld_second_end:
        return 1;
        break;
    }
    return 0;
}

static void current_reset(weld_opt_t *dev)
{
    dev->parameter->table->AO_767[0] = 0;
    SET_BIT_FALSE(dev->parameter->table->DO_767[0], 0);
    SET_BIT_FALSE(dev->parameter->table->DO_767[0], 7);
    dev->current->high_freq->state = high_freq_weld_idle;
    dev->current->pre->state = pre_weld_idle;
    dev->current->rise->state = rise_weld_idle;
    dev->current->down->state = down_weld_idle;
    dev->current->curise->state = curise_weld_idle;
    dev->current->lift_arc->state = lift_arc_weld_idle;
}

static void current_estop(weld_opt_t *dev)
{
    dev->parameter->table->AO_767[0] = 0;
    SET_BIT_FALSE(dev->parameter->table->DO_767[0], 0);
    SET_BIT_FALSE(dev->parameter->table->DO_767[0], 7);
}

static weld_current_high_freq_weld_t high_freq_weld = {
    .weld_current_high_freq = weld_current_high_freq_weld,
    .stop_tick = 0,
    .alarm_tick_out = 0,
    .state = high_freq_weld_idle,
};

static weld_current_lift_arc_weld_t lift_arc_weld = {
    .weld_current_lift_arc = weld_current_lift_arc_weld,
    .arc_backward_end_position = 0,
    .state = lift_arc_weld_idle,
};

static weld_current_curise_weld_t curise_weld = {
    .weld_current_curise = weld_current_curise_weld,
    .state = curise_weld_idle,
};

static weld_current_pre_weld_t pre_weld = {
    .weld_current_pre = weld_current_pre_weld,
    .stop_tick = 0,
    .state = pre_weld_idle,
};

static weld_current_rise_weld_t rise_weld = {
    .weld_current_rise = weld_current_rise_weld,
    .stop_tick = 0,
    .state = rise_weld_idle,
};

static weld_current_down_weld_t down_weld = {
    .weld_current_down = weld_current_down_weld,
    .stop_tick = 0,
    .state = down_weld_idle,
};

static weld_current_peak_base_change_t peak_base_change_weld = {
    .weld_current_peak_base_change = weld_current_peak_base_change_weld,
    .stop_tick = 0,
    .state = peak_base_idle,
    .enable = 0,
};

weld_current_t weld_current = {
    .pre = &pre_weld,
    .rise = &rise_weld,
    .curise = &curise_weld,
    .down = &down_weld,
    .high_freq = &high_freq_weld,
    .enable = 0,
    .reset = current_reset,
    .estop = current_estop,
    .peak_base_change = &peak_base_change_weld,
    .lift_arc = &lift_arc_weld,
};
