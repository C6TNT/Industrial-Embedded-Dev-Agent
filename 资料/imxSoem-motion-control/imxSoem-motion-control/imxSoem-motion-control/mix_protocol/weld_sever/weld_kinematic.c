#include "weld_sever/weld_kinematic.h"
#include "libtpr20pro/libmanager.h"

void kinematic_loop(weld_opt_t *dev)
{
    motorparam_ mp[JOINT_MAX_COUNT];
    int current_joint_dec[JOINT_MAX_COUNT];
    uint8_t enable_count = 0;
    for (int i = 0; i < JOINT_MAX_COUNT; i++)
    {
        mp[i].acctime = dev->joint[i].attribute.acc_time;
        mp[i].dectime = dev->joint[i].attribute.dec_time;
        mp[i].direction = dev->joint[i].attribute.direciton;
        mp[i].initDec = dev->joint[i].attribute.init_position;
        mp[i].transmission = dev->joint[i].attribute.f_transper;
        current_joint_dec[i] = dev->joint[i].attribute.actual_position;
        if (dev->joint[i].attribute.control_mode == 15)
        {
            enable_count++;
        }
    }
    switch (dev->kinematic->loop_state)
    {
    case kinematic_loop_idle:
        if (enable_count == JOINT_MAX_COUNT)
        {
            dev->kinematic->robotAngle = crobotdecToJoint(current_joint_dec, mp);

            // dev->kinematic->robotPositive = PositiveRobot(&dev->kinematic->robotAngle);
            RobotPosition word_pos;
            word_pos = PositiveRobot_dh(&dev->kinematic->robotAngle, dev->kinematic->model_param.dhparam);
            dev->kinematic->robotPositive = sixToendLinkConverter(word_pos, dev->kinematic->model_param.tool);
            dev->kinematic->robotQuationPos = RotationToQuaternionMatrix(&dev->kinematic->robotPositive);
            dev->kinematic->init(dev);
            dev->kinematic->loop_state = kinematic_loop_enable;
        }
        break;
    case kinematic_loop_enable:
        if (enable_count < JOINT_MAX_COUNT)
        {
            dev->kinematic->loop_state = kinematic_loop_idle;
        }
        dev->kinematic->robotAngle = crobotdecToJoint(current_joint_dec, mp);

        // dev->kinematic->robotPositive = PositiveRobot(&dev->kinematic->robotAngle);
        RobotPosition word_pos;
        word_pos = PositiveRobot_dh(&dev->kinematic->robotAngle, dev->kinematic->model_param.dhparam);
        dev->kinematic->robotPositive = sixToendLinkConverter(word_pos, dev->kinematic->model_param.tool);

        dev->kinematic->robotQuationPos = RotationToQuaternionMatrix(&dev->kinematic->robotPositive);
        break;
    }
}

void moveL_loop(weld_opt_t *dev)
{
    RobotQuationPos nextQuationPosition;
    RobotPosition nextPosition, nextPosition_tcp;
    RobotAngle nextrobotAngle;
    motorparam_ mp[JOINT_MAX_COUNT];
    int calaxisdec[JOINT_MAX_COUNT];
    switch (dev->kinematic->moveL->state)
    {
    case moveL_idle:
        break;
    case moveL_run:
    case moveL_stop:
        for (int i = 0; i < JOINT_MAX_COUNT; i++)
        {
            if (dev->joint[i].attribute.control_mode != 15)
            {
                dev->kinematic->moveL->state = moveL_idle;
                return;
            }
            if (dev->joint[i].attribute.segment_next_head == dev->joint[i].attribute.segment_tail)
            {
                return;
            }
            mp[i].acctime = dev->joint[i].attribute.acc_time;
            mp[i].dectime = dev->joint[i].attribute.dec_time;
            mp[i].direction = dev->joint[i].attribute.direciton;
            mp[i].initDec = dev->joint[i].attribute.init_position;
            mp[i].transmission = dev->joint[i].attribute.f_transper;
        }
        dev->kinematic->moveL->moveL_result = sinefittingPlan_movl(dev->kinematic->moveL->step,
                                                                   300.0,
                                                                   dev->kinematic->moveL->init_vel,
                                                                   (float)cycleTime / 1000.0 / 1000.0 / 1000.0,
                                                                   dev->kinematic->moveL->acc_time,
                                                                   dev->kinematic->moveL->dec_time,
                                                                   dev->kinematic->moveL->f_target_vel,
                                                                   dev->kinematic->moveL->targetDistance,
                                                                   &dev->kinematic->moveL->now_position,
                                                                   &dev->kinematic->moveL->now_vel);
        if (dev->kinematic->moveL->moveL_result == 0)
        {
            dev->kinematic->moveL->step += 1;
            dev->kinematic->moveL->persent = (float)dev->kinematic->moveL->now_position / dev->kinematic->moveL->targetDistance;
            nextQuationPosition = nextQuationPostion(dev->kinematic->moveL->startPosition,
                                                     dev->kinematic->moveL->endPosition,
                                                     dev->kinematic->moveL->persent);
            // nextPosition = QuaternionToRotationMatrix(&nextQuationPosition);
            nextPosition_tcp = QuaternionToRotationMatrix(&nextQuationPosition);
            nextPosition = endLinkToSixConverter(nextPosition_tcp, dev->kinematic->model_param.tool);

            nextrobotAngle = InverseRobot_dh(&nextPosition, &dev->kinematic->moveL->lastrobotangle, dev->kinematic->model_param.dhparam);

            dev->kinematic->moveL->lastrobotangle = nextrobotAngle;
            crobotJointToDec(&nextrobotAngle, mp, calaxisdec);
            for (int i = 0; i < JOINT_MAX_COUNT; i++)
            {
                dev->control->segment_write(&dev->joint[i].attribute, calaxisdec[i]);
                // dev->joint[i].attribute.target_position = calaxisdec[i];
                dev->joint[i].attribute.theory_position = calaxisdec[i];
            }
        }
        else
        {
            dev->kinematic->moveL->step = 0;
            dev->kinematic->moveL->state = moveL_idle;
        }
        break;
    }
}

void kinematic_init(weld_opt_t *dev)
{
    dev->kinematic->moveL->lastrobotangle = dev->kinematic->robotAngle;
}

void moveL_set(weld_opt_t *dev, RobotQuationPos endPosition, double targetVel)
{
    if (dev->kinematic->moveL->state == moveL_stop)
    {
    }
    else
    {
        int current_joint_dec[JOINT_MAX_COUNT];
        motorparam_ mp[JOINT_MAX_COUNT];
        for (int i = 0; i < JOINT_MAX_COUNT; i++)
        {
            mp[i].acctime = dev->joint[i].attribute.acc_time;
            mp[i].dectime = dev->joint[i].attribute.dec_time;
            mp[i].direction = dev->joint[i].attribute.direciton;
            mp[i].initDec = dev->joint[i].attribute.init_position;
            mp[i].transmission = dev->joint[i].attribute.f_transper;
            current_joint_dec[i] = dev->joint[i].attribute.theory_position;
        }
        dev->kinematic->moveL->lastrobotangle = crobotdecToJoint(current_joint_dec, mp);
        RobotPosition theory_initpos_w = PositiveRobot_dh(&dev->kinematic->moveL->lastrobotangle, dev->kinematic->model_param.dhparam);
        RobotPosition theory_initpos = sixToendLinkConverter(theory_initpos_w, dev->kinematic->model_param.tool);

        RobotQuationPos initpos = RotationToQuaternionMatrix(&theory_initpos);
        if (targetVel <= 0.0)
        {

            dev->kinematic->moveL->step = 0;
            dev->kinematic->moveL->init_vel = dev->kinematic->moveL->now_vel;
            dev->kinematic->moveL->f_target_vel = 0;
            dev->kinematic->moveL->startPosition = initpos;
            dev->kinematic->moveL->targetDistance = targetdistanceCal(dev->kinematic->moveL->startPosition, dev->kinematic->moveL->endPosition);
            dev->kinematic->moveL->endPosition = stopPlan_zt(dev->kinematic->moveL->startPosition, dev->kinematic->moveL->endPosition, (double)dev->kinematic->moveL->now_vel, (double)dev->kinematic->moveL->targetDistance);
            dev->kinematic->moveL->targetDistance = targetdistanceCal(dev->kinematic->moveL->startPosition, dev->kinematic->moveL->endPosition);
            dev->kinematic->moveL->state = moveL_stop;
        }
        else
        {
            dev->kinematic->moveL->step = 0;

            dev->kinematic->moveL->startPosition = initpos;
            dev->kinematic->moveL->init_vel = dev->kinematic->moveL->now_vel;
            dev->kinematic->moveL->endPosition = endPosition;
            dev->kinematic->moveL->acc_time = 300.0;
            dev->kinematic->moveL->dec_time = 300.0;
            dev->kinematic->moveL->targetDistance = targetdistanceCal(dev->kinematic->moveL->startPosition, dev->kinematic->moveL->endPosition);
            dev->kinematic->moveL->f_target_vel = targetVel * TRANSPER_1000;
            dev->kinematic->moveL->state = moveL_run;
        }
    }
}
void moveJ_set(weld_opt_t *dev, RobotAngle angle_endPos, double targetVel)
{
    // targetVel接收上位机的速度，单位rad/s
    sixvelplan_ plan_result;
    int current_joint_dec[JOINT_MAX_COUNT];
    motorparam_ mp[JOINT_MAX_COUNT];
    for (int i = 0; i < JOINT_MAX_COUNT; i++)
    {
        mp[i].acctime = dev->joint[i].attribute.acc_time;
        mp[i].dectime = dev->joint[i].attribute.dec_time;
        mp[i].direction = dev->joint[i].attribute.direciton;
        mp[i].initDec = dev->joint[i].attribute.init_position;
        mp[i].transmission = dev->joint[i].attribute.f_transper;

        current_joint_dec[i] = dev->joint[i].attribute.theory_position;
    }
    plan_result = sixRobot_movj(current_joint_dec, mp, angle_endPos, targetVel);

    for (int i = 0; i < JOINT_MAX_COUNT; i++)
    {
        dev->joint[i].attribute.step = 0;
        dev->joint[i].attribute.start_position = dev->joint[i].attribute.theory_position;
        dev->joint[i].attribute.init_vel = dev->joint[i].attribute.now_vel;
        dev->joint[i].attribute.target_step_position = plan_result.targetposition[i];
        dev->joint[i].attribute.f_target_vel = plan_result.velocity[i];
        dev->joint[i].attribute.state = axis_run;
    }
}
kinematic_moveL_t moveL = {
    .loop = moveL_loop,
    .moveL_set = moveL_set,
};

kinematic_moveJ_t moveJ = {
    .moveJ_set = moveJ_set,
};

weld_kinematic_t weld_kinematic = {
    .world_axis = {0},
    .robotAngle = {0},
    .robotPositive = {0},
    .robotQuationPos = {0},
    .loop = kinematic_loop,
    .moveL = &moveL,
    .moveJ = &moveJ,
    .init = kinematic_init,
};
