#include "weld_sever/weld_parameter.h"
#include "libtpr20pro/libmanager.h"
#include "rpmsg_loop.h"
#include "ups.h"
static weld_parameter_table_t weld_table = {0};
static protocol_block_t weld_protocol_block = {0};

/*
    旋转轴传动比: dec/r
    所有速度单位: mm/min    -> r     /60/半径/2PI
    所有长度单位: °         -> r     *(PI/180)/2PI

    其余轴传动比: dec/mm
    速度单位: mm/min
    长度单位: mm
*/

#define rot_vel_transfer 1.0 / 60.0 / (M_PI * 2.0)
#define rot_distance_transfer 1.0 * (M_PI / 180.0f) / (M_PI * 2.0)
#define wire_vel_transfer 1.0 / 60.0
#define wire_distance_transfer 1.0
#define yaw_vel_transfer 1.0 / 60.0
#define yaw_distance_transfer 1.0
#define angle_vel_transfer 1.0 / 60.0
#define angle_distance_transfer 1.0
#define arc_vel_transfer 1.0 / 60.0
#define arc_distance_transfer 1.0

static uint8_t table_write_weld(weld_opt_t *dev, uint16_t Index1, uint8_t Index2, uint16_t len, double *data)
{
    if (Index1 >= 0x1100 && Index1 < 0x1120)
    {
        int address = Index1 & 0xff;
        if (address >= MAX_SECTION)
        {
            return 1;
        }
        switch (Index2)
        {
        case 0:
            dev->parameter->table->interval_peak_current[address] = (uint32_t)data[0];
            break;
        case 0x1:
            dev->parameter->table->interval_base_current[address] = (uint32_t)data[0];
            break;
        case 0x2:
            dev->parameter->table->interval_peak_time[address] = (uint32_t)data[0];
            break;
        case 0x3:
            dev->parameter->table->interval_base_time[address] = (uint32_t)data[0];
            break;
        case 0x4:
            dev->parameter->table->interval_peak_track_vol[address] = (uint32_t)data[0];
            break;
        case 0x5:
            dev->parameter->table->interval_base_track_vol[address] = (uint32_t)data[0];
            break;
        case 0x6:
            dev->parameter->table->interval_peak_wire_vel[address] = data[0];
            break;
        case 0x7:
            dev->parameter->table->interval_peak_pluse_current[address] = (uint32_t)data[0];
            break;
        case 0x8:
            dev->parameter->table->interval_yaw_vel[address] = (data[0] * yaw_vel_transfer);
            break;
        case 0x9:
            dev->parameter->table->interval_yaw_distance[address] = (data[0] * yaw_distance_transfer);
            break;
        case 0xa:
            dev->parameter->table->interval_yaw_forward_time[address] = (uint32_t)data[0];
            break;
        case 0xb:
            dev->parameter->table->interval_yaw_backward_time[address] = (uint32_t)data[0];
            break;
        case 0xc:
            dev->parameter->table->interval_peak_rot_vel[address] = (data[0] * rot_vel_transfer / ((dev->parameter->table->tube_diameter < 1.0f) ? 1.0f : dev->parameter->table->tube_diameter) / 2.0);
            break;
        case 0xd:
            dev->parameter->table->interval_base_rot_vel[address] = (data[0] * rot_vel_transfer / ((dev->parameter->table->tube_diameter < 1.0f) ? 1.0f : dev->parameter->table->tube_diameter) / 2.0);
            break;
        case 0xe:
            dev->parameter->table->interval_rot_distance[address] = (data[0] * rot_distance_transfer);
            break;
        case 0xf:
            dev->parameter->table->interval_base_wire_vel[address] = (data[0] * wire_vel_transfer);
            break;
        case 0x10:
            dev->parameter->table->interval_angle_position[address] = (data[0] * angle_distance_transfer);
            break;
        case 0x11:
            dev->parameter->table->interval_rot_direction[address] = (uint32_t)data[0];
            break;
        case 0x12:
            dev->parameter->table->interval_track_mode[address] = (uint8_t)data[0];
            break;
        default:
            return 1;
        }
        return 0;
    }
    else if (Index1 >= 0x4000 && Index1 <= 0x4004)
    {
        /*  0x4000 旋转轴
            0x4001 送丝轴
            0x4002 横摆轴
            0x4003 弧长轴
            0x4004 角度轴
        */
        struct motor_attribute *addr = NULL;
        switch (Index1)
        {
        case 0x4000:
            addr = &dev->rot->attribute;
            break;
        case 0x4001:
            addr = &dev->wire->attribute;
            break;
        case 0x4002:
            addr = &dev->yaw->attribute;
            break;
        case 0x4003:
            addr = &dev->arc->attribute;
            break;
        case 0x4004:
            addr = &dev->angle->attribute;
            break;
        }
        switch (Index2)
        {
        case 0x5:
            addr->f_target_vel = data[0];
            if (addr->mode_of_operation == 3 || addr->mode_of_operation == 9)
            {
                if (SAFE_ABS(data[0]) < 1e-7)
                {
                    addr->move_mode = 0;
                    addr->target_vel = 0;
                    addr->target_position = addr->actual_position;
                    addr->state = move_idle;
                }
                else
                {
                    addr->move_mode = 2;
                    addr->target_vel = data[0] * addr->f_transper * addr->direciton;
                    /* Continuous velocity mode: do not rely on a far-away int32 target position,
                     * because the encoder value can already be near the int32 boundary. */
                    addr->target_position = addr->actual_position;
                    addr->state = move_run;
                }
            }
            break;
        case 0x6:
            if ((uint16_t)data[0] == 15)
            {
                dev->jog->axis_init(addr);
            }
            addr->control_mode = (uint16_t)data[0];
            break;
        case 0x7:
            addr->mode_of_operation = (int8_t)data[0];
            if (addr->mode_of_operation == 8)
            {
                dev->jog->axis_init(addr);
            }
            break;
        case 0x8:
            addr->acc = (uint32_t)data[0];
            break;
        case 0x9:
            addr->dec = (uint32_t)data[0];
            break;
        case 0xa:
            addr->direciton = (int8_t)data[0];
            break;
        case 0xb:
            addr->f_transper = data[0];
            break;
        case 0xc:
            addr->acc_time = data[0];
            break;
        case 0xd:
            addr->dec_time = data[0];
            break;
        case 0xe:
            addr->init_position = data[0];
            break;
        case 0xf:
            addr->f_target_step_position = data[0];
            break;
        case 0x10:
            dev->jog->single_motion_set(addr, data[0], data[1]);
            break;
        }
    }
    else if (Index1 >= 0x4005 && Index1 <= 0x400a)
    {
        uint16_t address = Index1 - 0x4005;
        switch (Index2)
        {
        case 0x5:
            dev->joint[address].attribute.f_target_vel = data[0];
            break;
        case 0x6:
            if ((uint16_t)data[0] == 15)
            {
                dev->control->axis_init(&dev->joint[address].attribute);
            }
            dev->joint[address].attribute.control_mode = (uint16_t)data[0];
            break;
        case 0x7:
            if ((int8_t)data[0] == 8)
            {
                dev->control->axis_init(&dev->joint[address].attribute);
            }
            dev->joint[address].attribute.mode_of_operation = (int8_t)data[0];
            break;
        case 0x8:
            dev->joint[address].attribute.acc = (uint32_t)data[0];
            break;
        case 0x9:
            dev->joint[address].attribute.dec = (uint32_t)data[0];
            break;
        case 0xa:
            dev->joint[address].attribute.direciton = (int8_t)data[0];
            break;
        case 0xb:
            dev->joint[address].attribute.f_transper = data[0];
            break;
        case 0xc:
            dev->joint[address].attribute.acc_time = data[0];
            break;
        case 0xd:
            dev->joint[address].attribute.dec_time = data[0];
            break;
        case 0xe:
            dev->joint[address].attribute.init_position = data[0];
            break;
        case 0xf:
            dev->joint[address].attribute.f_target_step_position = data[0];
            break;
        case 0x10:
            dev->control->single_motion_set(&dev->joint[address].attribute, data[0], data[1]);
            break;

        default:
            return 1; // 索引无效
        }
    }
    else
    {
        switch (Index1)
        {
        case 0x1000:
            switch (Index2)
            {
            case 0x0:
                dev->parameter->table->rot_interpolation_mode = (int8_t)data[0];
                break;
            case 0x1:
                dev->parameter->table->section_overall = (uint16_t)data[0];
                break;
            case 0x2:
                dev->parameter->table->rot_down_distance = data[0] * rot_distance_transfer;
                break;
            case 0x3:
                /*只读变量*/
                break;
            case 0x4:
                dev->parameter->table->rot_down_vel = data[0] * rot_vel_transfer / ((dev->parameter->table->tube_diameter < 1.0f) ? 1.0f : dev->parameter->table->tube_diameter) / 2.0;
                break;
            case 0x5:
                dev->parameter->table->write_system_state = (uint16_t)data[0];
                break;
            case 0x6:
                dev->parameter->table->switch_sinal = (uint16_t)data[0];
                break;
            case 0x7:
                dev->parameter->table->arc_mode = (uint32_t)data[0];
                break;
            case 0x8:
                dev->parameter->table->arc_forward_speed = data[0] * arc_vel_transfer;
                break;
            case 0x9:
                dev->parameter->table->arc_backward_speed = data[0] * arc_vel_transfer;
                break;
            case 0xa:
                dev->parameter->table->arc_backward_distance = data[0] * arc_distance_transfer;
                break;
            case 0xb:
                dev->parameter->table->track_speed = data[0] * arc_vel_transfer;
                break;
            case 0xc:
                dev->parameter->table->arc_weld_lift_distance = data[0] * arc_distance_transfer;
                break;
            case 0xd:
                dev->parameter->table->wire_pre_start_distance = data[0] * rot_distance_transfer;
                break;
            case 0xe:
                dev->parameter->table->wire_extract_start_distance = data[0] * rot_distance_transfer;
                break;
            case 0xf:
                dev->parameter->table->wire_pre_time = (uint32_t)data[0];
                break;
            case 0x10:
                dev->parameter->table->wire_pre_vel = data[0] * wire_vel_transfer;
                break;
            case 0x11:
                dev->parameter->table->wire_extract_time = (uint32_t)data[0];
                break;
            case 0x12:
                dev->parameter->table->wire_extract_vel = data[0] * wire_vel_transfer;
                break;
            case 0x13:
                dev->parameter->table->current_pre_time = (uint32_t)data[0];
                break;
            case 0x14:
                dev->parameter->table->current_pre = (uint32_t)data[0];
                break;
            case 0x15:
                dev->parameter->table->current_rise_time = (uint32_t)data[0];
                break;
            case 0x16:
                dev->parameter->table->current_arc = (uint32_t)data[0];
                break;
            case 0x17:
                dev->parameter->table->gas_pre_time = (uint32_t)data[0];
                break;
            case 0x18:
                dev->parameter->table->gas_delay_time = (uint32_t)data[0];
                break;
            case 0x19:
                dev->parameter->table->track_delay_time = (uint32_t)data[0];
                break;
            case 0x1a:
                dev->parameter->table->tube_diameter = data[0];
                break;
            default:
                return 1; // 索引无效
            }
            break;
        case 0x1001:
            if (Index2 < MAX_AO_767)
            {
                dev->parameter->table->AO_767[Index2] = (uint16_t)data[0];
            }
            break;
        case 0x1002:
            /*只读变量*/
            break;
        case 0x1003:
            if (Index2 < MAX_DO_767)
            {
                dev->parameter->table->DO_767[Index2] = (uint16_t)data[0];
            }
            break;
        case 0x1004:
            /*只读变量*/
            break;
        case 0x1005:
            if (Index2 < MAX_DO_GELI)
            {
                dev->parameter->table->DO_GELI[Index2] = (uint8_t)data[0];
            }
            break;
        case 0x1006:
            /*只读变量*/
            break;
        case 0x5000:
            switch (Index2)
            {
            case 0x0:
                /*
                    类型:写 目标世界坐标系 四元数表示
                    data[0]:x mm
                    data[1]:y mm
                    data[2]:z mm
                    data[3]:ox
                    data[4]:oy
                    data[5]:oz
                    data[6]:ow
                    data[7]:目标速度 mm/s
                */
                RobotQuationPos endPosition;
                double targetVel = data[7];
                endPosition.x = data[0];
                endPosition.y = data[1];
                endPosition.z = data[2];
                endPosition.ox = data[3];
                endPosition.oy = data[4];
                endPosition.oz = data[5];
                endPosition.ow = data[6];
                dev->kinematic->moveL->moveL_set(dev, endPosition, targetVel);
                break;
            case 0x1:
                /*
                    类型:写 目标世界坐标系 齐次矩阵表示
                    data[0]:nx
                    data[1]:ny
                    data[2]:nz
                    data[3]:ox
                    data[4]:oy
                    data[5]:oz
                    data[6]:ax
                    data[7]:ay
                    data[8]:az
                    data[9]:px
                    data[10]:py
                    data[11]:pz
                    data[12]:目标速度 mm/s
                */
                break;
            case 0x2:
                /*
                    类型:写 目标世界坐标系 RPY表示
                    data[0]:x mm
                    data[1]:y mm
                    data[2]:z mm
                    data[3]:R °
                    data[4]:P °
                    data[5]:Y °
                    data[6]:目标速度 mm/s
                */
                break;
            case 0x3:
                /*
                    类型:写 目标关节坐标系
                    data[0]:joint1 °
                    data[1]:joint2 °
                    data[2]:joint3 °
                    data[3]:joint4 °
                    data[4]:joint5 °
                    data[5]:joint6 °
                    data[6]:目标速度 mm/s
                */
                RobotAngle angle_endPos;
                angle_endPos.Angle1 = data[0];
                angle_endPos.Angle2 = data[1];
                angle_endPos.Angle3 = data[2];
                angle_endPos.Angle4 = data[3];
                angle_endPos.Angle5 = data[4];
                angle_endPos.Angle6 = data[5];
                double movj_targetVel = data[6]; // rad/s
                dev->kinematic->moveJ->moveJ_set(dev, angle_endPos, movj_targetVel);
                break;
            case 0x4:
                /*
                    类型:写 工具坐标系设置
                    data[0]:x方向
                    data[1]:y方向
                    data[2]:z方向
                */
                dev->kinematic->model_param.tool[0] = data[0];
                dev->kinematic->model_param.tool[1] = data[1];
                dev->kinematic->model_param.tool[2] = data[2];
                break;
            case 0x5:
                /*
                类型:写 模型DH参数
                    data[0]:a1
                    data[1]:a2
                    data[2]:a3
                    data[3]:d4
                */
                dev->kinematic->model_param.dhparam[0] = data[0];
                dev->kinematic->model_param.dhparam[1] = data[1];
                dev->kinematic->model_param.dhparam[2] = data[2];
                dev->kinematic->model_param.dhparam[3] = data[3];

                break;
            }
            break;
        case 0x5001:
            break;
        default:
            return 1; // 索引无效
        }
    }
    return 0;
}

static uint8_t table_read_weld(weld_opt_t *dev, uint16_t Index1, uint8_t Index2, uint16_t len, double *data)
{

    if (Index1 >= 0x1100 && Index1 < 0x1120)
    {
        int address = Index1 & 0xff;
        if (address >= MAX_SECTION)
        {
            return 1;
        }
        switch (Index2)
        {
        case 0:
            *data = dev->parameter->table->interval_peak_current[address];
            break;
        case 0x1:
            *data = dev->parameter->table->interval_base_current[address];
            break;
        case 0x2:
            *data = dev->parameter->table->interval_peak_time[address];
            break;
        case 0x3:
            *data = dev->parameter->table->interval_base_time[address];
            break;
        case 0x4:
            *data = dev->parameter->table->interval_peak_track_vol[address];
            break;
        case 0x5:
            *data = dev->parameter->table->interval_base_track_vol[address];
            break;
        case 0x6:
            *data = dev->parameter->table->interval_peak_wire_vel[address];
            break;
        case 0x7:
            *data = dev->parameter->table->interval_peak_pluse_current[address];
            break;
        case 0x8:
            *data = dev->parameter->table->interval_yaw_vel[address];
            break;
        case 0x9:
            *data = dev->parameter->table->interval_yaw_distance[address];
            break;
        case 0xa:
            *data = dev->parameter->table->interval_yaw_forward_time[address];
            break;
        case 0xb:
            *data = dev->parameter->table->interval_yaw_backward_time[address];
            break;
        case 0xc:
            *data = dev->parameter->table->interval_peak_rot_vel[address];
            break;
        case 0xd:
            *data = dev->parameter->table->interval_base_rot_vel[address];
            break;
        case 0xe:
            *data = dev->parameter->table->interval_rot_distance[address];
            break;
        case 0xf:
            *data = dev->parameter->table->interval_base_wire_vel[address];
            break;
        case 0x10:
            *data = dev->parameter->table->interval_angle_position[address];
            break;
        case 0x11:
            *data = dev->parameter->table->interval_rot_direction[address];
            break;
        case 0x12:
            *data = dev->parameter->table->interval_track_mode[address];
            break;
        default:
            return 1;
        }
        return 0;
    }
    else if (Index1 >= 0x4000 && Index1 <= 0x4004)
    {
        struct motor_attribute *addr = NULL;
        switch (Index1)
        {
        case 0x4000:
            addr = &dev->rot->attribute;
            break;
        case 0x4001:
            addr = &dev->wire->attribute;
            break;
        case 0x4002:
            addr = &dev->yaw->attribute;
            break;
        case 0x4003:
            addr = &dev->arc->attribute;
            break;
        case 0x4004:
            addr = &dev->angle->attribute;
            break;
        }
        switch (Index2)
        {
        case 0x0:
            *data = addr->actual_position;
            break;
        case 0x1:
            *data = addr->mode_of_operation;
            break;
        case 0x2:
            *data = addr->status_code;
            break;
        case 0x3:
            *data = addr->error_code;
            break;
        case 0x4:
            *data = addr->state;
            break;
        case 0x5:
            *data = addr->f_target_vel;
            break;
        case 0x6:
            *data = addr->control_mode;
            break;
        case 0x7:
            *data = addr->mode_of_operation;
            break;
        case 0x8:
            *data = addr->acc;
            break;
        case 0x9:
            *data = addr->dec;
            break;
        case 0xa:
            *data = addr->direciton;
            break;
        case 0xb:
            *data = addr->f_transper;
            break;
        case 0xc:
            *data = addr->acc_time;
            break;
        case 0xd:
            *data = addr->dec_time;
            break;
        case 0xe:
            *data = addr->init_position;
            break;
        case 0xf:
            *data = addr->f_target_step_position;
            break;
        case 0x10:
            *data = addr->actual_vel;
            break;
        case 0x11:
            *data = addr->actual_position_64;
            break;
        case 0x12:
            *data = addr->target_vel;
            break;
        case 0x14:
            *data = addr->drive_target_vel;
            break;
        case 0x16:
            *data = addr->drive_mode_of_operation;
            break;
        case 0x18:
            *data = addr->drive_mode_display;
            break;
        case 0x1A:
            *data = addr->drive_control_word;
            break;
        case 0x1C:
            *data = addr->drive_status_word;
            break;
        }
    }
    else if (Index1 >= 0x4005 && Index1 <= 0x400a)
    {
        uint16_t address = Index1 - 0x4005;
        switch (Index2)
        {
        case 0x0:
            *data = dev->joint[address].attribute.actual_position;
            break;
        case 0x1:
            *data = dev->joint[address].attribute.mode_of_operation;
            break;
        case 0x2:
            *data = dev->joint[address].attribute.status_code;
            break;
        case 0x3:
            *data = dev->joint[address].attribute.error_code;
            break;
        case 0x4:
            *data = dev->joint[address].attribute.state;
            break;
        case 0x5:
            *data = dev->joint[address].attribute.f_target_vel;
            break;
        case 0x6:
            *data = dev->joint[address].attribute.control_mode;
            break;
        case 0x7:
            *data = dev->joint[address].attribute.mode_of_operation;
            break;
        case 0x8:
            *data = dev->joint[address].attribute.acc;
            break;
        case 0x9:
            *data = dev->joint[address].attribute.dec;
            break;
        case 0xa:
            *data = dev->joint[address].attribute.direciton;
            break;
        case 0xb:
            *data = dev->joint[address].attribute.f_transper;
            break;
        case 0xc:
            *data = dev->joint[address].attribute.acc_time;
            break;
        case 0xd:
            *data = dev->joint[address].attribute.dec_time;
            break;
        case 0xe:
            *data = dev->joint[address].attribute.init_position;
            break;
        case 0xf:
            *data = dev->joint[address].attribute.f_target_step_position;
            break;
        case 0x10:
            *data = dev->joint[address].attribute.actual_vel;
            break;
        case 0x12:
            *data = dev->joint[address].attribute.target_vel;
            break;
        case 0x14:
            *data = dev->joint[address].attribute.drive_target_vel;
            break;
        case 0x16:
            *data = dev->joint[address].attribute.drive_mode_of_operation;
            break;
        case 0x18:
            *data = dev->joint[address].attribute.drive_mode_display;
            break;
        case 0x1A:
            *data = dev->joint[address].attribute.drive_control_word;
            break;
        case 0x1C:
            *data = dev->joint[address].attribute.drive_status_word;
            break;
        default:
            return 1; // 索引无效
        }
    }
    else
    {
        switch (Index1)
        {
        case 0x501:
            switch (Index2)
            {
            case 0x0:
                data[0] = UPS_power_check();
                break;
            }
            break;
        case 0x1000:
            switch (Index2)
            {
            case 0x0:
                data[0] = dev->parameter->table->rot_interpolation_mode;
                break;
            case 0x1:
                data[0] = dev->parameter->table->section_overall;
                break;
            case 0x2:
                data[0] = dev->parameter->table->rot_down_distance;
                break;
            case 0x3:
                /*只读变量*/
                data[0] = dev->parameter->table->section_rt;
                break;
            case 0x4:
                data[0] = dev->parameter->table->rot_down_vel;
                break;
            case 0x5:
                data[0] = dev->parameter->table->write_system_state;
                break;
            case 0x6:
                data[0] = dev->parameter->table->switch_sinal;
                break;
            default:
                return 1; // 索引无效
            }
            break;
        case 0x1001:
            if (Index2 < MAX_AO_767)
            {
                data[0] = dev->parameter->table->AO_767[Index2];
            }
            break;
        case 0x1002:
            /*只读变量*/
            if (Index2 < MAX_AI_767)
            {
                data[0] = dev->parameter->table->AI_767[Index2];
            }
            break;
        case 0x1003:
            if (Index2 < MAX_DO_767)
            {
                data[0] = dev->parameter->table->DO_767[Index2];
            }
            break;
        case 0x1004:
            /*只读变量*/
            if (Index2 < MAX_DI_767)
            {
                data[0] = dev->parameter->table->DI_767[Index2];
            }
            break;
        case 0x1005:
            if (Index2 < MAX_DO_GELI)
            {
                data[0] = dev->parameter->table->DO_GELI[Index2];
            }
            break;
        case 0x1006:
            if (Index2 < MAX_DI_GELI)
            {
                data[0] = dev->parameter->table->DI_GELI[Index2];
            }
            break;
        case 0x5000:
            switch (Index2)
            {
            case 0x6:
                data[0] = dev->kinematic->moveL->state;
                break;
            case 0x7:
                /*
                    类型:读 当前世界坐标系 四元数表示
                    data[0]:x mm
                    data[1]:y mm
                    data[2]:z mm
                    data[3]:ox
                    data[4]:oy
                    data[5]:oz
                    data[6]:ow
                */
                data[0] = dev->kinematic->robotQuationPos.x;
                data[1] = dev->kinematic->robotQuationPos.y;
                data[2] = dev->kinematic->robotQuationPos.z;
                data[3] = dev->kinematic->robotQuationPos.ox;
                data[4] = dev->kinematic->robotQuationPos.oy;
                data[5] = dev->kinematic->robotQuationPos.oz;
                data[6] = dev->kinematic->robotQuationPos.ow;
                break;
            case 0x8:
                /*
                    类型:读 当前世界坐标系 齐次矩阵表示
                    data[0]:nx
                    data[1]:ny
                    data[2]:nz
                    data[3]:ox
                    data[4]:oy
                    data[5]:oz
                    data[6]:ax
                    data[7]:ay
                    data[8]:az
                    data[9]:ax
                    data[10]:ay
                    data[11]:az
                */
                data[0] = dev->kinematic->robotPositive.nx;
                data[1] = dev->kinematic->robotPositive.ny;
                data[2] = dev->kinematic->robotPositive.nz;
                data[3] = dev->kinematic->robotPositive.ox;
                data[4] = dev->kinematic->robotPositive.oy;
                data[5] = dev->kinematic->robotPositive.oz;
                data[6] = dev->kinematic->robotPositive.ax;
                data[7] = dev->kinematic->robotPositive.ay;
                data[8] = dev->kinematic->robotPositive.az;
                data[9] = dev->kinematic->robotPositive.px;
                data[10] = dev->kinematic->robotPositive.py;
                data[11] = dev->kinematic->robotPositive.pz;
                break;
            case 0x9:
                /*
                    类型:读 当前世界坐标系 RPY表示
                    data[0]:x mm
                    data[1]:y mm
                    data[2]:z mm
                    data[3]:R °
                    data[4]:P °
                    data[5]:Y °
                */
                break;
            case 0xa:
                /*
                    类型:读 当前关节坐标系
                    data[0]:joint1 °
                    data[1]:joint2 °
                    data[2]:joint3 °
                    data[3]:joint4 °
                    data[4]:joint5 °
                    data[5]:joint6 °
                */
                data[0] = dev->kinematic->robotAngle.Angle1;
                data[1] = dev->kinematic->robotAngle.Angle2;
                data[2] = dev->kinematic->robotAngle.Angle3;
                data[3] = dev->kinematic->robotAngle.Angle4;
                data[4] = dev->kinematic->robotAngle.Angle5;
                data[5] = dev->kinematic->robotAngle.Angle6;
                break;
            case 0x4:
                /*
                    类型:读 工具坐标系设置
                    data[0]:x方向
                    data[1]:y方向
                    data[2]:z方向
                */
                data[0] = dev->kinematic->model_param.tool[0];
                data[1] = dev->kinematic->model_param.tool[1];
                data[2] = dev->kinematic->model_param.tool[2];
                break;
            case 0x5:
                /*
                类型:写 模型DH参数
                    data[0]:a1
                    data[1]:a2
                    data[2]:a3
                    data[3]:d4
                */
                data[0] = dev->kinematic->model_param.dhparam[0];
                data[1] = dev->kinematic->model_param.dhparam[1];
                data[2] = dev->kinematic->model_param.dhparam[2];
                data[3] = dev->kinematic->model_param.dhparam[3];
                break;
            }
            break;
        default:
            return 1; // 索引无效
        }
    }

    return 0; // 索引无效
}

static uint8_t table_protocol_config_weld(weld_opt_t *dev, uint16_t Index1, uint8_t Index2, uint16_t len, int32_t map_index)
{
    if (map_index >= RPMSG_PROTOCOL_MAX_LEN)
    {
        return 1;
    }
    weld_protocol_block.base_index[map_index] = Index1;
    weld_protocol_block.offect_index[map_index] = Index2;
    return 0;
}

weld_parameter_t weld_parameter = {
    .table = &weld_table,
    .table_write = table_write_weld,
    .table_read = table_read_weld,
};
