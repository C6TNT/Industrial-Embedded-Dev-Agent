#include "ethercat_loop.h"
#include "ethercat.h"
#include "bsp_basic_tim.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ethernet_rtl8211f.h"
#include "rpmsg_loop.h"
#include "libtpr20pro/libmanager.h"
#include "ups.h"
#define EC_TIMEOUTMON 500

static int64_t cycleTi = cycleTime / 1000;
static int64_t delta = 0;
static int64_t intergral = 0;
static BaseType_t xHigherPriorityTaskWoken;
int64_t ticktime = 0;
static bool task_ul_state = 0;
static char IOmap[512] = {0};
static const TickType_t xBlockTime = pdMS_TO_TICKS(50000);

#define WELD_MOTOR_AXIS_PROCESS(axis_name, axis_ptr)                                                                 \
    /*--------------输出------------------*/                                                                         \
    dev->send_buf_to_motor.sendbuf.motorOutWeld[axis_name].controlWord = axis_ptr->attribute.control_mode;           \
    dev->send_buf_to_motor.sendbuf.motorOutWeld[axis_name].modeofOperation = axis_ptr->attribute.mode_of_operation;  \
    if (dev->send_buf_to_motor.sendbuf.motorOutWeld[axis_name].modeofOperation == 9)                                 \
    {                                                                                                                \
        dev->send_buf_to_motor.sendbuf.motorOutWeld[axis_name].acc = axis_ptr->attribute.acc;                        \
        dev->send_buf_to_motor.sendbuf.motorOutWeld[axis_name].dec = axis_ptr->attribute.dec;                        \
        dev->send_buf_to_motor.sendbuf.motorOutWeld[axis_name].targetVel = axis_ptr->attribute.target_vel;           \
    }                                                                                                                \
    else if (dev->send_buf_to_motor.sendbuf.motorOutWeld[axis_name].modeofOperation == 8)                            \
    {                                                                                                                \
        dev->send_buf_to_motor.sendbuf.motorOutWeld[axis_name].targetPosition = axis_ptr->attribute.target_position; \
    }                                                                                                                \
    /*--------------输入------------------*/                                                                         \
    axis_ptr->attribute.actual_position = dev->recv_buf_to_a53.recvbuf.motorInWeld[axis_name].actualPosition;        \
    axis_ptr->attribute.error_code = dev->recv_buf_to_a53.recvbuf.motorInWeld[axis_name].errorCode;                  \
    axis_ptr->attribute.status_code = dev->recv_buf_to_a53.recvbuf.motorInWeld[axis_name].statusWord;

#if USE_SV660N == 1
#define MOTOR_AXIS_PROCESS(joint_name, axis_idx)                                                                                          \
    dev->send_buf_to_motor.sendbuf.motorOut[joint_name].controlWord = dev->joint[axis_idx].attribute.control_mode;                        \
    dev->send_buf_to_motor.sendbuf.motorOut[joint_name].modeofOperation = dev->joint[axis_idx].attribute.mode_of_operation;               \
    dev->joint[axis_idx].attribute.error_code = dev->recv_buf_to_a53.recvbuf.motorIn[joint_name].errorCode;                               \
    dev->joint[axis_idx].attribute.status_code = dev->recv_buf_to_a53.recvbuf.motorIn[joint_name].statusWord;                             \
    dev->joint[axis_idx].attribute.actual_position = dev->recv_buf_to_a53.recvbuf.motorIn[joint_name].actualPosition;                     \
    if (dev->joint[axis_idx].attribute.mode_of_operation == 9)                                                                            \
    {                                                                                                                                     \
        dev->send_buf_to_motor.sendbuf.motorOut[joint_name].targetVel = dev->joint[axis_idx].attribute.target_vel;                        \
        dev->send_buf_to_motor.sendbuf.motorOut[joint_name].acc = dev->joint[axis_idx].attribute.acc;                                     \
        dev->send_buf_to_motor.sendbuf.motorOut[joint_name].dec = dev->joint[axis_idx].attribute.dec;                                     \
    }                                                                                                                                     \
    else if (dev->joint[axis_idx].attribute.mode_of_operation == 8)                                                                       \
    {                                                                                                                                     \
        dev->send_buf_to_motor.sendbuf.motorOut[joint_name].targetPosition = dev->control->segment_read(&dev->joint[axis_idx].attribute); \
    }
#endif

#if USE_GELI == 1
#define MOTOR_AXIS_PROCESS(joint_name, axis_idx)                                                                                          \
    dev->send_buf_to_motor.sendbuf.motorOut[joint_name].controlWord = dev->joint[axis_idx].attribute.control_mode;                        \
    dev->joint[axis_idx].attribute.error_code = dev->recv_buf_to_a53.recvbuf.motorIn[joint_name].errorCode;                               \
    dev->joint[axis_idx].attribute.status_code = dev->recv_buf_to_a53.recvbuf.motorIn[joint_name].statusWord;                             \
    dev->joint[axis_idx].attribute.actual_position = dev->recv_buf_to_a53.recvbuf.motorIn[joint_name].actualPosition;                     \
    if (dev->joint[axis_idx].attribute.mode_of_operation == 8)                                                                            \
    {                                                                                                                                     \
        dev->send_buf_to_motor.sendbuf.motorOut[joint_name].targetPosition = dev->control->segment_read(&dev->joint[axis_idx].attribute); \
    }
#endif

static int csp_setup_1(uint16 slave)
{
    int retval = 0;

    uint8_t u8val = 0x0;
    uint16_t u16val = 0x0;
    uint32_t u32val = 0x0;

    retval += ec_SDOwrite(slave, 0x1c12, 0x00, false, sizeof(u8val), &u8val, EC_TIMEOUTRXM);
    retval += ec_SDOwrite(slave, 0x1c13, 0x00, false, sizeof(u8val), &u8val, EC_TIMEOUTRXM);

    u8val = 0x0;
    retval += ec_SDOwrite(slave, 0x1609, 0x00, false, sizeof(u8val), &u8val, EC_TIMEOUTRXM);
    u32val = 0x700002f0;
    retval += ec_SDOwrite(slave, 0x1609, 0x01, false, sizeof(u32val), &u32val, EC_TIMEOUTRXM);
    u8val = 0x1;
    retval += ec_SDOwrite(slave, 0x1609, 0x00, false, sizeof(u8val), &u8val, EC_TIMEOUTRXM);

    u8val = 0x0;
    retval += ec_SDOwrite(slave, 0x1a09, 0x00, false, sizeof(u8val), &u8val, EC_TIMEOUTRXM);
    u32val = 0x600002f0;
    retval += ec_SDOwrite(slave, 0x1a09, 0x01, false, sizeof(u32val), &u32val, EC_TIMEOUTRXM);
    u8val = 0x1;
    retval += ec_SDOwrite(slave, 0x1a09, 0x00, false, sizeof(u8val), &u8val, EC_TIMEOUTRXM);

    u16val = 0x1609;
    retval += ec_SDOwrite(slave, 0x1c12, 0x01, false, sizeof(u16val), &u16val, EC_TIMEOUTRXM);
    u16val = 0x1a09;
    retval += ec_SDOwrite(slave, 0x1c13, 0x01, false, sizeof(u16val), &u16val, EC_TIMEOUTRXM);

    u8val = 0x01;
    retval += ec_SDOwrite(slave, 0x1c12, 0x00, false, sizeof(u8val), &u8val, EC_TIMEOUTRXM);
    retval += ec_SDOwrite(slave, 0x1c13, 0x00, false, sizeof(u8val), &u8val, EC_TIMEOUTRXM);

    return 1;
}

static int csv_setup(uint16 slave)
{
    int retval = 0;

    uint8_t u8val = 0x0;
    uint16_t u16val = 0x0;
    uint32_t u32val = 0x0;

    retval += ec_SDOwrite(slave, 0x1c12, 0x00, false, sizeof(u8val), &u8val, EC_TIMEOUTRXM);
    retval += ec_SDOwrite(slave, 0x1c13, 0x00, false, sizeof(u8val), &u8val, EC_TIMEOUTRXM);

    u8val = 0x0;
    retval += ec_SDOwrite(slave, 0x1600, 0x00, false, sizeof(u8val), &u8val, EC_TIMEOUTRXM);
    u32val = 0x60400010;
    retval += ec_SDOwrite(slave, 0x1600, 0x01, false, sizeof(u32val), &u32val, EC_TIMEOUTRXM);
    u32val = 0x60FF0020;
    retval += ec_SDOwrite(slave, 0x1600, 0x02, false, sizeof(u32val), &u32val, EC_TIMEOUTRXM);
    u32val = 0x60830020;
    retval += ec_SDOwrite(slave, 0x1600, 0x03, false, sizeof(u32val), &u32val, EC_TIMEOUTRXM);
    u32val = 0x60840020;
    retval += ec_SDOwrite(slave, 0x1600, 0x04, false, sizeof(u32val), &u32val, EC_TIMEOUTRXM);
    u32val = 0x60600008;
    retval += ec_SDOwrite(slave, 0x1600, 0x05, false, sizeof(u32val), &u32val, EC_TIMEOUTRXM);
    u8val = 0x5;
    retval += ec_SDOwrite(slave, 0x1600, 0x00, false, sizeof(u8val), &u8val, EC_TIMEOUTRXM);

    u8val = 0x0;
    retval += ec_SDOwrite(slave, 0x1A00, 0x00, false, sizeof(u8val), &u8val, EC_TIMEOUTRXM);
    u32val = 0x60410010;
    retval += ec_SDOwrite(slave, 0x1A00, 0x01, false, sizeof(u32val), &u32val, EC_TIMEOUTRXM);
    u32val = 0x603F0010;
    retval += ec_SDOwrite(slave, 0x1A00, 0x02, false, sizeof(u32val), &u32val, EC_TIMEOUTRXM);
    u32val = 0x60640020;
    retval += ec_SDOwrite(slave, 0x1A00, 0x03, false, sizeof(u32val), &u32val, EC_TIMEOUTRXM);
    u32val = 0x60610008;
    retval += ec_SDOwrite(slave, 0x1A00, 0x04, false, sizeof(u32val), &u32val, EC_TIMEOUTRXM);
    u8val = 0x4;
    retval += ec_SDOwrite(slave, 0x1A00, 0x00, false, sizeof(u8val), &u8val, EC_TIMEOUTRXM);

    u16val = 0x1600;
    retval += ec_SDOwrite(slave, 0x1c12, 0x01, false, sizeof(u16val), &u16val, EC_TIMEOUTRXM);
    u16val = 0x1A00;
    retval += ec_SDOwrite(slave, 0x1c13, 0x01, false, sizeof(u16val), &u16val, EC_TIMEOUTRXM);

    u8val = 0x01;
    retval += ec_SDOwrite(slave, 0x1c12, 0x00, false, sizeof(u8val), &u8val, EC_TIMEOUTRXM);
    retval += ec_SDOwrite(slave, 0x1c13, 0x00, false, sizeof(u8val), &u8val, EC_TIMEOUTRXM);

    return 1;
}

static int csp_setup(uint16 slave)
{
    int retval = 0;

    uint8_t u8val = 0x0;
    uint16_t u16val = 0x0;
    uint32_t u32val = 0x0;

    retval += ec_SDOwrite(slave, 0x1c12, 0x00, false, sizeof(u8val), &u8val, EC_TIMEOUTRXM);
    retval += ec_SDOwrite(slave, 0x1c13, 0x00, false, sizeof(u8val), &u8val, EC_TIMEOUTRXM);

    // u8val = 0x0;
    // retval += ec_SDOwrite(slave, 0x1600, 0x00, false, sizeof(u8val), &u8val, EC_TIMEOUTRXM);
    // u32val = 0x60400010;
    // retval += ec_SDOwrite(slave, 0x1600, 0x01, false, sizeof(u32val), &u32val, EC_TIMEOUTRXM);
    // u32val = 0x607A0020;
    // retval += ec_SDOwrite(slave, 0x1600, 0x02, false, sizeof(u32val), &u32val, EC_TIMEOUTRXM);
    // u32val = 0x60600008;
    // retval += ec_SDOwrite(slave, 0x1600, 0x03, false, sizeof(u32val), &u32val, EC_TIMEOUTRXM);
    // u8val = 0x3;
    // retval += ec_SDOwrite(slave, 0x1600, 0x00, false, sizeof(u8val), &u8val, EC_TIMEOUTRXM);

    u8val = 0x0;
    retval += ec_SDOwrite(slave, 0x1600, 0x00, false, sizeof(u8val), &u8val, EC_TIMEOUTRXM);
    u32val = 0x60400010;
    retval += ec_SDOwrite(slave, 0x1600, 0x01, false, sizeof(u32val), &u32val, EC_TIMEOUTRXM);
    u32val = 0x607A0020;
    retval += ec_SDOwrite(slave, 0x1600, 0x02, false, sizeof(u32val), &u32val, EC_TIMEOUTRXM);
    u32val = 0x60FF0020;
    retval += ec_SDOwrite(slave, 0x1600, 0x03, false, sizeof(u32val), &u32val, EC_TIMEOUTRXM);
    u32val = 0x60830020;
    retval += ec_SDOwrite(slave, 0x1600, 0x04, false, sizeof(u32val), &u32val, EC_TIMEOUTRXM);
    u32val = 0x60840020;
    retval += ec_SDOwrite(slave, 0x1600, 0x05, false, sizeof(u32val), &u32val, EC_TIMEOUTRXM);
    u32val = 0x60600008;
    retval += ec_SDOwrite(slave, 0x1600, 0x06, false, sizeof(u32val), &u32val, EC_TIMEOUTRXM);
    u8val = 0x6;
    retval += ec_SDOwrite(slave, 0x1600, 0x00, false, sizeof(u8val), &u8val, EC_TIMEOUTRXM);

    u8val = 0x0;
    retval += ec_SDOwrite(slave, 0x1A00, 0x00, false, sizeof(u8val), &u8val, EC_TIMEOUTRXM);
    u32val = 0x60410010;
    retval += ec_SDOwrite(slave, 0x1A00, 0x01, false, sizeof(u32val), &u32val, EC_TIMEOUTRXM);
    u32val = 0x603F0010;
    retval += ec_SDOwrite(slave, 0x1A00, 0x02, false, sizeof(u32val), &u32val, EC_TIMEOUTRXM);
    u32val = 0x60640020;
    retval += ec_SDOwrite(slave, 0x1A00, 0x03, false, sizeof(u32val), &u32val, EC_TIMEOUTRXM);
    u32val = 0x60610008;
    retval += ec_SDOwrite(slave, 0x1A00, 0x04, false, sizeof(u32val), &u32val, EC_TIMEOUTRXM);
    u32val = 0x60FD0020; // 正负限位标志
    retval += ec_SDOwrite(slave, 0x1A00, 0x05, false, sizeof(u32val), &u32val, EC_TIMEOUTRXM);
    u8val = 0x5;
    retval += ec_SDOwrite(slave, 0x1A00, 0x00, false, sizeof(u8val), &u8val, EC_TIMEOUTRXM);

    u16val = 0x1600;
    retval += ec_SDOwrite(slave, 0x1c12, 0x01, false, sizeof(u16val), &u16val, EC_TIMEOUTRXM);
    u16val = 0x1A00;
    retval += ec_SDOwrite(slave, 0x1c13, 0x01, false, sizeof(u16val), &u16val, EC_TIMEOUTRXM);

    u8val = 0x01;
    retval += ec_SDOwrite(slave, 0x1c12, 0x00, false, sizeof(u8val), &u8val, EC_TIMEOUTRXM);
    retval += ec_SDOwrite(slave, 0x1c13, 0x00, false, sizeof(u8val), &u8val, EC_TIMEOUTRXM);

    return 1;
}

static int EcatMix_setup(weld_opt_t *dev, uint8_t mode)
{
    int retval = 0;
    uint8_t u8val = 0x0;
    uint16_t u16val = 0x0;
    int32_t init_p = 0x0;
    u8val = mode;
    uint32_t home_speed = 50000;
    uint32_t home_acc = 1000000;
    uint8_t axis_num = 0;
#if USE_GELI
    axis_num = 2;
#elif USE_SV660N
    axis_num = AXIS;
#endif
    if (mode == 0)
    {
        u8val = 35;
        u16val = 6;
#ifdef YAW
        init_p = dev->yaw->attribute.init_position;

        retval += ec_SDOwrite(YAW + 1 + axis_num, 0x6098, 0x00, false, sizeof(u8val), &u8val, EC_TIMEOUTRXM);
        retval += ec_SDOwrite(YAW + 1 + axis_num, 0x607C, 0x00, false, sizeof(init_p), &init_p, EC_TIMEOUTRXM);
        retval += ec_SDOwrite(YAW + 1 + axis_num, 0x2214, 0x00, false, sizeof(u16val), &u16val, EC_TIMEOUTRXM);
        retval += ec_SDOwrite(YAW + 1 + axis_num, 0x6099, 0x01, false, sizeof(home_speed), &home_speed, EC_TIMEOUTRXM);
        retval += ec_SDOwrite(YAW + 1 + axis_num, 0x609A, 0x00, false, sizeof(home_acc), &home_acc, EC_TIMEOUTRXM);
#endif
#ifdef ANGLE
        init_p = dev->angle->attribute.init_position;
        retval += ec_SDOwrite(ANGLE + 1 + axis_num, 0x6098, 0x00, false, sizeof(u8val), &u8val, EC_TIMEOUTRXM);
        retval += ec_SDOwrite(ANGLE + 1 + axis_num, 0x607C, 0x00, false, sizeof(init_p), &init_p, EC_TIMEOUTRXM);
        retval += ec_SDOwrite(ANGLE + 1 + axis_num, 0x2214, 0x00, false, sizeof(u16val), &u16val, EC_TIMEOUTRXM);
        retval += ec_SDOwrite(ANGLE + 1 + axis_num, 0x6099, 0x01, false, sizeof(home_speed), &home_speed, EC_TIMEOUTRXM);
        retval += ec_SDOwrite(ANGLE + 1 + axis_num, 0x609A, 0x00, false, sizeof(home_acc), &home_acc, EC_TIMEOUTRXM);
#endif
#ifdef ARC
        init_p = dev->arc->attribute.init_position;
        retval += ec_SDOwrite(ARC + 1 + axis_num, 0x6098, 0x00, false, sizeof(u8val), &u8val, EC_TIMEOUTRXM);
        retval += ec_SDOwrite(ARC + 1 + axis_num, 0x607C, 0x00, false, sizeof(init_p), &init_p, EC_TIMEOUTRXM);
        retval += ec_SDOwrite(ARC + 1 + axis_num, 0x2214, 0x00, false, sizeof(u16val), &u16val, EC_TIMEOUTRXM);
        retval += ec_SDOwrite(ARC + 1 + axis_num, 0x6099, 0x01, false, sizeof(home_speed), &home_speed, EC_TIMEOUTRXM);
        retval += ec_SDOwrite(ARC + 1 + axis_num, 0x609A, 0x00, false, sizeof(home_acc), &home_acc, EC_TIMEOUTRXM);
#endif
    }
    else
    {
#ifdef YAW
        u16val = 7;
        u8val = 17;
        retval += ec_SDOwrite(YAW + 1 + axis_num, 0x6098, 0x00, false, sizeof(u8val), &u8val, EC_TIMEOUTRXM);
        retval += ec_SDOwrite(YAW + 1 + axis_num, 0x2214, 0x00, false, sizeof(u16val), &u16val, EC_TIMEOUTRXM);
#endif
#ifdef ANGLE
        u8val = 18;
        retval += ec_SDOwrite(ANGLE + 1 + axis_num, 0x6098, 0x00, false, sizeof(u8val), &u8val, EC_TIMEOUTRXM);
        retval += ec_SDOwrite(ANGLE + 1 + axis_num, 0x2214, 0x00, false, sizeof(u16val), &u16val, EC_TIMEOUTRXM);
#endif
#ifdef ARC
        u8val = 18;
        retval += ec_SDOwrite(ARC + 1 + axis_num, 0x6098, 0x00, false, sizeof(u8val), &u8val, EC_TIMEOUTRXM);
        retval += ec_SDOwrite(ARC + 1 + axis_num, 0x2214, 0x00, false, sizeof(u16val), &u16val, EC_TIMEOUTRXM);
#endif
    }

    return 1;
}

static void simpletest(char *IOmap, weld_opt_t *dev)
{
    int wkc;
    int chk;
    int usedmem;
    /* initialise SOEM, bind socket to ifname */
    if (ec_init("test", g_handle))
    {
        /* find and auto-config slaves */
        if (ec_config_init(FALSE) > 0)
        {
#if USE_SV660N == 1
            for (int i = 1; i <= ec_slavecount; i++)
            {
#if USE_STM32F767 == 1
                if (i == STM32F767_IO)
                {
                    continue;
                }
#endif
#if USE_GELIIO == 1
                if (i == GELI_IO)
                {
                    continue;
                }
#endif
                ec_slave[i].PO2SOconfig = csp_setup;
            }

            ec_configdc();
            for (int i = 1; i <= ec_slavecount; i++)
            {
                ec_dcsync0(i, TRUE, cycleTime, cycleTime / 2);
            }
#endif
#if USE_GELI == 1
            for (int i = 1; i <= ec_slavecount; i++)
            {
#if USE_STM32F767 == 1
                if (i == STM32F767_IO)
                {
                    continue;
                }
#endif
#if USE_GELIIO == 1
                if (i == GELI_IO)
                {
                    continue;
                }
#endif
                if (i == 1)
                {
                    // 跳过格力驱动器
                    continue;
                }
                ec_slave[i].PO2SOconfig = csp_setup;
            }
#endif
            usedmem = ec_config_map(IOmap);

#if USE_GELI == 1
            ec_configdc();

            for (int i = 1; i <= ec_slavecount; i++)
            {
                ec_dcsync0(i, TRUE, cycleTime, cycleTime / 2);
            }
#endif
            // PRINTF("%d slaves found and configured.\n",ec_slavecount);
            // PRINTF("usedmem:%d,sizeIOmap:%d\n",usedmem,sizeof(IOmap));

            /* wait for all slaves to reach SAFE_OP state */
            ec_statecheck(0, EC_STATE_SAFE_OP, EC_TIMEOUTSTATE * 4);

            for (int i = 0; i <= ec_slavecount; i++)
            {
                ec_slave[i].state = EC_STATE_OPERATIONAL;
            }
            /* send one valid process data to make outputs in slaves happy*/
            ec_send_processdata();
            wkc = ec_receive_processdata(EC_TIMEOUTRET);
            /* request OP state for all slaves */
            ec_writestate(0);
            chk = 200;

            /* wait for all slaves to reach OP state */
            do
            {
                ec_send_processdata();
                ec_receive_processdata(EC_TIMEOUTRET);
                // ec_statecheck(0, EC_STATE_OPERATIONAL, 50000);
            } while (chk-- && (ec_slave[0].state != EC_STATE_OPERATIONAL));
            // PRINTF("0state:%d,1state:%d\n",ec_slave[0].state,ec_slave[1].state);
            // EthercatSteperInit_1(1, 8388);
            // EthercatSteperInit(2, 3000); // 2000----2A
            // EthercatSteperInit(3, 3000); // 5500----5.5A
            // EthercatSteperInit(4, 3000); // 2500--2.5A
            EcatMix_setup(dev, 0);
            if (ec_slave[0].state == EC_STATE_OPERATIONAL)
            {
// PRINTF("Operational state reached for all slaves.\r\n");
#if COMMON_COMMUNICATION_PROTOCOL == 1
                dev->inOP = TRUE;
#endif
            }
            else
            {
                PRINTF("Not all slaves reached operational state.\r\n");
                ec_readstate();
                for (int i = 1; i <= ec_slavecount; i++)
                {
                    if (ec_slave[i].state != EC_STATE_OPERATIONAL)
                    {
                        PRINTF("Slave %d State=0x%2.2x StatusCode=0x%4.4x : %s\n",
                               i, ec_slave[i].state, ec_slave[i].ALstatuscode, ec_ALstatuscode2string(ec_slave[i].ALstatuscode));
                    }
                }
                ec_slave[0].state = EC_STATE_INIT;
                /* request INIT state for all slaves */
                ec_writestate(0);
            }
        }
        else
        {
            PRINTF("No slaves found!\n");
        }
    }
    else
    {
        PRINTF("No socket connection\nExecute as root\n");
    }
}

static void simpletest_1(char *IOmap, weld_opt_t *dev)
{
    int wkc;
    int chk;
    int usedmem;
    /* initialise SOEM, bind socket to ifname */
    if (ec_init("test", g_handle))
    {
        /* find and auto-config slaves */
        if (ec_config_init(FALSE) > 0)
        {
            for (int i = 1; i <= ec_slavecount; i++)
            {

                ec_slave[i].PO2SOconfig = csv_setup;
            }

            ec_configdc();
            for (int i = 1; i <= ec_slavecount; i++)
            {
                ec_dcsync0(i, TRUE, cycleTime, cycleTime / 2);
            }
            usedmem = ec_config_map(IOmap);

            // PRINTF("%d slaves found and configured.\n",ec_slavecount);
            // PRINTF("usedmem:%d,sizeIOmap:%d\n",usedmem,sizeof(IOmap));

            /* wait for all slaves to reach SAFE_OP state */
            ec_statecheck(0, EC_STATE_SAFE_OP, EC_TIMEOUTSTATE * 4);

            for (int i = 0; i <= ec_slavecount; i++)
            {
                ec_slave[i].state = EC_STATE_OPERATIONAL;
            }
            /* send one valid process data to make outputs in slaves happy*/
            ec_send_processdata();
            wkc = ec_receive_processdata(EC_TIMEOUTRET);
            /* request OP state for all slaves */
            ec_writestate(0);
            chk = 200;

            /* wait for all slaves to reach OP state */
            do
            {
                ec_send_processdata();
                ec_receive_processdata(EC_TIMEOUTRET);
                // ec_statecheck(0, EC_STATE_OPERATIONAL, 50000);
            } while (chk-- && (ec_slave[0].state != EC_STATE_OPERATIONAL));
            // PRINTF("0state:%d,1state:%d\n",ec_slave[0].state,ec_slave[1].state);
            // EthercatSteperInit_1(1, 8388);
            // EthercatSteperInit(2, 3000); // 2000----2A
            // EthercatSteperInit(3, 3000); // 5500----5.5A
            // EthercatSteperInit(4, 3000); // 2500--2.5A
            if (ec_slave[0].state == EC_STATE_OPERATIONAL)
            {
// PRINTF("Operational state reached for all slaves.\r\n");
#if COMMON_COMMUNICATION_PROTOCOL == 1
                dev->inOP = TRUE;
#endif
            }
            else
            {
                PRINTF("Not all slaves reached operational state.\r\n");
                ec_readstate();
                for (int i = 1; i <= ec_slavecount; i++)
                {
                    if (ec_slave[i].state != EC_STATE_OPERATIONAL)
                    {
                        PRINTF("Slave %d State=0x%2.2x StatusCode=0x%4.4x : %s\n",
                               i, ec_slave[i].state, ec_slave[i].ALstatuscode, ec_ALstatuscode2string(ec_slave[i].ALstatuscode));
                    }
                }
                ec_slave[0].state = EC_STATE_INIT;
                /* request INIT state for all slaves */
                ec_writestate(0);
            }
        }
        else
        {
            PRINTF("No slaves found!\n");
        }
    }
    else
    {
        PRINTF("No socket connection\nExecute as root\n");
    }
}

void ethercat_loop_task(void *param)
{
    weld_opt_t *dev = (weld_opt_t *)param;
    osal_timert rt_ts;
    MX_TIM2_Init(); // init gpt timer
    simpletest(IOmap, dev);
    // simpletest_1(IOmap, dev);
#if QUICK_COMMUNICATION_PROTOCOL == 1
    for (int i = 1; i <= ec_slavecount; i++)
    {
        dev->Tpdo_size += ec_slave[i].Obytes;
    }
    for (int i = 1; i <= ec_slavecount; i++)
    {
        dev->Rpdo_size += ec_slave[i].Ibytes;
    }
    PRINTF("2Tpdo_size %d Rpdo_size %d ec_slavecount %d\r\n", dev->Tpdo_size, dev->Rpdo_size, ec_slavecount);
#endif

    PRINTF("2Tpdo_size %d Rpdo_size %d ec_slavecount %d\r\n", ec_slave[1].Obytes, ec_slave[1].Ibytes, ec_slavecount);
    uint16_t startAddress = 0;

    rt_ts.stop_time = osal_current_time();

    int cur_cycle_cnt = rt_ts.stop_time.sec * (1000000 / cycleTi) + rt_ts.stop_time.usec / cycleTi; // calcualte number of cycles has passed
    int cycle_time = cur_cycle_cnt * cycleTi;
    int dc_remain_time = ec_DCtime / 1000 % cycleTi;
    int stoptime = cycle_time + dc_remain_time + cycleTi;
    rt_ts.stop_time.sec = stoptime / 1000000;
    rt_ts.stop_time.usec = stoptime % 1000000;
#if COMMON_COMMUNICATION_PROTOCOL == 1
    dev->rpmsg_handler_flag = 1;
#endif
    task_ul_state = 1;
    for (;;)
    {
#if COMMON_COMMUNICATION_PROTOCOL == 1
        /*----------for Debug------------*/
        ulTaskNotifyTake(pdTRUE, xBlockTime);
        //------USER  CODE-------//

        if (xSemaphoreTake(dev->xMutex, portMAX_DELAY) == pdTRUE)
        {
            /*焊接模块循环*/
            dev->system->weld_task(dev);
            //------USER  CODE-------//

            //------对外映射 电机参数以及IO-------//
#ifdef YAW
            WELD_MOTOR_AXIS_PROCESS(YAW, dev->yaw);
#endif
#ifdef ROT
            WELD_MOTOR_AXIS_PROCESS(ROT, dev->rot);
#endif
#ifdef WIRE
            WELD_MOTOR_AXIS_PROCESS(WIRE, dev->wire);
#endif
#ifdef ARC
            WELD_MOTOR_AXIS_PROCESS(ARC, dev->arc);
#endif
#ifdef ANGLE
            WELD_MOTOR_AXIS_PROCESS(ANGLE, dev->angle);
#endif
#if USE_STM32F767 == 1
            /*--------------输出------------------*/
            for (int i = 0; i < MAX_AO_767; i++)
            {
                dev->send_buf_to_motor.sendbuf.AO_767[i] = dev->parameter->table->AO_767[i];
            }
            for (int i = 0; i < MAX_DO_767; i++)
            {
                dev->send_buf_to_motor.sendbuf.DO_767[i] = dev->parameter->table->DO_767[i];
            }
            /*--------------输入------------------*/
            for (int i = 0; i < MAX_DI_767; i++)
            {
                dev->parameter->table->DI_767[i] = dev->recv_buf_to_a53.recvbuf.DI_767[i];
            }
            for (int i = 0; i < MAX_AI_767; i++)
            {
                dev->parameter->table->AI_767[i] = dev->recv_buf_to_a53.recvbuf.AI_767[i];
            }
#endif
#if USE_GELIIO == 1
            /*--------------输出------------------*/
            for (int i = 0; i < MAX_DO_GELI; i++)
            {
                dev->send_buf_to_motor.sendbuf.DO_GELI[i] = dev->parameter->table->DO_GELI[i];
            }

            /*--------------输入------------------*/
            for (int i = 0; i < MAX_DI_GELI; i++)
            {
                dev->parameter->table->DI_GELI[i] = dev->recv_buf_to_a53.recvbuf.DI_GELI[i];
            }
#endif
#ifdef JOINT_0
            MOTOR_AXIS_PROCESS(JOINT_0, AXIS_0);
#endif
#ifdef JOINT_1
            MOTOR_AXIS_PROCESS(JOINT_1, AXIS_1);
#endif
#ifdef JOINT_2
            MOTOR_AXIS_PROCESS(JOINT_2, AXIS_2);
#endif
#ifdef JOINT_3
            MOTOR_AXIS_PROCESS(JOINT_3, AXIS_3);
#endif
#ifdef JOINT_4
            MOTOR_AXIS_PROCESS(JOINT_4, AXIS_4);
#endif
#ifdef JOINT_5
            MOTOR_AXIS_PROCESS(JOINT_5, AXIS_5);
#endif
            /*----------for Debug------------*/
            /*--------recv  and   send-------------*/

            /*--------------------------------------*/

            startAddress = 0;
            for (int i = 1; i <= ec_slavecount; i++)
            {
                memcpy(ec_slave[i].outputs, dev->send_buf_to_motor.inf + startAddress, ec_slave[i].Obytes);
                startAddress += ec_slave[i].Obytes;
            }

            startAddress = 0;
            for (int i = 1; i <= ec_slavecount; i++)
            {
                memcpy(dev->recv_buf_to_a53.inf + startAddress, ec_slave[i].inputs, ec_slave[i].Ibytes);
                startAddress += ec_slave[i].Ibytes;
            }

            // 释放互斥锁
            xSemaphoreGive(dev->xMutex);
        }

#endif
        if (xSemaphoreTake(dev->ETHMutex, portMAX_DELAY) == pdTRUE)
        {
            ec_send_processdata();
            dev->parameter->table->ec_slave_heart_beat_wkc = ec_receive_processdata(EC_TIMEOUTRET);
            /*从站彻底消失  销毁线程*/
            if (dev->parameter->table->ec_slave_heart_beat_wkc == -1)
            {
                if (ethercat_loop_task_handle != NULL)
                {
                    vTaskDelete(ethercat_loop_task_handle);
                    ethercat_loop_task_handle = NULL;
                }
                if (ecatcheck_handle != NULL)
                {
                    vTaskDelete(ecatcheck_handle);
                    ecatcheck_handle = NULL;
                }
            }
            xSemaphoreGive(dev->ETHMutex);
        }
    }
}

void GPT2_IRQHandler(void)
{
    GPT_ClearStatusFlags(GPT2, kGPT_OutputCompare1Flag);

#if USE_CAN == 0
    ticktime += cycleTi;
    if (task_ul_state)
    {
        delta = (ec_DCtime - 50000) % cycleTime;
        if (delta > (cycleTime / 2))
        {
            delta = delta - cycleTime;
        }
        if (delta > 0)
        {
            intergral++;
        }
        else if (delta < 0)
        {
            intergral--;
        }

        GPT_SetOutputCompareValue(GPT2, kGPT_OutputCompare_Channel1, (cycleTi - delta / 10000 - intergral / 2000));

        xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(ethercat_loop_task_handle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
#else
    xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(can_loop_handle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
#endif
    SDK_ISR_EXIT_BARRIER;
}

void ecatcheck(void *param)
{
    weld_opt_t *dev = (weld_opt_t *)param;
    int slave = 0;
    while (1)
    {
        osDelay(1000);
        if (dev->inOP) //&& ((wkc < expectedWKC) || ec_group[0].docheckstate))
        {
            if (xSemaphoreTake(dev->ETHMutex, portMAX_DELAY) == pdTRUE)
            {
                ec_readstate();
                for (slave = 1; slave <= ec_slavecount; slave++)
                {
                    if ((ec_slave[slave].group == 0) && (ec_slave[slave].state != EC_STATE_OPERATIONAL))
                    {
                        ec_group[0].docheckstate = TRUE;
                        if (ec_slave[slave].state == (EC_STATE_SAFE_OP + EC_STATE_ERROR))
                        {
                            // rt_printf("ERROR : slave %d is in SAFE_OP + ERROR, attempting ack.\n", slave);
                            ec_slave[slave].state = (EC_STATE_SAFE_OP + EC_STATE_ACK);
                            ec_writestate(slave);
                        }
                        else if (ec_slave[slave].state == EC_STATE_SAFE_OP)
                        {
                            // rt_printf("WARNING : slave %d is in SAFE_OP, change to OPERATIONAL.\n", slave);
                            ec_slave[slave].state = EC_STATE_OPERATIONAL;
                            ec_writestate(slave);
                        }
                        else if (ec_slave[slave].state > EC_STATE_NONE)
                        {
                            if (ec_reconfig_slave(slave, EC_TIMEOUTMON))
                            {
                                ec_slave[slave].islost = FALSE;
                                // rt_printf("MESSAGE : slave %d reconfigured\n",slave);
                            }
                        }
                        else if (!ec_slave[slave].islost)
                        {
                            // re-check state
                            ec_statecheck(slave, EC_STATE_OPERATIONAL, EC_TIMEOUTRET);
                            if (ec_slave[slave].state == EC_STATE_NONE)
                            {
                                ec_slave[slave].islost = TRUE;
                                // rt_printf("ERROR : slave %d lost\n",slave);
                            }
                        }
                    }
                    if (ec_slave[slave].islost)
                    {
                        if (ec_slave[slave].state == EC_STATE_NONE)
                        {
                            if (ec_recover_slave(slave, EC_TIMEOUTMON))
                            {
                                ec_slave[slave].islost = FALSE;
                                // rt_printf("MESSAGE : slave %d recovered\n",slave);
                            }
                        }
                        else
                        {
                            ec_slave[slave].islost = FALSE;
                            // rt_printf("MESSAGE : slave %d found\n",slave);
                        }
                    }
                }
                xSemaphoreGive(dev->ETHMutex);
            }
        }
    }
}

ethercat_control_t ethercat_control = {
    .ec_home_set = EcatMix_setup,
};

/*
sdo_r 0x20
sdo_w 0x21
pdo_r 0x22
pdo_w 0x23
常规r 0x24
常规w 0x25

读/写    基地址     偏移地址        数据长度     数据内容(uint32)  从站号
sdo_r    SDO基地址  SDO偏移地址     32/16/8     SDO数据内容       从1开始
sdo_w    SDO基地址  SDO偏移地址     32/16/8     SDO数据内容       从1开始
pdo_r    PDO基地址  PDO偏移地址     32/16/8     PDO映射内容       从1开始
pdo_w    PDO基地址  PDO偏移地址     32/16/8     PDO映射内容       从1开始
常规r    0x6000     0x0             ×          读取主站状态
常规w    0x6001     0x0             ×
*/