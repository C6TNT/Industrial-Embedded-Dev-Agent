#include "servo_can.h"

#include <string.h>

#include "board.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "fsl_flexcan.h"
#include "fsl_gpio.h"
#include "pin_mux.h"

#define SERVO_CAN_DEVICE FLEXCAN1
#define SERVO_CAN_CLK_FREQ                                                                    \
    (CLOCK_GetPllFreq(kCLOCK_SystemPll1Ctrl) / (CLOCK_GetRootPreDivider(kCLOCK_RootFlexCan1)) / \
     (CLOCK_GetRootPostDivider(kCLOCK_RootFlexCan1)))
#define SERVO_CAN_USE_IMPROVED_TIMING_CONFIG (1U)

#define SERVO_CAN_RX_MB (9U)
#define SERVO_CAN_TX_MB (8U)
#define RPDO1_BASE_COB_ID (0x200U)

#define SERVO_MODE_VELOCITY (0x03U)
#define CTRL_WORD_SHUTDOWN (0x0006U)
#define CTRL_WORD_SWITCH_ON (0x0007U)
#define CTRL_WORD_ENABLE_OPERATION (0x000FU)
#define STATUS_WORD_OPERATION_ENABLED_MASK (0x006FU)
#define STATUS_WORD_OPERATION_ENABLED_VALUE (0x0027U)

#define LOG_INFO (void)PRINTF

typedef struct
{
    struct
    {
        uint8_t mode;
        uint16_t ctrl;
        int32_t speed;
    };
    union
    {
        uint64_t data;
        struct
        {
            uint8_t dataByte0;
            uint8_t dataByte1;
            uint8_t dataByte2;
            uint8_t dataByte3;
            uint8_t dataByte4;
            uint8_t dataByte5;
            uint8_t dataByte6;
            uint8_t dataByte7;
        };
    };
} PDOData_t;

static flexcan_handle_t s_flexcanHandle;
static volatile bool s_txComplete = false;
static volatile bool s_rxComplete = false;
static volatile bool s_wakenUp = false;
static flexcan_mb_transfer_t s_txXfer;
static flexcan_mb_transfer_t s_rxXfer;
static flexcan_frame_t s_frame;
static flexcan_rx_mb_config_t s_rxMbConfig;

static ServoCan_Config s_config;
static ServoCan_Feedback s_feedbackCache;
static PDOData_t s_pdoData;
static uint32_t s_runtimeCounter = 0U;
static bool s_isConfigured = false;

static void CAN_Send(uint32_t id, uint64_t data, uint8_t length);
static void CAN_StartReceive(uint32_t id);
static void CAN_WaitReceive(void);
static uint32_t SDO_ReadU32(uint8_t nodeId, uint16_t index, uint8_t subindex);
static void SDO_Write(uint8_t nodeId, uint16_t index, uint8_t subindex, uint32_t data, uint8_t dataSize);
static void FillPDOData(PDOData_t *pdoData, uint8_t mode, uint16_t ctrl, int32_t speed);
static void SendPDOData(uint8_t nodeId, PDOData_t *pdoData);
static void SendRPDORepeated(uint8_t nodeId, PDOData_t *pdoData, uint32_t repeatCount, uint32_t periodUs);
static bool IsServoOperationEnabled(uint16_t statusWord);
static uint16_t ReadStatusWord(uint8_t nodeId);
static int32_t ReadActualPosition(uint8_t nodeId);
static void UpdateFeedbackCache(uint8_t nodeId);
static void LogCachedFeedback(const char *tag, bool expectEnabled);
static void ClearRPDO1Mapping(uint8_t nodeId);
static void ConfigureRPDO1Mapping(uint8_t nodeId);
static void SetRPDO1MappingNumber(uint8_t nodeId);
static void ConfigureRPDO1Communication(uint8_t nodeId);
static void SendNMTStartNodeCommand(uint8_t nodeId);
static FLEXCAN_CALLBACK(ServoCan_FlexcanCallback);

void ServoCan_LoadDefaultConfig(ServoCan_Config *config)
{
    if (config == NULL)
    {
        return;
    }

    config->nodeId = 1U;
    config->targetSpeed = 1000000;
    config->startupPeriodUs = 8000U;
    config->runtimeFeedbackCycles = 125U;
}

void ServoCan_InitHardware(void)
{
    flexcan_config_t flexcanConfig;
    gpio_pin_config_t gpioConfig = {kGPIO_DigitalOutput, 1, kGPIO_NoIntmode};

    BOARD_InitMemory();
    BOARD_RdcInit();
    BOARD_InitBootPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();

    CLOCK_SetRootMux(kCLOCK_RootFlexCan1, kCLOCK_FlexCanRootmuxSysPll1);
    CLOCK_SetRootDivider(kCLOCK_RootFlexCan1, 2U, 5U);

    GPIO_PinInit(GPIO5, 5U, &gpioConfig);

    LOG_INFO("********* FLEXCAN Servo Interface *********\r\n");
    LOG_INFO("    Message format: Standard (11 bit id)\r\n");
    LOG_INFO("    Message buffer %d used for Rx.\r\n", SERVO_CAN_RX_MB);
    LOG_INFO("    Message buffer %d used for Tx.\r\n", SERVO_CAN_TX_MB);
    LOG_INFO("    Runtime path: RPDO + SDO\r\n");
    LOG_INFO("*******************************************\r\n\r\n");

    FLEXCAN_GetDefaultConfig(&flexcanConfig);
    flexcanConfig.enableIndividMask = true;

#if (defined(SERVO_CAN_USE_IMPROVED_TIMING_CONFIG) && SERVO_CAN_USE_IMPROVED_TIMING_CONFIG)
    {
        flexcan_timing_config_t timingConfig;

        (void)memset(&timingConfig, 0, sizeof(timingConfig));
        if (FLEXCAN_CalculateImprovedTimingValues(
                SERVO_CAN_DEVICE, flexcanConfig.bitRate, SERVO_CAN_CLK_FREQ, &timingConfig))
        {
            (void)memcpy(&(flexcanConfig.timingConfig), &timingConfig, sizeof(timingConfig));
        }
        else
        {
            LOG_INFO("No improved timing configuration found, using defaults\r\n");
        }
    }
#endif

    FLEXCAN_Init(SERVO_CAN_DEVICE, &flexcanConfig, SERVO_CAN_CLK_FREQ);
    FLEXCAN_TransferCreateHandle(SERVO_CAN_DEVICE, &s_flexcanHandle, ServoCan_FlexcanCallback, NULL);
    FLEXCAN_SetTxMbConfig(SERVO_CAN_DEVICE, SERVO_CAN_TX_MB, true);
}

void ServoCan_Init(const ServoCan_Config *config)
{
    ServoCan_Config defaultConfig;

    ServoCan_LoadDefaultConfig(&defaultConfig);
    s_config = defaultConfig;
    if (config != NULL)
    {
        s_config = *config;
    }

    if (s_config.startupPeriodUs == 0U)
    {
        s_config.startupPeriodUs = defaultConfig.startupPeriodUs;
    }
    if (s_config.runtimeFeedbackCycles == 0U)
    {
        s_config.runtimeFeedbackCycles = defaultConfig.runtimeFeedbackCycles;
    }

    FillPDOData(&s_pdoData, SERVO_MODE_VELOCITY, CTRL_WORD_SHUTDOWN, s_config.targetSpeed);
    (void)memset(&s_feedbackCache, 0, sizeof(s_feedbackCache));
    s_runtimeCounter = 0U;
    s_isConfigured = true;
}

void ServoCan_Start(void)
{
    if (!s_isConfigured)
    {
        return;
    }

    ClearRPDO1Mapping(s_config.nodeId);
    ConfigureRPDO1Mapping(s_config.nodeId);
    SetRPDO1MappingNumber(s_config.nodeId);
    ConfigureRPDO1Communication(s_config.nodeId);
    SendNMTStartNodeCommand(s_config.nodeId);

    SDK_DelayAtLeastUs(50000U, SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY);

    FillPDOData(&s_pdoData, SERVO_MODE_VELOCITY, CTRL_WORD_SHUTDOWN, s_config.targetSpeed);
    SendRPDORepeated(s_config.nodeId, &s_pdoData, 8U, s_config.startupPeriodUs);
    LogCachedFeedback("After shutdown", false);

    FillPDOData(&s_pdoData, SERVO_MODE_VELOCITY, CTRL_WORD_SWITCH_ON, s_config.targetSpeed);
    SendRPDORepeated(s_config.nodeId, &s_pdoData, 8U, s_config.startupPeriodUs);
    LogCachedFeedback("After switch on", false);

    FillPDOData(&s_pdoData, SERVO_MODE_VELOCITY, CTRL_WORD_ENABLE_OPERATION, s_config.targetSpeed);
    SendRPDORepeated(s_config.nodeId, &s_pdoData, 12U, s_config.startupPeriodUs);
    LogCachedFeedback("After enable operation", true);

    if (!IsServoOperationEnabled(ReadStatusWord(s_config.nodeId)))
    {
        SendRPDORepeated(s_config.nodeId, &s_pdoData, 12U, s_config.startupPeriodUs);
        LogCachedFeedback("Retry enable operation", true);
    }
}

void ServoCan_SetTargetSpeed(int32_t targetSpeed)
{
    s_config.targetSpeed = targetSpeed;
    FillPDOData(&s_pdoData, SERVO_MODE_VELOCITY, CTRL_WORD_ENABLE_OPERATION, s_config.targetSpeed);
}

void ServoCan_Service(void)
{
    if (!s_isConfigured)
    {
        return;
    }

    SendPDOData(s_config.nodeId, &s_pdoData);

    s_runtimeCounter++;

    if (s_runtimeCounter >= s_config.runtimeFeedbackCycles)
    {
        s_runtimeCounter = 0U;
        LogCachedFeedback("Runtime feedback", false);
    }
}

void ServoCan_ReadFeedback(ServoCan_Feedback *feedback)
{
    UpdateFeedbackCache(s_config.nodeId);
    ServoCan_GetCachedFeedback(feedback);
}

uint32_t ServoCan_GetServicePeriodUs(void)
{
    return s_config.startupPeriodUs;
}

void ServoCan_GetCachedFeedback(ServoCan_Feedback *feedback)
{
    if (feedback == NULL)
    {
        return;
    }

    *feedback = s_feedbackCache;
}

static void CAN_Send(uint32_t id, uint64_t data, uint8_t length)
{
    s_frame.id = FLEXCAN_ID_STD(id);
    s_frame.length = length;
    s_frame.type = (uint8_t)kFLEXCAN_FrameTypeData;
    s_frame.format = (uint8_t)kFLEXCAN_FrameFormatStandard;
    s_frame.data = data;

    s_txXfer.mbIdx = SERVO_CAN_TX_MB;
    s_txXfer.frame = &s_frame;

    (void)FLEXCAN_TransferSendNonBlocking(SERVO_CAN_DEVICE, &s_flexcanHandle, &s_txXfer);
    while (!s_txComplete)
    {
    }
    s_txComplete = false;
}

static void CAN_StartReceive(uint32_t id)
{
    FLEXCAN_TransferAbortReceive(SERVO_CAN_DEVICE, &s_flexcanHandle, SERVO_CAN_RX_MB);
    FLEXCAN_SetRxIndividualMask(SERVO_CAN_DEVICE, SERVO_CAN_RX_MB, FLEXCAN_RX_MB_STD_MASK(id, 0, 0));

    s_rxMbConfig.format = kFLEXCAN_FrameFormatStandard;
    s_rxMbConfig.type = kFLEXCAN_FrameTypeData;
    s_rxMbConfig.id = FLEXCAN_ID_STD(id);
    FLEXCAN_SetRxMbConfig(SERVO_CAN_DEVICE, SERVO_CAN_RX_MB, &s_rxMbConfig, true);

    s_rxXfer.mbIdx = SERVO_CAN_RX_MB;
    s_rxXfer.frame = &s_frame;
    (void)FLEXCAN_TransferReceiveNonBlocking(SERVO_CAN_DEVICE, &s_flexcanHandle, &s_rxXfer);
}

static void CAN_WaitReceive(void)
{
    while (!s_rxComplete)
    {
    }
    s_rxComplete = false;

    LOG_INFO("Rx MB ID: 0x%3x, Rx MB data: 0x%02X%02X%02X%02X%02X%02X%02X%02X, Time stamp: %d\r\n",
             (s_frame.id >> CAN_ID_STD_SHIFT),
             s_frame.dataByte3, s_frame.dataByte2, s_frame.dataByte1, s_frame.dataByte0,
             s_frame.dataByte7, s_frame.dataByte6, s_frame.dataByte5, s_frame.dataByte4, s_frame.timestamp);
}

static uint32_t SDO_ReadU32(uint8_t nodeId, uint16_t index, uint8_t subindex)
{
    uint64_t value;

    value = ((uint64_t)subindex) |
            ((uint64_t)((index >> 8) & 0xFFU) << 8) |
            ((uint64_t)(index & 0xFFU) << 16) |
            ((uint64_t)0x40U << 24);

    CAN_StartReceive(0x580U + nodeId);
    CAN_Send(0x600U + nodeId, value, 8U);
    CAN_WaitReceive();

    return ((uint32_t)s_frame.dataByte7) |
           ((uint32_t)s_frame.dataByte6 << 8) |
           ((uint32_t)s_frame.dataByte5 << 16) |
           ((uint32_t)s_frame.dataByte4 << 24);
}

static void SDO_Write(uint8_t nodeId, uint16_t index, uint8_t subindex, uint32_t data, uint8_t dataSize)
{
    uint8_t cs;
    uint64_t value;

    if (dataSize == 1U)
    {
        cs = 0x2FU;
        data &= 0xFFU;
    }
    else if (dataSize == 2U)
    {
        cs = 0x2BU;
        data &= 0xFFFFU;
    }
    else
    {
        cs = 0x23U;
    }

    value = ((uint64_t)subindex) |
            ((uint64_t)((index >> 8) & 0xFFU) << 8) |
            ((uint64_t)(index & 0xFFU) << 16) |
            ((uint64_t)cs << 24) |
            ((uint64_t)((data >> 24) & 0xFFU) << 32) |
            ((uint64_t)((data >> 16) & 0xFFU) << 40) |
            ((uint64_t)((data >> 8) & 0xFFU) << 48) |
            ((uint64_t)(data & 0xFFU) << 56);

    CAN_StartReceive(0x580U + nodeId);
    CAN_Send(0x600U + nodeId, value, 8U);
    CAN_WaitReceive();
}

static void FillPDOData(PDOData_t *pdoData, uint8_t mode, uint16_t ctrl, int32_t speed)
{
    pdoData->mode = mode;
    pdoData->ctrl = ctrl;
    pdoData->speed = speed;
}

static void SendPDOData(uint8_t nodeId, PDOData_t *pdoData)
{
    static bool firstLog = true;
    static uint8_t lastNodeId = 0U;
    static uint8_t lastMode = 0U;
    static uint16_t lastCtrl = 0U;
    static int32_t lastSpeed = 0;

    pdoData->dataByte0 = (uint8_t)((uint32_t)pdoData->speed & 0xFFU);
    pdoData->dataByte1 = (uint8_t)(pdoData->ctrl >> 8);
    pdoData->dataByte2 = (uint8_t)(pdoData->ctrl & 0xFFU);
    pdoData->dataByte3 = pdoData->mode;
    pdoData->dataByte4 = 0x00U;
    pdoData->dataByte5 = (uint8_t)(((uint32_t)pdoData->speed >> 24) & 0xFFU);
    pdoData->dataByte6 = (uint8_t)(((uint32_t)pdoData->speed >> 16) & 0xFFU);
    pdoData->dataByte7 = (uint8_t)(((uint32_t)pdoData->speed >> 8) & 0xFFU);

    CAN_Send(RPDO1_BASE_COB_ID + nodeId, pdoData->data, 8U);
    if (firstLog || (lastNodeId != nodeId) || (lastMode != pdoData->mode) || (lastCtrl != pdoData->ctrl) ||
        (lastSpeed != pdoData->speed))
    {
        LOG_INFO("RPDO1 sent: mode=0x%02X ctrl=0x%04X speed=%d raw=0x%02X%02X%02X%02X%02X%02X%02X%02X\r\n",
                 pdoData->mode,
                 pdoData->ctrl,
                 pdoData->speed,
                 pdoData->dataByte7, pdoData->dataByte6, pdoData->dataByte5, pdoData->dataByte4,
                 pdoData->dataByte3, pdoData->dataByte2, pdoData->dataByte1, pdoData->dataByte0);
        firstLog = false;
        lastNodeId = nodeId;
        lastMode = pdoData->mode;
        lastCtrl = pdoData->ctrl;
        lastSpeed = pdoData->speed;
    }
}

static void SendRPDORepeated(uint8_t nodeId, PDOData_t *pdoData, uint32_t repeatCount, uint32_t periodUs)
{
    uint32_t i;

    for (i = 0U; i < repeatCount; i++)
    {
        SendPDOData(nodeId, pdoData);
        SDK_DelayAtLeastUs(periodUs, SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY);
    }
}

static bool IsServoOperationEnabled(uint16_t statusWord)
{
    return (statusWord & STATUS_WORD_OPERATION_ENABLED_MASK) == STATUS_WORD_OPERATION_ENABLED_VALUE;
}

static uint16_t ReadStatusWord(uint8_t nodeId)
{
    return (uint16_t)(SDO_ReadU32(nodeId, 0x6041U, 0x00U) & 0xFFFFU);
}

static int32_t ReadActualPosition(uint8_t nodeId)
{
    return (int32_t)SDO_ReadU32(nodeId, 0x6064U, 0x00U);
}

static void UpdateFeedbackCache(uint8_t nodeId)
{
    uint32_t modeDisplay = SDO_ReadU32(nodeId, 0x6061U, 0x00U);

    s_feedbackCache.modeDisplay = (int8_t)(modeDisplay & 0xFFU);
    s_feedbackCache.statusWord = ReadStatusWord(nodeId);
    s_feedbackCache.actualSpeed = (int32_t)SDO_ReadU32(nodeId, 0x606CU, 0x00U);
    s_feedbackCache.actualPosition = ReadActualPosition(nodeId);
    s_feedbackCache.operationEnabled = IsServoOperationEnabled(s_feedbackCache.statusWord);
}

static void LogCachedFeedback(const char *tag, bool expectEnabled)
{
    UpdateFeedbackCache(s_config.nodeId);

    LOG_INFO("%s: modeDisplay=%d status=0x%04X actualSpeed=%d actualPosition=%d\r\n",
             tag,
             s_feedbackCache.modeDisplay,
             s_feedbackCache.statusWord,
             s_feedbackCache.actualSpeed,
             s_feedbackCache.actualPosition);

    if (expectEnabled && !s_feedbackCache.operationEnabled)
    {
        LOG_INFO("%s: warning, servo is not in Operation Enabled state yet\r\n", tag);
    }
}

static void ClearRPDO1Mapping(uint8_t nodeId)
{
    SDO_Write(nodeId, 0x1600U, 0x00U, 0x00U, 1U);
}

static void ConfigureRPDO1Mapping(uint8_t nodeId)
{
    SDO_Write(nodeId, 0x1600U, 0x01U, 0x60600008U, 4U);
    SDO_Write(nodeId, 0x1600U, 0x02U, 0x60400010U, 4U);
    SDO_Write(nodeId, 0x1600U, 0x03U, 0x60FF0020U, 4U);
}

static void SetRPDO1MappingNumber(uint8_t nodeId)
{
    SDO_Write(nodeId, 0x1600U, 0x00U, 0x03U, 1U);
}

static void ConfigureRPDO1Communication(uint8_t nodeId)
{
    SDO_Write(nodeId, 0x1400U, 0x01U, (uint32_t)(0x80000000U | (RPDO1_BASE_COB_ID + nodeId)), 4U);
    SDO_Write(nodeId, 0x1400U, 0x02U, 255U, 1U);
    SDO_Write(nodeId, 0x1400U, 0x01U, (uint32_t)(RPDO1_BASE_COB_ID + nodeId), 4U);
}

static void SendNMTStartNodeCommand(uint8_t nodeId)
{
    CAN_Send(0x000U, (((uint64_t)0x01U) | ((uint64_t)nodeId << 8)) << 16, 8U);
}

static FLEXCAN_CALLBACK(ServoCan_FlexcanCallback)
{
    switch (status)
    {
        case kStatus_FLEXCAN_RxIdle:
            if (SERVO_CAN_RX_MB == result)
            {
                s_rxComplete = true;
            }
            break;

        case kStatus_FLEXCAN_TxIdle:
            if (SERVO_CAN_TX_MB == result)
            {
                s_txComplete = true;
            }
            break;

        case kStatus_FLEXCAN_WakeUp:
            s_wakenUp = true;
            break;

        default:
            break;
    }
}
