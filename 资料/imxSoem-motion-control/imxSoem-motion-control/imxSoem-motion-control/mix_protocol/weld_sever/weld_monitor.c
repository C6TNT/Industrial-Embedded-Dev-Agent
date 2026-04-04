#include "weld_sever/weld_monitor.h"
#include "ethercat.h"
/***************暂定所有方法为速度控制模式***************/

static uint8_t weld_arc_protect_move(weld_opt_t *dev)
{
    switch (dev->monitor->arc->state)
    {
    case arc_protect_move_idle:
        dev->arc->attribute.state = move_weld;
        dev->monitor->arc->state = arc_protect_move_work;
        dev->monitor->arc->end_position = dev->arc->attribute.actual_position -
                                          2.0 * dev->arc->attribute.f_transper * dev->arc->attribute.direciton;
        break;
    case arc_protect_move_work:
        if (dev->arc->attribute.actual_position * dev->arc->attribute.direciton <= dev->monitor->arc->end_position * dev->arc->attribute.direciton)
        {
            dev->arc->attribute.target_vel = 0;
            dev->arc->attribute.state = move_idle;
            dev->monitor->arc->state = arc_protect_move_finsh;
            dev->parameter->table->read_system_state = STATE_RESET;
        }
        else
        {
            dev->arc->attribute.target_vel = -1 * 3.33 * dev->arc->attribute.f_transper * dev->arc->attribute.direciton;
        }
        break;
    case arc_protect_move_finsh:
        return 1;
        break;
    }
    return 0;
}

static void weld_monitor_work(weld_opt_t *dev)
{
    /*按序按状态检测*/
    if (dev->system->sys_tick_get() > 2)
    {
        /*---------------主站掉线检测-----------------------*/
        if (dev->parameter->table->ec_slave_heart_beat_wkc != ec_slavecount * 3)
        {
            dev->parameter->table->read_system_state = STATE_ALARM;
            dev->parameter->table->alarm_state = ALARM_ETHERCAT_DISCONNNECT;
            return;
        }
        /***************短路保护*************/
        if (!CHECK_BIT_TURE(dev->parameter->table->DI_767[0], 6))
        {
            if (dev->parameter->table->read_system_state == STATE_IDLE_WELD)
            {
                dev->monitor->work->arc_protect_move_enable = 1;
                dev->parameter->table->read_system_state = STATE_ALARM;
                dev->parameter->table->alarm_state = ALARM_HAND_SHORT_CIRCUIT;
            }
            else if (dev->parameter->table->read_system_state & (STATE_PRE_WELD |
                                                                 STATE_RISE_WELD |
                                                                 STATE_RUN_WELD |
                                                                 STATE_DOWN_WELD |
                                                                 STATE_LIFT_WELD |
                                                                 STATE_DELAY_GAS_WELD))
            {
                dev->parameter->table->read_system_state = STATE_ALARM;
                dev->parameter->table->alarm_state = ALARM_SHORT_CIRCUIT;
            }
        }
        /*****************断弧保护*****************/
        if (dev->current->enable)
        {
            if (CHECK_BIT_TURE(dev->parameter->table->DI_767[0], 14))
            {
                if (dev->parameter->table->read_system_state & (STATE_PRE_WELD |
                                                                STATE_RISE_WELD |
                                                                STATE_RUN_WELD))
                {
                    dev->parameter->table->read_system_state = STATE_ALARM;
                    dev->parameter->table->alarm_state = ALARM_BREAK_ARC;
                }
            }
        }

        /****************缺丝保护*****************/
        /****************缺水保护*****************/
        /****************缺气保护*****************/
        /****************机芯保护*****************/
        if (CHECK_BIT_TURE(dev->parameter->table->DI_767[0], 5))
        {
            dev->parameter->table->read_system_state = STATE_ALARM;
            dev->parameter->table->alarm_state = ALARM_MACHINE_ERROR;
        }
        /****************限位保护*****************/
        /*按序执行*/
        if (dev->monitor->work->arc_protect_move_enable)
        {
            if (dev->monitor->arc->arc_protect_move(dev))
            {
                dev->monitor->work->arc_protect_move_enable = 0;
                dev->monitor->arc->state = arc_protect_move_idle;
            }
        }
    }
}
static void weld_monitor_alarm(weld_opt_t *dev)
{
    switch (dev->monitor->alarm->state)
    {
    case monitor_alarm_idle:
        dev->angle->estop(dev);
        dev->arc->estop(dev);
        dev->current->estop(dev);
        dev->rot->estop(dev);
        dev->wire->estop(dev);
        dev->yaw->estop(dev);
        dev->monitor->alarm->state = monitor_alarm_wait;
        break;
    case monitor_alarm_wait:
        dev->coollant->alarm->weld_coollant_alarm(dev);
        dev->gas->alarm->weld_gas_alarm(dev);
        break;
    }
}
static void monitor_reset(weld_opt_t *dev)
{
    dev->monitor->alarm->state = monitor_alarm_idle;
}
static arc_protect_move_t apm = {
    .arc_protect_move = weld_arc_protect_move,
    .state = arc_protect_move_idle,
    .end_position = 0,
};

static monitor_work_t monitor_work = {
    .monitor_work = weld_monitor_work,
    .arc_protect_move_enable = 0,
};

static monitor_alarm_t monitor_alarm = {
    .monitor_alarm = weld_monitor_alarm,
    .state = monitor_alarm_idle,
};

weld_monitor_t weld_monitor = {
    .work = &monitor_work,
    .arc = &apm,
    .alarm = &monitor_alarm,
    .reset = monitor_reset,
};
