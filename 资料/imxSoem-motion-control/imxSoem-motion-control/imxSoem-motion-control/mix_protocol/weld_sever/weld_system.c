#include "weld_sever/weld_system.h"

static weld_task_block_t weld_task_block = {0};
static weld_opt_t dev_memory = {0};

static sys_state_t state_get_weld(weld_opt_t *dev)
{
    return dev->parameter->table->read_system_state;
}

static uint32_t sys_tick_get_weld()
{
    return weld_task_block.sys_tick;
}
static bool weld_rot_direct_can_active(weld_opt_t *dev)
{
    if ((dev == NULL) || (dev->rot == NULL))
    {
        return false;
    }

    return ((dev->rot->attribute.control_mode == 15) &&
            (dev->rot->attribute.mode_of_operation == 3) &&
            ((dev->rot->attribute.target_vel != 0) ||
             (dev->rot->attribute.f_target_vel != 0.0) ||
             (dev->rot->attribute.acc != 0U) ||
             (dev->rot->attribute.dec != 0U)));
}
#if USE_TUBE_TUBE == 1
static void weld_task_tube(weld_opt_t *dev)
{
    uint8_t taskhandler[5]; // 任务完成标志  不具备结束标志的任务无须返回值
    // 上升沿检测
    bool rotDirectCanActive = weld_rot_direct_can_active(dev);
    detectRisingEdges(dev->parameter->table->write_system_state, &weld_task_block.prep_state, &weld_task_block.exec_state);
    if (weld_task_block.exec_state & EXEC_STATE_RESET)
    {
        dev->parameter->table->read_system_state = STATE_RESET;
        // dev->rot->gohome->state = rot_go_home_idle;
    }
    if (weld_task_block.exec_state & EXEC_STATE_START)
    {
        if (dev->parameter->table->read_system_state == STATE_IDLE_WELD)
        {
            dev->parameter->table->read_system_state = STATE_PRE_GAS_WELD;
            // dev->parameter->table->yaw_center_positiopn = dev->yaw->attribute.actual_position;
        }
    }
    if (weld_task_block.exec_state & EXEC_STATE_STOP)
    {
        if (dev->parameter->table->read_system_state & (STATE_RUN_WELD))
        {
            dev->parameter->table->read_system_state = STATE_DOWN_WELD;
            // dev->rot->final_interval_flag = 1;
        }
        else
        {
            dev->parameter->table->read_system_state = STATE_RESET;
        }
    }
    if (weld_task_block.exec_state & EXEC_STATE_ESTOP)
    {
        dev->parameter->table->read_system_state = STATE_ALARM;
    }
    memset(&weld_task_block.exec_state, 0, sizeof(weld_task_block.exec_state));
    if (!rotDirectCanActive)
    {
        dev->jog->move_loop(dev, &dev->rot->attribute);
    }
    dev->jog->move_loop(dev, &dev->wire->attribute);
    dev->jog->move_loop(dev, &dev->angle->attribute);
    dev->jog->move_loop(dev, &dev->arc->attribute);
    dev->jog->move_loop(dev, &dev->yaw->attribute);
    dev->jog->jog_home->jog_home(dev);
    dev->monitor->work->monitor_work(dev); // 状态监测
    switch (dev->parameter->table->read_system_state)
    {
    case STATE_RESET:
        // 复位所有功能状态机
        dev->parameter->table->alarm_state = ALARM_IDLE;
        if (!rotDirectCanActive)
        {
            dev->rot->reset(dev);
        }
        dev->wire->reset(dev);
        dev->arc->reset(dev);
        dev->yaw->reset(dev);
        dev->gas->reset(dev);
        dev->current->reset(dev);
        dev->coollant->reset(dev);
        dev->angle->reset(dev);
        dev->monitor->reset(dev);
        dev->parameter->table->section_rt = 0; // 回到第一区间
        dev->parameter->table->read_system_state = STATE_IDLE_WELD;
        break;
    case STATE_IDLE_WELD:
        /*------------空闲执行水任务------------*/
        dev->coollant->finish->weld_coollant_finish(dev);
        break;
    case STATE_PRE_GAS_WELD:
        taskhandler[0] = dev->gas->pre->weld_gas_pre(dev);
        taskhandler[1] = dev->coollant->work->weld_coollant_work(dev);
        taskhandler[2] = dev->arc->ihs->weld_ihs_moving(dev);
        if (taskhandler[0] == 1 &&
            taskhandler[1] == 1 &&
            taskhandler[2] == 1)
        {
            dev->parameter->table->read_system_state = STATE_ARC_WELD;
            // PRINTF("section0 %d\r\n", dev->parameter->table->section_rt);
        }
        break;
    case STATE_ARC_WELD:
        if (dev->parameter->table->arc_mode == 1)
        {
            taskhandler[0] = dev->current->lift_arc->weld_current_lift_arc(dev);
            if (taskhandler[0] == 1)
            {
                dev->parameter->table->read_system_state = STATE_PRE_WELD;
                // PRINTF("1 %d\r\n", dev->parameter->table->section_rt);
            }
        }
        else
        {
            taskhandler[0] = dev->current->high_freq->weld_current_high_freq(dev);
            if (taskhandler[0] == 1)
            {
                dev->parameter->table->read_system_state = STATE_PRE_WELD;
                // PRINTF("1 %d\r\n", dev->parameter->table->section_rt);
            }
        }
        break;
    case STATE_PRE_WELD:
        taskhandler[0] = dev->current->pre->weld_current_pre(dev);
        dev->wire->pre->weld_wire_pre(dev);
        dev->yaw->ahead->weld_yaw_ahead(dev);
        if (taskhandler[0] == 1)
        {
            dev->parameter->table->read_system_state = STATE_RISE_WELD;
            // PRINTF("2 %d\r\n", dev->parameter->table->section_rt);
        }
        break;
    case STATE_RISE_WELD:
        taskhandler[0] = dev->current->rise->weld_current_rise(dev);
        dev->wire->pre->weld_wire_pre(dev);
        dev->yaw->ahead->weld_yaw_ahead(dev);
        if (taskhandler[0] == 1)
        {
            dev->parameter->table->read_system_state = STATE_RUN_WELD;
            // PRINTF("3 %d\r\n", dev->parameter->table->section_rt);
        }
        break;
    case STATE_RUN_WELD:
        dev->rot->curise_tube->weld_rot_moving(dev);
        dev->current->curise->weld_current_curise(dev);
        dev->yaw->curise->weld_yaw_moving(dev);
        dev->wire->curise->weld_wire_moving(dev);
        dev->arc->track->weld_track(dev);
        break;
    case STATE_DOWN_WELD:
        dev->arc->attribute.target_vel = 0; // 衰减消除残余跟踪速度
        taskhandler[0] = dev->rot->down->weld_rot_down(dev);
        taskhandler[1] = dev->yaw->down->weld_yaw_down(dev);
        taskhandler[2] = dev->current->down->weld_current_down(dev);
        taskhandler[3] = dev->wire->extract->weld_wire_extract(dev);
        if (taskhandler[0] == 1 &&
            taskhandler[1] == 1 &&
            taskhandler[2] == 1 &&
            taskhandler[3] == 1)
        {
            dev->parameter->table->read_system_state = STATE_DELAY_GAS_WELD;
        }

        break;
    case STATE_DELAY_GAS_WELD:
        taskhandler[0] = dev->gas->lag->weld_gas_lag(dev);
        if (taskhandler[0] == 1)
        {
            dev->parameter->table->read_system_state = STATE_LIFT_WELD;
        }
        break;
    case STATE_LIFT_WELD:
        taskhandler[0] = dev->arc->lift->weld_lift(dev);
        if (taskhandler[0] == 1)
        {
            dev->parameter->table->read_system_state = STATE_RESET;
        }
        break;
    case STATE_ALARM:
        dev->monitor->alarm->monitor_alarm(dev);
        break;
    case STATE_SECOND_START:
        dev->parameter->table->alarm_state = ALARM_IDLE;
        /*注意使用二次启动复位旋转轴相关方法*/
        if (!rotDirectCanActive)
        {
            dev->rot->second_start_reset(dev);
        }
        dev->wire->reset(dev);
        dev->arc->reset(dev);
        dev->yaw->reset(dev);
        dev->gas->reset(dev);
        dev->current->reset(dev);
        dev->coollant->reset(dev);
        dev->angle->reset(dev);
        dev->monitor->reset(dev);
        dev->parameter->table->read_system_state = STATE_SECOND_START_TRANSITION;
        break;
    case STATE_SECOND_START_TRANSITION:
        taskhandler[0] = dev->rot->second_start_transition->weld_rot_second_start_transition(dev);
        if (taskhandler[0] == 1)
        {
            dev->parameter->table->read_system_state = STATE_PRE_GAS_WELD;
        }
        break;
    }

/*-------开关量常驻检测---------*/
#if ANGLE
    dev->angle->curise->weld_angle_moving(dev);
#endif
    dev->coollant->check->weld_coollant_check(dev);                     // 水检开关
    dev->gas->check->weld_gas_check(dev);                               // 气检开关
    dev->arc->check(dev);                                               // 跟踪开关，碰工件开关和焊后抬升开关
    dev->wire->check(dev);                                              // 送丝开关
    dev->current->peak_base_change->weld_current_peak_base_change(dev); // 脉冲开关和模拟焊开关
    dev->yaw->change->weld_yaw_change(dev);                             // 横摆和边缘跟随开关
    dev->rot->gohome->weld_rot_go_home(dev);                            // 回原点开关以及回原点操作
    dev->rot->check->weld_rot_encoder_check(dev);                       // 64位dec持续更新
    weld_task_block.sys_tick = weld_task_block.sys_tick + 1;
}
#endif

static weld_system_t weld_system = {
    .state_get = state_get_weld,
    .sys_tick_get = sys_tick_get_weld,
    .weld_task = weld_task_tube,
};

weld_opt_t *weld_task_init()
{
    weld_opt_t *dev = &dev_memory;
    if (dev == NULL)
    {
        PRINTF("no spare on device\r\n");
        return NULL;
    }

    dev->system = &weld_system;
    dev->parameter = &weld_parameter;
    dev->current = &weld_current;
    dev->gas = &weld_gas;
    dev->coollant = &weld_coollant;
    dev->arc = &weld_arc;
    dev->rot = &weld_rot;
    dev->wire = &weld_wire;
    dev->yaw = &weld_yaw;
    dev->angle = &weld_angle;
    dev->monitor = &weld_monitor;
    dev->joint = weld_joint;
    dev->control = &position_control;
    dev->jog = &weld_jog;
    dev->kinematic = &weld_kinematic;
    dev->ethercat_control = &ethercat_control;
#if QUICK_COMMUNICATION_PROTOCOL
    dev->flag_rpmsg = 1;
#endif
#if COMMON_COMMUNICATION_PROTOCOL == 1
    dev->rpmsg_handler_flag = 0;
    dev->inOP = 0;
    memset(&dev->recv_buf_to_a53, 0, sizeof(dev->recv_buf_to_a53));
    memset(&dev->send_buf_to_motor, 0, sizeof(dev->send_buf_to_motor));
    dev->xMutex = xSemaphoreCreateMutex();
    dev->ETHMutex = xSemaphoreCreateMutex();
#endif

    memset(&weld_task_block, 0, sizeof(weld_task_block_t));
    return dev;
}

void debug_for_weld(weld_opt_t *dev)
{
    // dev->parameter->table_write(dev, 0x1200, 0x0, 32, db_section_overall);      // 焊接总区间2
    // dev->parameter->table_write(dev, 0x1200, 0x2, 32, db_section_speed_change); // 焊接第2区间换向

    // dev->parameter->table_write(dev, 0x1100, 0x0, 32, db_interval_peak_current); // 1峰值电流
    // dev->parameter->table_write(dev, 0x1100, 0x1, 32, db_interval_base_current); // 1峰值电流
    // dev->parameter->table_write(dev, 0x1101, 0x0, 32, db_interval_peak_current); // 2峰值电流
    // dev->parameter->table_write(dev, 0x1101, 0x1, 32, db_interval_base_current); // 2峰值电流
    // dev->parameter->table_write(dev, 0x1102, 0x0, 32, db_interval_peak_current); // 3峰值电流
    // dev->parameter->table_write(dev, 0x1102, 0x1, 32, db_interval_base_current); // 3峰值电流
    // dev->parameter->table_write(dev, 0x1103, 0x0, 32, db_interval_peak_current); // 4峰值电流
    // dev->parameter->table_write(dev, 0x1103, 0x1, 32, db_interval_base_current); // 4峰值电流
    // dev->parameter->table_write(dev, 0x1104, 0x0, 32, db_interval_peak_current); // 5峰值电流
    // dev->parameter->table_write(dev, 0x1104, 0x1, 32, db_interval_base_current); // 5峰值电流

    // dev->parameter->table_write(dev, 0x1100, 0x2, 32, db_interval_peak_time); // 1峰值
    // dev->parameter->table_write(dev, 0x1100, 0x3, 32, db_interval_base_time); // 1基值
    // dev->parameter->table_write(dev, 0x1101, 0x2, 32, db_interval_peak_time); // 2峰值
    // dev->parameter->table_write(dev, 0x1101, 0x3, 32, db_interval_base_time); // 2基值
    // dev->parameter->table_write(dev, 0x1102, 0x2, 32, db_interval_peak_time); // 3峰值
    // dev->parameter->table_write(dev, 0x1102, 0x3, 32, db_interval_base_time); // 3基值
    // dev->parameter->table_write(dev, 0x1103, 0x2, 32, db_interval_peak_time); // 4峰值
    // dev->parameter->table_write(dev, 0x1103, 0x3, 32, db_interval_base_time); // 4基值
    // dev->parameter->table_write(dev, 0x1104, 0x2, 32, db_interval_peak_time); // 5峰值
    // dev->parameter->table_write(dev, 0x1104, 0x3, 32, db_interval_base_time); // 5基值

    // dev->parameter->table_write(dev, 0x1203, 0x5, 32, db_current_arc);                 // 起弧电流
    // dev->parameter->table_write(dev, 0x1203, 0x3, 32, db_current_rise_time);           // 上升时间
    // dev->parameter->table_write(dev, 0x1203, 0x4, 32, db_down_time);                   // 衰减时间
    // dev->parameter->table_write(dev, 0x1203, 0x1, 32, db_current_pre);                 // 预熔电流
    // dev->parameter->table_write(dev, 0x1203, 0x2, 32, db_current_pre_time);            // 预熔时间
    // dev->parameter->table_write(dev, 0x1201, 0x1, 32, db_rot_down_vel);                // 衰减速度
    // dev->parameter->table_write(dev, 0x1203, 0x6, 32, db_arc_forward_speed);           // 碰件速度
    // dev->parameter->table_write(dev, 0x1203, 0x7, 32, db_arc_backward_speed);          // 碰件返回速度
    // dev->parameter->table_write(dev, 0x1203, 0x8, 32, db_arc_backward_distance);       // 碰件返回距离
    // dev->parameter->table_write(dev, 0x1203, 0x9, 32, db_arc_weld_lift_distance);      // 焊后抬升距离
    // dev->parameter->table_write(dev, 0x1203, 0xa, 32, db_yaw_center_position);         // 焊后抬升距离
    // dev->parameter->table_write(dev, 0x1203, 0xb, 32, db_arc_mode);                    // 引弧方式
    // dev->parameter->table_write(dev, 0x1203, 0xc, 32, 0);                              // 角度零点
    // dev->parameter->table_write(dev, 0x1202, 0x0, 32, db_gas_pre_time);                // 预送气时间
    // dev->parameter->table_write(dev, 0x1202, 0x1, 32, db_gas_delay_time);              // 滞后送气时间
    // dev->parameter->table_write(dev, 0x1206, 0x0, 32, db_wire_pre_time);               // 提前送丝速度
    // dev->parameter->table_write(dev, 0x1206, 0x1, 32, db_wire_pre_vel);                // 提前送丝时间
    // dev->parameter->table_write(dev, 0x1206, 0x3, 32, db_wire_extract_vel);            // 抽丝速度
    // dev->parameter->table_write(dev, 0x1206, 0x4, 32, db_wire_extract_time);           // 抽丝时间
    // dev->parameter->table_write(dev, 0x1206, 0x5, 32, db_wire_pre_start_distance);     // 送丝开始位置
    // dev->parameter->table_write(dev, 0x1206, 0x6, 32, db_wire_extract_start_distance); // 抽丝开始位置

    // dev->parameter->table_write(dev, 0x1207, 0x0, 32, db_yaw_ahead_vel);       // 提前横摆速度
    // dev->parameter->table_write(dev, 0x1207, 0x1, 32, db_yaw_ahead_distance);  // 提前横摆宽度
    // dev->parameter->table_write(dev, 0x1207, 0x2, 32, db_yaw_ahead_time);      // 横摆开始时间
    // dev->parameter->table_write(dev, 0x1208, 0x0, 32, db_coollant_alarm_time); // 水冷时间

    // dev->parameter->table_write(dev, 0x1205, 0x2, 32, db_track_speed); // 跟踪速度

    // dev->parameter->table_write(dev, 0x1100, 0x8, 32, db_interval_yaw_vel);           // 1横摆速度
    // dev->parameter->table_write(dev, 0x1100, 0x9, 32, db_interval_yaw_distance);      // 1横摆宽度
    // dev->parameter->table_write(dev, 0x1100, 0xa, 32, db_interval_yaw_forward_time);  // 1横摆左边缘停留时间
    // dev->parameter->table_write(dev, 0x1100, 0xb, 32, db_interval_yaw_backward_time); // 1横摆右边缘停留时间
    // dev->parameter->table_write(dev, 0x1101, 0x8, 32, db_interval_yaw_vel);           // 2横摆速度
    // dev->parameter->table_write(dev, 0x1101, 0x9, 32, db_interval_yaw_distance);      // 2横摆宽度
    // dev->parameter->table_write(dev, 0x1101, 0xa, 32, db_interval_yaw_forward_time);  // 2横摆左边缘停留时间
    // dev->parameter->table_write(dev, 0x1101, 0xb, 32, db_interval_yaw_backward_time); // 2横摆右边缘停留时间
    // dev->parameter->table_write(dev, 0x1102, 0x8, 32, db_interval_yaw_vel);           // 3横摆速度
    // dev->parameter->table_write(dev, 0x1102, 0x9, 32, db_interval_yaw_distance);      // 3横摆宽度
    // dev->parameter->table_write(dev, 0x1102, 0xa, 32, db_interval_yaw_forward_time);  // 3横摆左边缘停留时间
    // dev->parameter->table_write(dev, 0x1102, 0xb, 32, db_interval_yaw_backward_time); // 3横摆右边缘停留时间
    // dev->parameter->table_write(dev, 0x1103, 0x8, 32, db_interval_yaw_vel);           // 4横摆速度
    // dev->parameter->table_write(dev, 0x1103, 0x9, 32, db_interval_yaw_distance);      // 4横摆宽度
    // dev->parameter->table_write(dev, 0x1103, 0xa, 32, db_interval_yaw_forward_time);  // 4横摆左边缘停留时间
    // dev->parameter->table_write(dev, 0x1103, 0xb, 32, db_interval_yaw_backward_time); // 4横摆右边缘停留时间
    // dev->parameter->table_write(dev, 0x1104, 0x8, 32, db_interval_yaw_vel);           // 5横摆速度
    // dev->parameter->table_write(dev, 0x1104, 0x9, 32, db_interval_yaw_distance);      // 5横摆宽度
    // dev->parameter->table_write(dev, 0x1104, 0xa, 32, db_interval_yaw_forward_time);  // 5横摆左边缘停留时间
    // dev->parameter->table_write(dev, 0x1104, 0xb, 32, db_interval_yaw_backward_time); // 5横摆右边缘停留时间

    // dev->parameter->table_write(dev, 0x1100, 0xc, 32, db_interval_peak_rot_vel);     // 1旋转峰值速度
    // dev->parameter->table_write(dev, 0x1100, 0xd, 32, db_interval_base_rot_vel);     // 1旋转基值速度
    // dev->parameter->table_write(dev, 0x1100, 0xe, 32, db_interval_rot_distance);     // 1旋转行程
    // dev->parameter->table_write(dev, 0x1101, 0xc, 32, db_interval_peak_rot_vel);     // 2旋转峰值速度
    // dev->parameter->table_write(dev, 0x1101, 0xd, 32, db_interval_base_rot_vel);     // 2旋转基值速度
    // dev->parameter->table_write(dev, 0x1101, 0xe, 32, db_interval_rot_distance);     // 2旋转行程
    // dev->parameter->table_write(dev, 0x1102, 0xc, 32, db_interval_peak_rot_vel);     // 3旋转峰值速度
    // dev->parameter->table_write(dev, 0x1102, 0xd, 32, db_interval_base_rot_vel);     // 3旋转基值速度
    // dev->parameter->table_write(dev, 0x1102, 0xe, 32, db_interval_rot_distance);     // 3旋转行程
    // dev->parameter->table_write(dev, 0x1103, 0xc, 32, db_interval_peak_rot_vel);     // 4旋转峰值速度
    // dev->parameter->table_write(dev, 0x1103, 0xd, 32, db_interval_base_rot_vel);     // 4旋转基值速度
    // dev->parameter->table_write(dev, 0x1103, 0xe, 32, db_interval_rot_distance);     // 4旋转行程
    // dev->parameter->table_write(dev, 0x1104, 0xc, 32, db_interval_peak_rot_vel);     // 5旋转峰值速度
    // dev->parameter->table_write(dev, 0x1104, 0xd, 32, db_interval_base_rot_vel);     // 5旋转基值速度
    // dev->parameter->table_write(dev, 0x1104, 0xe, 32, db_interval_rot_distance * 4); // 5旋转行程

    // dev->parameter->table_write(dev, 0x1100, 0x6, 32, db_interval_peak_wire_vel); // 1峰值送丝速度
    // dev->parameter->table_write(dev, 0x1100, 0xf, 32, db_interval_base_wire_vel); // 1基值送丝速度
    // dev->parameter->table_write(dev, 0x1101, 0x6, 32, db_interval_peak_wire_vel); // 2峰值送丝速度
    // dev->parameter->table_write(dev, 0x1101, 0xf, 32, db_interval_base_wire_vel); // 2基值送丝速度
    // dev->parameter->table_write(dev, 0x1102, 0x6, 32, db_interval_peak_wire_vel); // 3峰值送丝速度
    // dev->parameter->table_write(dev, 0x1102, 0xf, 32, db_interval_base_wire_vel); // 3基值送丝速度
    // dev->parameter->table_write(dev, 0x1103, 0x6, 32, db_interval_peak_wire_vel); // 4峰值送丝速度
    // dev->parameter->table_write(dev, 0x1103, 0xf, 32, db_interval_base_wire_vel); // 4基值送丝速度
    // dev->parameter->table_write(dev, 0x1104, 0x6, 32, db_interval_peak_wire_vel); // 5峰值送丝速度
    // dev->parameter->table_write(dev, 0x1104, 0xf, 32, db_interval_base_wire_vel); // 5基值送丝速度

    // dev->parameter->table_write(dev, 0x1100, 0x10, 32, db_interval_angle_position_0); // 1角摆位置
    // dev->parameter->table_write(dev, 0x1101, 0x10, 32, db_interval_angle_position_1); // 2角摆位置
    // dev->parameter->table_write(dev, 0x1102, 0x10, 32, db_interval_angle_position_2); // 3角摆位置
    // dev->parameter->table_write(dev, 0x1103, 0x10, 32, db_interval_angle_position_3); // 4角摆位置
    // dev->parameter->table_write(dev, 0x1104, 0x10, 32, db_interval_angle_position_4); // 5角摆位置

    // dev->parameter->table_write(dev, 0x1100, 0x11, 32, 0); // 1旋转方向
    // dev->parameter->table_write(dev, 0x1101, 0x11, 32, 0); // 2旋转方向
    // dev->parameter->table_write(dev, 0x1102, 0x11, 32, 0); // 3旋转方向
    // dev->parameter->table_write(dev, 0x1103, 0x11, 32, 0); // 4旋转方向
    // dev->parameter->table_write(dev, 0x1104, 0x11, 32, 1); // 5旋转方向

    // dev->parameter->table_write(dev, 0x1100, 0x12, 32, 0); // 1跟踪方式
    // dev->parameter->table_write(dev, 0x1101, 0x12, 32, 0); // 2跟踪方式
    // dev->parameter->table_write(dev, 0x1102, 0x12, 32, 0); // 3跟踪方式
    // dev->parameter->table_write(dev, 0x1103, 0x12, 32, 0); // 4跟踪方式
    // dev->parameter->table_write(dev, 0x1104, 0x12, 32, 0); // 5跟踪方式

    // dev->parameter->table_write(dev, 0x1100, 0x4, 32, db_interval_peak_track_vol); // 1峰值跟踪电压
    // dev->parameter->table_write(dev, 0x1100, 0x5, 32, db_interval_base_track_vol); // 1基值跟踪电压
    // dev->parameter->table_write(dev, 0x1101, 0x4, 32, db_interval_peak_track_vol); // 2峰值跟踪电压
    // dev->parameter->table_write(dev, 0x1101, 0x5, 32, db_interval_base_track_vol); // 2基值跟踪电压
    // dev->parameter->table_write(dev, 0x1102, 0x4, 32, db_interval_peak_track_vol); // 3峰值跟踪电压
    // dev->parameter->table_write(dev, 0x1102, 0x5, 32, db_interval_base_track_vol); // 3基值跟踪电压
    // dev->parameter->table_write(dev, 0x1103, 0x4, 32, db_interval_peak_track_vol); // 4峰值跟踪电压
    // dev->parameter->table_write(dev, 0x1103, 0x5, 32, db_interval_base_track_vol); // 4基值跟踪电压
    // dev->parameter->table_write(dev, 0x1104, 0x4, 32, db_interval_peak_track_vol); // 5峰值跟踪电压
    // dev->parameter->table_write(dev, 0x1104, 0x5, 32, db_interval_base_track_vol); // 5基值跟踪电压
    // for (int i = 0; i < AXIS; i++)
    // {
    //     dev->parameter->table_write(dev, 0x4000 + i, 0x5, 32, 0); // 目标速度 0
    //     dev->parameter->table_write(dev, 0x4000 + i, 0x6, 16, 6); // 控制字 6
    //     dev->parameter->table_write(dev, 0x4000 + i, 0x7, 8, 9);  // 模式 9
    //     if (i == 2)
    //     {
    //         dev->parameter->table_write(dev, 0x4000 + i, 0x8, 32, 50000000); // 加速度100000
    //         dev->parameter->table_write(dev, 0x4000 + i, 0x9, 32, 50000000); // 减速度100000
    //         dev->parameter->table_write(dev, 0x4000 + i, 0xa, 8, -1);
    //     }
    //     else if (i == 3)
    //     {
    //         dev->parameter->table_write(dev, 0x4000 + i, 0x8, 32, 10000000); // 加速度100000
    //         dev->parameter->table_write(dev, 0x4000 + i, 0x9, 32, 10000000); // 减速度100000
    //         dev->parameter->table_write(dev, 0x4000 + i, 0xa, 8, 1);
    //     }
    //     else
    //     {
    //         dev->parameter->table_write(dev, 0x4000 + i, 0x8, 32, 1000000); // 加速度100000
    //         dev->parameter->table_write(dev, 0x4000 + i, 0x9, 32, 1000000); // 减速度100000
    //         dev->parameter->table_write(dev, 0x4000 + i, 0xa, 8, -1);
    //     }
    // }
}
