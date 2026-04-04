#include "weld_sever/weld_wire.h"

/***************暂定所有方法为速度控制模式***************/
static void weld_wire_check(weld_opt_t *dev)
{
    dev->wire->enable = dev->parameter->table->switch_sinal & bit(1);
}

static uint8_t weld_wire_curise(weld_opt_t *dev)
{
    switch (dev->wire->curise->state)
    {
    case wire_curise_idle:
        if (dev->wire->enable && dev->parameter->table->read_system_state == STATE_RUN_WELD)
        {
            dev->wire->curise->state = wire_curise_work_ready;
        }
        break;
    case wire_curise_work_ready:
        if (dev->wire->enable)
        {
            dev->wire->curise->state = wire_curise_work;
        }
        else
        {
            dev->wire->curise->state = wire_curise_work_ready;
        }
        break;
    case wire_curise_work:
        if (dev->wire->enable)
        {
            switch (dev->parameter->table->AO_767[2])
            {
            case 0:
            case 2:
                dev->wire->attribute.target_vel = dev->parameter->table->interval_peak_wire_vel[dev->parameter->table->section_rt] *
                                                  dev->wire->attribute.f_transper * dev->wire->attribute.direciton;
                break;
            case 1:
                dev->wire->attribute.target_vel = dev->parameter->table->interval_base_wire_vel[dev->parameter->table->section_rt] *
                                                  dev->wire->attribute.f_transper * dev->wire->attribute.direciton;
                break;
            }
        }
        else
        {

            dev->wire->attribute.target_vel = 0;
            dev->wire->curise->state = wire_curise_work_ready;
        }
        break;
    }
    return 0;
}

static uint8_t weld_wire_pre_weld(weld_opt_t *dev)
{
    switch (dev->wire->pre->state)
    {
    case wire_pre_idle:
        if (dev->wire->enable)
        {
            if (dev->current->enable)
            {
                dev->wire->pre->state = wire_pre_work;
                dev->wire->pre->stop_tick = dev->system->sys_tick_get() +
                                            getCycle(dev->parameter->table->wire_pre_time);
            }
            else
            {
                dev->wire->pre->state = wire_pre_finish;
            }
        }
        else
        {
            dev->wire->pre->state = wire_pre_idle;
        }
        break;
    case wire_pre_work:
        if (dev->wire->enable)
        {
            if (dev->system->sys_tick_get() >= dev->wire->pre->stop_tick)
            {
                dev->wire->pre->state = wire_pre_finish;
                dev->wire->attribute.target_vel = 0;
            }
            else
            {
                dev->wire->attribute.target_vel = dev->parameter->table->wire_pre_vel * dev->wire->attribute.f_transper * dev->wire->attribute.direciton;
                dev->wire->attribute.move_mode = 1;
            }
        }
        else
        {
            dev->wire->pre->state = wire_pre_idle;
            dev->wire->attribute.target_vel = 0;
        }

        break;
    case wire_pre_finish:
        return 1;
        break;
    }
    return 0;
}

static uint8_t weld_wire_extract_weld(weld_opt_t *dev)
{
    switch (dev->wire->extract->state)
    {
    case wire_extract_idle:
        if (dev->wire->enable)
        {
            if ((dev->parameter->table->read_system_state == STATE_DOWN_WELD))
            {
                dev->wire->extract->state = wire_extract_work;
                dev->wire->extract->stop_tick = dev->system->sys_tick_get() +
                                                getCycle(dev->parameter->table->wire_extract_time);
            }
            else
            {
                return 1;
            }
        }
        else
        {
            dev->wire->extract->state = wire_extract_finish;
        }
        break;
    case wire_extract_work:
        if (dev->system->sys_tick_get() < dev->wire->extract->stop_tick)
        {
            dev->wire->attribute.target_vel = -1 * dev->parameter->table->wire_extract_vel * dev->wire->attribute.f_transper * dev->wire->attribute.direciton;
        }
        else
        {
            dev->wire->attribute.target_vel = 0;
            dev->wire->extract->state = wire_extract_finish;
        }
        break;
    case wire_extract_finish:
        return 1;
        break;
    }
    return 0;
}

static void wire_reset(weld_opt_t *dev)
{
    dev->wire->attribute.move_mode = 0;
    dev->wire->attribute.target_vel = 0;
    dev->wire->curise->state = wire_curise_idle;
    dev->wire->pre->state = wire_pre_idle;
    dev->wire->extract->state = wire_extract_idle;
}

static void wire_estop(weld_opt_t *dev)
{
    dev->wire->attribute.move_mode = 0;
    dev->wire->attribute.target_vel = 0;
}

static wire_curise_t wire_curise = {
    .weld_wire_moving = weld_wire_curise,
    .state = wire_curise_idle,
    .rot_end_position = 0,
    .rot_start_position = 0,
    .start_section = 0,
    .end_section = 0,
};

// static wire_back_t wire_back = {
//     .weld_wire_back = weld_wire_back,
//     .state = wire_back_idle,
//     .rot_end_position = 0,
// };

static wire_pre_t wire_pre = {
    .weld_wire_pre = weld_wire_pre_weld,
    .stop_tick = 0,
    .state = wire_pre_idle,
};

static wire_extract_t wire_extract = {
    .weld_wire_extract = weld_wire_extract_weld,
    .stop_tick = 0,
    .state = wire_extract_idle,
};

weld_wire_t weld_wire = {
    .attribute = {0},
    .reset = wire_reset,
    .curise = &wire_curise,
    .pre = &wire_pre,
    .extract = &wire_extract,
    .enable = 0,
    .check = weld_wire_check,
    .estop = wire_estop,
};
