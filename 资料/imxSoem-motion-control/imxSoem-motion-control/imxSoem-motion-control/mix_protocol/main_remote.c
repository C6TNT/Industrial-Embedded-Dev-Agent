
#include "clock_config.h"
#include "fsl_uart.h"
#include "ethernet_rtl8211f.h"
#include "rpmsg_loop.h"
#include "flexspi_nor.h"
#include "ups.h"
typedef struct
{
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;  // 链接寄存器（异常返回地址）
    uint32_t pc;  // 程序计数器（出错的指令地址！核心定位依据）
    uint32_t psr; // 程序状态寄存器
} HardFault_StackFrame_t;

int main(void)
{

    BOARD_InitMemory();

    /* Board specific RDC settings */
    BOARD_RdcInit();

    BOARD_InitBootPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();

    copyResourceTable();

    CLOCK_SetRootDivider(kCLOCK_RootEnetAxi, 1U, 1U);
    CLOCK_SetRootMux(kCLOCK_RootEnetAxi, kCLOCK_EnetAxiRootmuxSysPll2Div4); /* SYSTEM PLL2 divided by 4: 250Mhz */

    CLOCK_SetRootDivider(kCLOCK_RootEnetTimer, 1U, 1U);
    CLOCK_SetRootMux(kCLOCK_RootEnetTimer, kCLOCK_EnetTimerRootmuxSysPll2Div10); /* SYSTEM PLL2 divided by 10: 100Mhz */

    CLOCK_SetRootDivider(kCLOCK_RootEnetRef, 1U, 1U);
    CLOCK_SetRootMux(kCLOCK_RootEnetRef, kCLOCK_EnetRefRootmuxSysPll2Div8); /* SYSTEM PLL2 divided by 8: 125Mhz */

    CLOCK_EnableClock(kCLOCK_Sim_enet);
    //eth_init_rtl8211();
    UPS_GPIO_Init();
    osKernelInitialize();
    weld_opt_t *dev = weld_task_init();
    if (dev == NULL)
    {
        PRINTF("\r\n no spare to create dev\r\n");
        return 0;
    }
    // 初始化互斥锁

    flash_init();
    if (rpmsg_loop_handle == NULL &&
        xTaskCreate(rpmsg_loop, "rpmsgCommunication", RPMSG_TASK_STACK_SIZE, (void *)dev, tskIDLE_PRIORITY + 2, &rpmsg_loop_handle) != pdPASS)
    {
        PRINTF("\r\nFailed to create rpmsgCommunication task\r\n");
        for (;;)
            ;
    }
    if (rpmsg_loop_2_handle == NULL &&
        xTaskCreate(rpmsg_loop_2, "rpmsgCommunication_2", RPMSG_TASK_2_STACK_SIZE, (void *)dev, tskIDLE_PRIORITY + 2, &rpmsg_loop_2_handle) != pdPASS)
    {
        PRINTF("\r\nFailed to create rpmsgCommunication task\r\n");
        for (;;)
            ;
    }
#if USE_CAN
    if (can_loop_handle == NULL &&
        xTaskCreate(can_loop, "CAN_LOOP_TASK", CAN_LOOP_TASK_STACK_SIZE, (void *)dev, tskIDLE_PRIORITY + 2, &can_loop_handle) != pdPASS)
    {
        PRINTF("\r\nFailed to create CAN loop task\r\n");
        for (;;)
            ;
    }
#endif
    if (kinematic_task_handle == NULL &&
        xTaskCreate(kinematic_task, "KINEMATIC_TASK", KINEMATIC_TASK_STATCK_SIZE, (void *)dev, tskIDLE_PRIORITY + 1, &kinematic_task_handle) != pdPASS)
    {
        PRINTF("\r\nFailed to create kinematic task\r\n");
        for (;;)
            ;
    }
    osKernelStart();

    PRINTF("Failed to start FreeRTOS on core0.\n");
    for (;;)
        ;
    return 0;
}

void HardFault_Process(HardFault_StackFrame_t *stackFrame)
{
    // 打印HardFault标识
    PRINTF("\r\n=====================================\r\n");
    PRINTF("           HARD FAULT ERROR          \r\n");
    PRINTF("=====================================\r\n");

    // 打印核心寄存器（定位关键）
    PRINTF("R0  = 0x%08X\r\n", stackFrame->r0);
    PRINTF("R1  = 0x%08X\r\n", stackFrame->r1);
    PRINTF("R2  = 0x%08X\r\n", stackFrame->r2);
    PRINTF("R3  = 0x%08X\r\n", stackFrame->r3);
    PRINTF("R12 = 0x%08X\r\n", stackFrame->r12);
    PRINTF("LR  = 0x%08X\r\n", stackFrame->lr); // EXC_RETURN，判断模式
    PRINTF("PC  = 0x%08X\r\n", stackFrame->pc); // 出错的指令地址！！！
    PRINTF("PSR = 0x%08X\r\n", stackFrame->psr);

    // 打印栈类型（MSP/PSP）
    uint32_t control_reg;
    __asm volatile("MRS %0, CONTROL" : "=r"(control_reg)); // 读取CONTROL寄存器
    PRINTF("Stack Type: %s\r\n", (control_reg & 0x02) ? "PSP(Thread)" : "MSP(Handler)");

    // 打印当前MSP/PSP值（辅助排查栈溢出）
    PRINTF("MSP = 0x%08X\r\n", (uint32_t)__get_MSP());
    PRINTF("PSP = 0x%08X\r\n", (uint32_t)__get_PSP());

    // 死循环，防止程序继续运行
    while (1)
    {
    }
}

void HardFault_Handler()
{
    __asm volatile(
        "TST LR, #4                \n" // 检查LR的bit2，判断使用PSP还是MSP
        "ITE EQ                    \n"
        "MRSEQ R0, MSP             \n" // EQ：使用MSP，R0 = MSP（栈帧地址）
        "MRSNE R0, PSP             \n" // NE：使用PSP，R0 = PSP（栈帧地址）
        "B HardFault_Process       \n" // 跳转到C处理函数，R0传递栈帧指针
    );
}
