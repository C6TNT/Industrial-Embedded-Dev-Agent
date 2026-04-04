#include "weld_sever/weld_gas.h"

static void weld_gas_check_weld(weld_opt_t *dev)
{
    switch (dev->gas->check->state)
    {
    case check_gas_idle:
        if (dev->parameter->table->switch_sinal & bit(8))
        {
            SET_BIT_TURE(dev->parameter->table->DO_767[0], 9);
            dev->gas->check->state = check_gas_work;
        }
        break;
    case check_gas_work:
        if (!(dev->parameter->table->switch_sinal & bit(8)))
        {
            SET_BIT_FALSE(dev->parameter->table->DO_767[0], 9);
            dev->gas->check->state = check_gas_idle;
        }
        break;
    }
}

static uint8_t weld_gas_pre_weld(weld_opt_t *dev)
{
    // dev->parameter->table->DO1
    switch (dev->gas->pre->state)
    {
    case pre_gas_idle:
        SET_BIT_TURE(dev->parameter->table->DO_767[0], 9);
        dev->gas->pre->stop_tick = dev->system->sys_tick_get() +
                                   getCycle(dev->parameter->table->gas_pre_time);
        dev->gas->pre->state = pre_gas_work;
        break;
    case pre_gas_work:
        if (dev->system->sys_tick_get() > dev->gas->pre->stop_tick)
        {
            dev->gas->pre->state = pre_gas_finsh;
        }
        break;
    case pre_gas_finsh:
        return 1;
        break;
    }
    return 0;
}

static uint8_t weld_gas_lag_weld(weld_opt_t *dev)
{
    switch (dev->gas->lag->state)
    {
    case lag_gas_idle:
        dev->gas->lag->stop_tick = dev->system->sys_tick_get() +
                                   getCycle(dev->parameter->table->gas_delay_time);
        dev->gas->lag->state = lag_gas_work;
        break;
    case lag_gas_work:
        if (dev->system->sys_tick_get() > dev->gas->lag->stop_tick)
        {
            dev->gas->lag->state = lag_gas_finsh;
            SET_BIT_FALSE(dev->parameter->table->DO_767[0], 9);
        }
        break;
    case lag_gas_finsh:
        return 1;
        break;
    }
    return 0;
}

static uint8_t weld_gas_alarm_weld(weld_opt_t *dev)
{
    switch (dev->gas->alarm->state)
    {
    case alarm_gas_idle:
        // SET_BIT_TURE(dev->parameter->table->DO1, 9);
        dev->gas->lag->stop_tick = dev->system->sys_tick_get() +
                                   getCycle(dev->parameter->table->gas_alarm_time);
        dev->gas->alarm->state = alarm_gas_work;
        break;
    case alarm_gas_work:
        if (dev->system->sys_tick_get() > dev->gas->alarm->stop_tick)
        {
            dev->gas->alarm->state = alarm_gas_finsh;
            SET_BIT_FALSE(dev->parameter->table->DO_767[0], 9);
        }
        break;
    case alarm_gas_finsh:
        return 1;
        break;
    }
    return 0;
}

static void gas_reset(weld_opt_t *dev)
{
    dev->gas->lag->state = lag_gas_idle;
    dev->gas->alarm->state = alarm_gas_idle;
    dev->gas->pre->state = pre_gas_idle;
    dev->gas->check->state = check_gas_idle;
}

static weld_gas_check_t check_gas = {
    .weld_gas_check = weld_gas_check_weld,
    .state = check_gas_idle,
};

static weld_gas_pre_t pre_gas = {
    .weld_gas_pre = weld_gas_pre_weld,
    .stop_tick = 0,
    .state = pre_gas_idle,
};

static weld_gas_lag_t lag_gas = {
    .weld_gas_lag = weld_gas_lag_weld,
    .stop_tick = 0,
    .state = lag_gas_idle,
};

static weld_gas_alarm_t alarm_gas = {
    .weld_gas_alarm = weld_gas_alarm_weld,
    .stop_tick = 0,
    .state = alarm_gas_idle,
};

weld_gas_t weld_gas = {
    .alarm = &alarm_gas,
    .lag = &lag_gas,
    .pre = &pre_gas,
    .check = &check_gas,
    .reset = gas_reset,
};
