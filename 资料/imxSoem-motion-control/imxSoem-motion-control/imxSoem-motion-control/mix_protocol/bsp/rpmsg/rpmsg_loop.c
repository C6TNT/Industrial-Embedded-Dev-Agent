#include "rpmsg_loop.h"
#include "fsl_gpio.h"
TaskHandle_t ethercat_loop_task_handle = NULL;
TaskHandle_t ecatcheck_handle = NULL;
TaskHandle_t can_loop_handle = NULL;
TaskHandle_t rpmsg_loop_handle = NULL;
TaskHandle_t rpmsg_loop_2_handle = NULL;
TaskHandle_t position_control_handle = NULL;
TaskHandle_t kinematic_task_handle = NULL;

static struct rpmsg_lite_instance *volatile my_rpmsg = NULL;
static struct rpmsg_lite_endpoint *volatile my_ept = NULL;
static volatile rpmsg_queue_handle my_queue = NULL;

static struct rpmsg_lite_instance *volatile my_rpmsg_2 = NULL;
static struct rpmsg_lite_endpoint *volatile my_ept_2 = NULL;
static volatile rpmsg_queue_handle my_queue_2 = NULL;

#if QUICK_COMMUNICATION_PROTOCOL
char QuickReceiveData[512] = {0};
char QuickSendData[512] = {0};
uint8_t command = 0;
#endif

void kinematic_task(void *param)
{
    weld_opt_t *dev = (weld_opt_t *)param;
    while (1)
    {

        for (int i = 0; i < JOINT_MAX_COUNT; i++)
        {
            dev->control->single_motion(&dev->joint[i].attribute);
        }
        /*联动关节坐标转换循环*/
        dev->kinematic->loop(dev);
        /*联动关节运动循环*/
        dev->kinematic->moveL->loop(dev);
        // if (dev->kinematic->moveL->state == moveL_run ||
        //     dev->kinematic->moveL->state == moveL_stop ||
        //     dev->joint[0].attribute.state == axis_run ||
        //     dev->joint[1].attribute.state == axis_run ||
        //     dev->joint[2].attribute.state == axis_run ||
        //     dev->joint[3].attribute.state == axis_run ||
        //     dev->joint[4].attribute.state == axis_run ||
        //     dev->joint[5].attribute.state == axis_run)
        // {
        //     PRINTF("dec:%d,%d,%d,%d,%d,%d\r\n",
        //            dev->joint[0].attribute.target_position,
        //            dev->joint[1].attribute.target_position,
        //            dev->joint[2].attribute.target_position,
        //            dev->joint[3].attribute.target_position,
        //            dev->joint[4].attribute.target_position,
        //            dev->joint[5].attribute.target_position);
        // }
        // if (dev->parameter->table->read_system_state != STATE_IDLE_WELD)
        // {
        //     PRINTF("welding %d p %lld \r\n", dev->parameter->table->read_system_state, dev->rot->attribute.actual_position_64);
        // }
        // if (dev->joint[0].attribute.state == axis_run ||
        //     dev->joint[1].attribute.state == axis_run ||
        //     dev->joint[2].attribute.state == axis_run ||
        //     dev->joint[3].attribute.state == axis_run ||
        //     dev->joint[4].attribute.state == axis_run ||
        //     dev->joint[5].attribute.state == axis_run)
        // {
        //     PRINTF("dec 1: %d 2: %d 3: %d 4: %d 5: %d 6: %d np: %d nv: %d\r\n", dev->joint[0].attribute.target_position,
        //            dev->joint[1].attribute.target_position,
        //            dev->joint[2].attribute.target_position,
        //            dev->joint[3].attribute.target_position,
        //            dev->joint[4].attribute.target_position,
        //            dev->joint[5].attribute.target_position,
        //            dev->joint[5].attribute.now_position,
        //            dev->joint[5].attribute.now_vel);
        // }
        // dev->kinematic->loop(dev);

        /* Yield periodically so CAN/RPMsg tasks are not starved by this tight loop. */
        osDelay(1);
    }
}

void rpmsg_loop_2(void *param)
{
    weld_opt_t *dev = (weld_opt_t *)param;
    volatile uint32_t remote_addr;
    void *rx_buf;
    uint32_t len;
    int32_t result;
    void *tx_buf;
    uint32_t size;
    int32_t ns_ret;

    /* Print the initial banner */
    osDelay(100);
    PRINTF("\r\nRPMSG String Echo FreeRTOS RTOS API Demo rpmsg 2...\r\n");

#ifdef MCMGR_USED
    uint32_t startupData;

    /* Get the startup data */
    (void)MCMGR_GetStartupData(kMCMGR_Core1, &startupData);

    my_rpmsg = rpmsg_lite_remote_init((void *)startupData, RPMSG_LITE_LINK_ID, RL_NO_FLAGS);

    /* Signal the other core we are ready */
    (void)MCMGR_SignalReady(kMCMGR_Core1);
#else
    my_rpmsg_2 = rpmsg_lite_remote_init((void *)RPMSG_LITE_SHMEM_BASE_2, RPMSG_LITE_LINK_ID_2, RL_NO_FLAGS);
#endif /* MCMGR_USED */

    rpmsg_lite_wait_for_link_up(my_rpmsg_2, RL_BLOCK);

    my_queue_2 = rpmsg_queue_create(my_rpmsg_2);
    my_ept_2 = rpmsg_lite_create_ept(my_rpmsg_2, LOCAL_EPT_ADDR_2, rpmsg_queue_rx_cb, my_queue_2);
    ns_ret = rpmsg_ns_announce(my_rpmsg_2, my_ept_2, RPMSG_LITE_NS_ANNOUNCE_STRING_2, RL_NS_CREATE);
    tx_buf = rpmsg_lite_alloc_tx_buffer(my_rpmsg_2, &size, 0);
    osDelay(100);
    PRINTF("\r\nrpmsg2 init: inst=%p queue=%p ept=%p ns_ret=%d tx_buf=%p size=%u\r\n",
           my_rpmsg_2,
           my_queue_2,
           my_ept_2,
           ns_ret,
           tx_buf,
           size);
    PRINTF("\r\nNameservice sent, ready for incoming messages rpmsg 42...\r\n");

    for (;;)
    {
        result = rpmsg_queue_recv_nocopy(my_rpmsg_2, my_queue_2, (uint32_t *)&remote_addr, (char **)&rx_buf, &len, RL_BLOCK);
        char rec_cmd = *((char *)rx_buf);
        PRINTF("the second loop recv success %d\r\n", rec_cmd);
        result = rpmsg_queue_nocopy_free(my_rpmsg_2, rx_buf);
        // osDelay(500);
        // uint8_t gpio5_6_level;                        // 定义变量存储电平值：0=低电平，1=高电平
        // gpio5_6_level = GPIO_ReadPinInput(GPIO5, 24); // 专用输入读取函数
        // if (gpio5_6_level == 1)
        // {
        //     PRINTF("1111111111111111\r\n");
        // }
        // else
        // {
        //     PRINTF("0000000000000000\r\n");
        // }
    }
}

void rpmsg_loop(void *param)
{
    weld_opt_t *dev = (weld_opt_t *)param;
    volatile uint32_t remote_addr;
    void *rx_buf;
    uint32_t len;
    int32_t result;
    void *tx_buf;
    uint32_t size;
    int32_t ns_ret;

    /* Print the initial banner */
    PRINTF("\r\nRPMSG String Echo FreeRTOS RTOS API Demo...\r\n");

#ifdef MCMGR_USED
    uint32_t startupData;

    /* Get the startup data */
    (void)MCMGR_GetStartupData(kMCMGR_Core1, &startupData);

    my_rpmsg = rpmsg_lite_remote_init((void *)startupData, RPMSG_LITE_LINK_ID, RL_NO_FLAGS);

    /* Signal the other core we are ready */
    (void)MCMGR_SignalReady(kMCMGR_Core1);
#else
    my_rpmsg = rpmsg_lite_remote_init((void *)RPMSG_LITE_SHMEM_BASE, RPMSG_LITE_LINK_ID, RL_NO_FLAGS);
#endif /* MCMGR_USED */

    rpmsg_lite_wait_for_link_up(my_rpmsg, RL_BLOCK);

    my_queue = rpmsg_queue_create(my_rpmsg);
    my_ept = rpmsg_lite_create_ept(my_rpmsg, LOCAL_EPT_ADDR, rpmsg_queue_rx_cb, my_queue);
    ns_ret = rpmsg_ns_announce(my_rpmsg, my_ept, RPMSG_LITE_NS_ANNOUNCE_STRING, RL_NS_CREATE);
    tx_buf = rpmsg_lite_alloc_tx_buffer(my_rpmsg, &size, 0);
    PRINTF("\r\nrpmsg init: inst=%p queue=%p ept=%p ns_ret=%d tx_buf=%p size=%u\r\n",
           my_rpmsg,
           my_queue,
           my_ept,
           ns_ret,
           tx_buf,
           size);
    PRINTF("\r\nNameservice sent, ready for incoming messages...\r\n");

    static union rpmsg_single_frame recv_data;
    static union rpmsg_single_frame send_data;
    rpmsg_state_t state = RPMSG_INIT;
    for (;;)
    {

        result = rpmsg_queue_recv_nocopy(my_rpmsg, my_queue, (uint32_t *)&remote_addr, (char **)&rx_buf, &len, RL_BLOCK);
#if COMMON_COMMUNICATION_SPECIAL
        char rec_cmd = *((char *)rx_buf);

        if (len == 216)
        {
            memcpy(recv_data.inf, rx_buf, len);
            if (rec_cmd == 0x06 && recv_data.frame.baseIndex == 0x500 && recv_data.frame.offectIndex == 0 && state == RPMSG_INIT)
            {
                PRINTF("RPMSG init command received: type=%d base=0x%x offect=0x%x len=%d\r\n",
                       recv_data.frame.type,
                       recv_data.frame.baseIndex,
                       recv_data.frame.offectIndex,
                       recv_data.frame.len);
#if USE_CAN == 0
                if (ethercat_loop_task_handle == NULL &&
                    xTaskCreate(ethercat_loop_task, "APP_TASK", ETHERCAT_LOOP_TASK_STACK_SIZE, (void *)dev, tskIDLE_PRIORITY + 3, &ethercat_loop_task_handle) != pdPASS)
                {
                    PRINTF("\r\nFailed to create application task\r\n");
                    for (;;)
                        ;
                }
                if (ecatcheck_handle == NULL &&
                    xTaskCreate(ecatcheck, "ETHERCAT_CHECK_TASK", ECATCHECK_TASK_STACK_SIZE, (void *)dev, tskIDLE_PRIORITY + 2, &ecatcheck_handle) != pdPASS)
                {
                    PRINTF("\r\nFailed to create application task\r\n");
                    for (;;)
                        ;
                }
#else
                if (can_loop_handle == NULL &&
                    xTaskCreate(can_loop, "CAN_LOOP_TASK", CAN_LOOP_TASK_STACK_SIZE, (void *)dev, tskIDLE_PRIORITY + 2, &can_loop_handle) != pdPASS)
                {
                    PRINTF("\r\nFailed to create application task\r\n");
                    for (;;)
                        ;
                }
#endif
                if (kinematic_task_handle == NULL &&
                    xTaskCreate(kinematic_task, "KINEMATIC_TASK", KINEMATIC_TASK_STATCK_SIZE, (void *)dev, tskIDLE_PRIORITY + 1, &kinematic_task_handle) != pdPASS)
                {
                    PRINTF("\r\nFailed to create application task\r\n");
                    for (;;)
                        ;
                }

                osDelay(2000);
                state = RPMSG_RUN;
                PRINTF("RPMSG init command finished, state=%d can_loop=%p kinematic=%p\r\n",
                       state,
                       can_loop_handle,
                       kinematic_task_handle);
                memcpy(tx_buf, recv_data.inf, sizeof(recv_data.inf));
                /* Echo back received message with nocopy send */
                result = rpmsg_lite_send_nocopy(my_rpmsg, my_ept, remote_addr, tx_buf, sizeof(recv_data.inf));
            }
            else
            {
                switch (rec_cmd)
                {
                case 0x05:
                    send_data.frame.type = recv_data.frame.type;
                    send_data.frame.baseIndex = recv_data.frame.baseIndex;
                    send_data.frame.offectIndex = recv_data.frame.offectIndex;
                    send_data.frame.len = recv_data.frame.len;
                    send_data.frame.returnValue = dev->parameter->table_read(dev,
                                                                             recv_data.frame.baseIndex,
                                                                             recv_data.frame.offectIndex,
                                                                             0,
                                                                             send_data.frame.data);
                    memcpy(tx_buf, send_data.inf, sizeof(send_data.inf));
                    /* Echo back received message with nocopy send */
                    result = rpmsg_lite_send_nocopy(my_rpmsg, my_ept, remote_addr, tx_buf, sizeof(send_data.inf));
                    break;
                case 0x06:
                    send_data.frame.type = recv_data.frame.type;
                    send_data.frame.baseIndex = recv_data.frame.baseIndex;
                    send_data.frame.offectIndex = recv_data.frame.offectIndex;
                    send_data.frame.len = recv_data.frame.len;
                    if (xSemaphoreTake(dev->xMutex, portMAX_DELAY) == pdTRUE)
                    {
                        send_data.frame.returnValue = dev->parameter->table_write(dev,
                                                                                  recv_data.frame.baseIndex,
                                                                                  recv_data.frame.offectIndex,
                                                                                  recv_data.frame.len,
                                                                                  recv_data.frame.data);
                        /* Copy string to RPMsg tx buffer */

                        memcpy(tx_buf, send_data.inf, sizeof(send_data.inf));
                        /* Echo back received message with nocopy send */
                        result = rpmsg_lite_send_nocopy(my_rpmsg, my_ept, remote_addr, tx_buf, sizeof(send_data.inf));

                        xSemaphoreGive(dev->xMutex);
                    }
                    break;
                }
            }
        }
#endif
        result = rpmsg_queue_nocopy_free(my_rpmsg, rx_buf);
    }
}
