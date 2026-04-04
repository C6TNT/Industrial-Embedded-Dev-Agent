#ifndef RPMSG_LOOP_H_
#define RPMSG_LOOP_H_
#include "pin_mux.h"
#include "rpmsg_lite.h"
#include "rpmsg_queue.h"
#include "rpmsg_ns.h"
#include "rsc_table.h"
#include "cmsis_os2.h"
#include "ethercat_loop.h"
#include "can_loop.h"
#define RPMSG_LITE_SHMEM_BASE (VDEV0_VRING_BASE)
#define RPMSG_LITE_LINK_ID (RL_PLATFORM_IMX8MP_M7_USER_LINK_ID)
#define RPMSG_LITE_NS_ANNOUNCE_STRING "rpmsg-virtual-tty-channel-1"

#define ETHERCAT_LOOP_TASK_STACK_SIZE (60 * 1024U)
#define ECATCHECK_TASK_STACK_SIZE (1024U)
#define CAN_LOOP_TASK_STACK_SIZE (4096U)
#define RPMSG_TASK_2_STACK_SIZE (1 * 1024U)
#define RPMSG_TASK_STACK_SIZE (60 * 1024U)
#define POSITION_CONTROL_STATCK_SIZE (1024U)
#define KINEMATIC_TASK_STATCK_SIZE (1024U)

#ifndef LOCAL_EPT_ADDR
#define LOCAL_EPT_ADDR (30)
#endif

#define RPMSG_LITE_SHMEM_BASE_2 (VDEV1_VRING_BASE)
#define RPMSG_LITE_LINK_ID_2 (RL_PLATFORM_IMX8MP_M7_USER_LINK_ID_2)
#define RPMSG_LITE_NS_ANNOUNCE_STRING_2 "rpmsg-virtual-tty-channel-2"
#define LOCAL_EPT_ADDR_2 (32)

typedef enum
{
    RPMSG_INIT,
    RPMSG_RUN,
} rpmsg_state_t;
typedef enum
{
    rpmsg_init,
    rpmsg_op,
    rpmsg_error,
} rpmsg_task_state_t;
// #pragma pack(push, 1)
/*------------------上位机通讯---------------------*/

typedef struct
{
    uint8_t type;
    uint16_t baseIndex;
    uint16_t offectIndex;
    uint8_t len;
    double data[25];
    int32_t returnValue;
} rpmsg_single_frame_t;

union rpmsg_single_frame
{
    char inf[sizeof(rpmsg_single_frame_t)];
    rpmsg_single_frame_t frame;
};

typedef struct
{
    uint8_t type;
    uint16_t baseIndex;
    uint16_t offectIndex;
    uint8_t len;
    int32_t data[RPMSG_PROTOCOL_MAX_LEN];
} rpmsg_protocol_frame_t;

union rpmsg_protocol_frame
{
    char inf[sizeof(rpmsg_protocol_frame_t)];
    rpmsg_protocol_frame_t frame;
};
// #pragma pack(pop)
extern TaskHandle_t ethercat_loop_task_handle;
extern TaskHandle_t ecatcheck_handle;
extern TaskHandle_t can_loop_handle;
extern TaskHandle_t rpmsg_loop_handle;
extern TaskHandle_t rpmsg_loop_2_handle;
extern TaskHandle_t position_control_handle;
extern TaskHandle_t kinematic_task_handle;
void rpmsg_loop(void *param);
void rpmsg_loop_2(void *param);
void kinematic_task(void *param);
#endif /* RPMSG_CONFIG_H_ */
