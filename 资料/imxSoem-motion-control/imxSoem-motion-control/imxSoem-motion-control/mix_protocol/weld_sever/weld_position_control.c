#include "weld_sever/weld_position_control.h"
#include "libtpr20pro/libmanager.h"

static void axis_position_init(joint_axis_t *attribute)
{
    attribute->target_position = attribute->actual_position;
    attribute->theory_position = attribute->actual_position;
    attribute->segment_tail = attribute->segment_head = 0;
    attribute->segment_next_head = 1;
}

static int32_t segment_block_read(joint_axis_t *attribute)
{
    if (attribute->segment_tail != attribute->segment_head)
    {
        attribute->target_position = attribute->segment[attribute->segment_tail].target_position;
        attribute->segment_tail = (attribute->segment_tail + 1) % MAX_SEGMENT_LEN;
    }
    return attribute->target_position;
}

static void segment_block_write(joint_axis_t *attribute, int32_t now_p)
{
    attribute->segment[attribute->segment_head].target_position = now_p;
    attribute->segment_head = (attribute->segment_head + 1) % MAX_SEGMENT_LEN;
    attribute->segment_next_head = (attribute->segment_head + 1) % MAX_SEGMENT_LEN;
}

static void single_motion(joint_axis_t *attribute)
{
    // /*状态字故障检测*/
    // if (attribute->status_code & bit(3))
    // {
    //     attribute->control_mode = 6;
    // }
    if (attribute->mode_of_operation == 8)
    {
        switch (attribute->state)
        {
        case axis_idle:
            break;
        case axis_run:
        case axis_stop:
            /*队满返回*/
            if (attribute->segment_next_head == attribute->segment_tail)
            {
                return;
            }
            if (attribute->control_mode != 15)
            {
                attribute->state = axis_idle;
                return;
            }
            attribute->single_result = sinefittingPlan(attribute->step,
                                                       300.0,
                                                       attribute->init_vel,
                                                       (float)cycleTime / 1000.0 / 1000.0 / 1000.0,
                                                       attribute->acc_time,
                                                       attribute->dec_time,
                                                       attribute->f_target_vel,
                                                       attribute->target_step_position,
                                                       &attribute->now_position,
                                                       &attribute->now_vel);
            if (attribute->single_result == 0)
            {
                attribute->step += 1;
                position_control.segment_write(attribute, attribute->start_position + attribute->now_position);
                // attribute->target_position = attribute->start_position + attribute->now_position;
                attribute->theory_position = attribute->start_position + attribute->now_position;
            }
            else
            {
                attribute->step = 0;
                attribute->state = axis_idle;
            }
            break;
        }
    }
}

static void single_motion_set(joint_axis_t *attribute, double target_step_position, double target_vel)
{
    attribute->step = 0;
    attribute->start_position = attribute->theory_position;
    attribute->init_vel = attribute->now_vel;
    attribute->target_step_position = target_step_position * (attribute->f_transper * TRANSPER_1000);
    attribute->f_target_vel = target_vel * (attribute->f_transper * TRANSPER_1000);
    attribute->state = axis_run;
}

weld_joint_axis_t weld_joint[AXIS_MAX_COUNT] = {
    {.attribute = {0}}, // 第1个轴的attribute初始化
    {.attribute = {0}}, // 第2个轴的attribute初始化
    {.attribute = {0}}, // 第3个轴的attribute初始化
    {.attribute = {0}}, // 第4个轴的attribute初始化
    {.attribute = {0}}, // 第5个轴的attribute初始化
    {.attribute = {0}}  // 第6个轴的attribute初始化
};

weld_position_control_t position_control = {
    .segment_write = segment_block_write,
    .segment_read = segment_block_read,
    .single_motion = single_motion,
    .axis_init = axis_position_init,
    .single_motion_set = single_motion_set,
};
