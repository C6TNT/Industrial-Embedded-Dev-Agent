#include "weld_sever/weld_coollant.h"

static void weld_coollant_check_weld(weld_opt_t *dev)
{
    switch (dev->coollant->check->state)
    {
    case check_coollant_idle:
        if (dev->parameter->table->switch_sinal & bit(9))
        {
            SET_BIT_TURE(dev->parameter->table->DO_767[0], 12);
            dev->coollant->check->state = check_coollant_work;
        }
        break;
    case check_coollant_work:
        if (!(dev->parameter->table->switch_sinal & bit(9)))
        {
            SET_BIT_FALSE(dev->parameter->table->DO_767[0], 12);
            dev->coollant->check->state = check_coollant_idle;
        }
        break;
    }
}

static uint8_t weld_coollant_work_weld(weld_opt_t *dev)
{
    // dev->parameter->table->DO1
    switch (dev->coollant->work->state)
    {
    case work_coollant_idle:
        SET_BIT_TURE(dev->parameter->table->DO_767[0], 12);
        dev->coollant->work->state = work_coollant_work;
        break;
    case work_coollant_work:
        dev->coollant->work->state = work_coollant_finsh;
        break;
    case work_coollant_finsh:
        return 1;
        break;
    }
    return 0;
}

static uint8_t weld_coollant_finish_weld(weld_opt_t *dev)
{
    // dev->parameter->table->DO1
    switch (dev->coollant->finish->state)
    {
    case end_coollant_idle:
        dev->coollant->finish->stop_tick = dev->system->sys_tick_get() +
                                           getCycle(dev->parameter->table->coollant_alarm_time);
        dev->coollant->finish->state = end_coollant_work;
        break;
    case end_coollant_work:
        if (dev->system->sys_tick_get() > dev->coollant->finish->stop_tick)
        {
            SET_BIT_FALSE(dev->parameter->table->DO_767[0], 12);
            dev->coollant->finish->state = end_coollant_finsh;
        }
        break;
    case end_coollant_finsh:
        return 1;
        break;
    }
    return 0;
}

static uint8_t weld_coollant_alarm_weld(weld_opt_t *dev)
{
    switch (dev->coollant->alarm->state)
    {
    case alarm_coollant_idle:
        // SET_BIT_TURE(dev->parameter->table->DO1, 12);
        dev->coollant->alarm->stop_tick = dev->system->sys_tick_get() +
                                          getCycle(dev->parameter->table->coollant_alarm_time);
        dev->coollant->alarm->state = alarm_coollant_work;
        break;
    case alarm_coollant_work:
        if (dev->system->sys_tick_get() > dev->coollant->alarm->stop_tick)
        {
            SET_BIT_FALSE(dev->parameter->table->DO_767[0], 12);
            dev->coollant->alarm->state = alarm_coollant_finsh;
        }
        break;
    case alarm_coollant_finsh:
        return 1;
        break;
    }
    return 0;
}

static void coollant_reset(weld_opt_t *dev)
{
    dev->coollant->work->state = work_coollant_idle;
    dev->coollant->alarm->state = alarm_coollant_idle;
    dev->coollant->finish->state = end_coollant_idle;
    dev->coollant->check->state = check_coollant_idle;
}
static weld_coollant_work_t work_coollant = {
    .weld_coollant_work = weld_coollant_work_weld,
    .state = work_coollant_idle,
};

static weld_coollant_check_t check_coollant = {
    .weld_coollant_check = weld_coollant_check_weld,
    .state = check_coollant_idle,
};
static weld_coollant_alarm_t alarm_coollant = {
    .weld_coollant_alarm = weld_coollant_alarm_weld,
    .stop_tick = 0,
    .state = alarm_coollant_idle,
};

static weld_coollant_finish_t finish_coollant = {
    .weld_coollant_finish = weld_coollant_finish_weld,
    .stop_tick = 0,
    .state = end_coollant_idle,
};

weld_coollant_t weld_coollant = {
    .finish = &finish_coollant,
    .alarm = &alarm_coollant,
    .work = &work_coollant,
    .check = &check_coollant,
    .reset = coollant_reset,
};
