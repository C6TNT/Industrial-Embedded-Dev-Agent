#include "weld_sever/weld_arc.h"
static vol_track_pid_t pid = {0};
/***************暂定所有方法为速度控制模式***************/
static void weld_arc_check(weld_opt_t *dev)
{
    dev->arc->ihs->enable = dev->parameter->table->switch_sinal & bit(12);
    dev->arc->track->enable = dev->parameter->table->switch_sinal & bit(0);
    dev->arc->lift->enable = dev->parameter->table->switch_sinal & bit(11);
}

static uint8_t weld_arc_ihs(weld_opt_t *dev)

{
    switch (dev->arc->ihs->state)
    {
    case ihs_idle:
        if (dev->parameter->table->arc_mode == 1)
        {
            dev->arc->ihs->state = ihs_second_end;
        }
        else
        {
            if (dev->arc->ihs->enable)
            {
                dev->arc->attribute.target_vel = dev->parameter->table->arc_forward_speed * dev->arc->attribute.f_transper * dev->arc->attribute.direciton;
                dev->arc->ihs->state = ihs_forward;
            }
            else
            {
                dev->arc->ihs->state = ihs_second_end;
            }
        }
        break;
    case ihs_forward:
        if (!CHECK_BIT_TURE(dev->parameter->table->DI_767[0], 6))
        {
            dev->arc->ihs->state = ihs_first_end;
            dev->arc->attribute.target_vel = 0;
        }
        break;
    case ihs_first_end:
        dev->arc->attribute.target_vel = -1 * dev->parameter->table->arc_backward_speed * dev->arc->attribute.f_transper * dev->arc->attribute.direciton;
        dev->arc->ihs->ihs_backward_end_position = dev->arc->attribute.actual_position -
                                                   dev->parameter->table->arc_backward_distance * dev->arc->attribute.f_transper * dev->arc->attribute.direciton;
        dev->arc->ihs->state = ihs_backward;
        break;
    case ihs_backward:
        if (SAFE_ABS(dev->arc->attribute.actual_position - dev->arc->ihs->ihs_backward_end_position) < POSITION_ACCURACY)
        {
            dev->arc->attribute.target_vel = 0;
            dev->arc->ihs->state = ihs_second_end;
        }
        break;
    case ihs_second_end:
        return 1;
        break;
    }
    return 0;
}
/*------------普通调整-------------*/
// static void volContrast(weld_opt_t *dev, uint16_t vol_reference)
// {
//     if ((dev->parameter->table->AI2 - vol_reference) > TRACK_VOL_ACCURACY)
//     {
//         dev->arc->attribute.target_vel = dev->parameter->table->track_speed;
//     }
//     else if ((dev->parameter->table->AI2 - vol_reference) < TRACK_VOL_ACCURACY)
//     {
//         dev->arc->attribute.target_vel = -1 * dev->parameter->table->track_speed;
//     }
//     else
//     {
//         dev->arc->attribute.target_vel = 0;
//     }
// }

/*--------------PID电压调整----------*/

static void volContrastPID(weld_opt_t *dev, uint16_t vol_reference)
{
    int error = (int)dev->parameter->table->AI_767[1] - (int)vol_reference;
    if (SAFE_ABS(error) <= TRACK_VOL_ACCURACY)
    {
        dev->arc->attribute.target_vel = 0;
        return;
    }
    // PID计算
    int P = PID_KP * error;
    // pid.integral += PID_KI * error;
    int D = PID_KD * (error - pid.prev_error);
    pid.prev_error = error;

    // 输出限幅
    int32_t output = P + D;
    int32_t normal_speed = (int32_t)(dev->parameter->table->track_speed * dev->arc->attribute.f_transper * dev->arc->attribute.direciton);
    if (output >= normal_speed)
    {
        output = normal_speed;
    }
    else if (output <= -normal_speed)
    {
        output = -1 * normal_speed;
    }
    else
    {
        output = output;
    }
    // 设置输出速度
    dev->arc->attribute.target_vel = output;
}

static void weld_arc_track(weld_opt_t *dev)
{
    switch (dev->arc->track->state)
    {
    case track_idle:
        if (dev->arc->track->enable && (dev->parameter->table->read_system_state & (STATE_RUN_WELD)) && dev->current->enable)
        {
            dev->arc->track->state = track_work;
            memset(&pid, 0, sizeof(vol_track_pid_t));
        }
        break;
    case track_work:
        if (dev->arc->track->enable && (dev->parameter->table->read_system_state & (STATE_RUN_WELD)) && dev->current->enable)
        {
            dev->arc->attribute.move_mode = 1;
            /*-------暂定为只峰值跟踪--如有需要后需添加------*/
            switch (dev->parameter->table->interval_track_mode[dev->parameter->table->section_rt])
            {
            case 0:
                if (dev->parameter->table->AO_767[2] == 0 || dev->parameter->table->AO_767[2] == 2)
                {
                    volContrastPID(dev, dev->parameter->table->interval_peak_track_vol[dev->parameter->table->section_rt]);
                }
                else
                {
                    dev->arc->attribute.target_vel = 0;
                }
                break;
            case 1:
                if (dev->parameter->table->AO_767[2] == 1)
                {
                    volContrastPID(dev, dev->parameter->table->interval_base_track_vol[dev->parameter->table->section_rt]);
                }
                else
                {
                    dev->arc->attribute.target_vel = 0;
                }
                break;
            case 2:
                if (dev->parameter->table->AO_767[2] == 0 || dev->parameter->table->AO_767[2] == 2)
                {
                    volContrastPID(dev, dev->parameter->table->interval_peak_track_vol[dev->parameter->table->section_rt]);
                }
                else if (dev->parameter->table->AO_767[2] == 1)
                {
                    volContrastPID(dev, dev->parameter->table->interval_base_track_vol[dev->parameter->table->section_rt]);
                }
                break;
            }
        }
        else
        {
            dev->arc->track->state = track_idle;
            dev->arc->attribute.target_vel = 0;
            dev->arc->attribute.move_mode = 0;
        }
        break;
    }
}

static uint8_t weld_arc_lift(weld_opt_t *dev)
{
    switch (dev->arc->lift->state)
    {
    case lift_idle:
        if (dev->arc->lift->enable)
        {
            dev->arc->lift->lift_backward_end_position = dev->arc->attribute.actual_position -
                                                         dev->parameter->table->arc_weld_lift_distance * dev->arc->attribute.f_transper * dev->arc->attribute.direciton;
            dev->arc->attribute.target_vel = -1 * dev->parameter->table->arc_backward_speed * dev->arc->attribute.f_transper * dev->arc->attribute.direciton;
            dev->arc->lift->state = lift_work;
        }
        else
        {
            dev->arc->lift->state = lift_finish;
        }
        break;
    case lift_work:
        if (SAFE_ABS(dev->arc->attribute.actual_position - dev->arc->lift->lift_backward_end_position) < POSITION_ACCURACY)
        {
            dev->arc->attribute.target_vel = 0;
            dev->arc->lift->state = lift_finish;
        }
        break;
    case lift_finish:
        return 1;
        break;
    }
    return 0;
}

static void arc_reset(weld_opt_t *dev)
{
    dev->arc->attribute.move_mode = 0;
    dev->arc->attribute.target_vel = 0;
    dev->arc->ihs->state = ihs_idle;
    dev->arc->track->state = track_idle;
    dev->arc->lift->state = lift_idle;
}
void arc_estop(weld_opt_t *dev)
{
    dev->arc->attribute.move_mode = 0;
    dev->arc->attribute.target_vel = 0;
}

static arc_ihs_t arc_ihs = {
    .weld_ihs_moving = weld_arc_ihs,
    .state = ihs_idle,
    .ihs_backward_end_position = 0,
    .enable = 0,
};

static arc_track_t arc_track = {
    .weld_track = weld_arc_track,
    .state = track_idle,
    .enable = 0,
};

static arc_lift_t arc_lift = {
    .weld_lift = weld_arc_lift,
    .state = lift_idle,
    .enable = 0,
    .lift_backward_end_position = 0,
};

weld_arc_t weld_arc = {
    .attribute = {0},
    .reset = arc_reset,
    .check = weld_arc_check,
    .lift = &arc_lift,
    .ihs = &arc_ihs,
    .track = &arc_track,
    .estop = arc_estop,
};
