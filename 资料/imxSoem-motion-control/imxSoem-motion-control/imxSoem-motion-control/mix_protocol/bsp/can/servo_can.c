#include "servo_can.h"
#include <string.h>
#include "board.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "fsl_flexcan.h"
#include "fsl_gpio.h"
#include "pin_mux.h"
#include "bsp_basic_tim.h"
#include "task.h"

#define SERVO_CAN_DEVICE FLEXCAN1
#define SERVO_CAN_CLK_FREQ                                                                      \
    (CLOCK_GetPllFreq(kCLOCK_SystemPll1Ctrl) / (CLOCK_GetRootPreDivider(kCLOCK_RootFlexCan1)) / \
     (CLOCK_GetRootPostDivider(kCLOCK_RootFlexCan1)))
#define SERVO_CAN_USE_IMPROVED_TIMING_CONFIG (1U)

#define SERVO_CAN_RX_MB (9U)
#define SERVO_CAN_TX_MB (8U)
#define SERVO_CAN_CHANNEL_COUNT (2U)
#define RPDO1_BASE_COB_ID (0x200U)
#define RPDO2_BASE_COB_ID (0x300U)
#define SERVO_MODE_VELOCITY (0x03U)
#define CTRL_WORD_SHUTDOWN (0x0006U)
#define CTRL_WORD_SWITCH_ON (0x0007U)
#define CTRL_WORD_ENABLE_OPERATION (0x000FU)
#define STATUS_WORD_OPERATION_ENABLED_MASK (0x006FU)
#define STATUS_WORD_OPERATION_ENABLED_VALUE (0x0027U)
#define SERVO_CAN_RX_TIMEOUT_US (50000U)

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

typedef struct
{
    uint8_t nodeId;
    const char *name;
    struct motor_attribute *attribute;
    ServoCan_Feedback feedback;
    PDOData_t pdoData;
    uint32_t runtimeCounter;
    int32_t lastReportedPdoSpeed;
    bool runtimeCommandActive;
    bool lastRuntimeCommandActive;
    bool waitingMessageShown;
    bool profileRampInitialized;
    uint32_t lastAcc;
    uint32_t lastDec;
    int32_t lastAppliedTargetSpeed;
    int32_t latchedTargetSpeed;
    uint8_t latchedMode;
    uint16_t latchedCtrl;
    uint32_t latchedAcc;
    uint32_t latchedDec;
    bool latchedCommandValid;
    bool recoveryAttempted;
} ServoCan_Channel;

static flexcan_handle_t s_flexcanHandle;
static volatile bool s_txComplete = false;
static volatile bool s_rxComplete = false;
static volatile bool s_wakenUp = false;
static flexcan_mb_transfer_t s_txXfer;
static flexcan_mb_transfer_t s_rxXfer;
static flexcan_frame_t s_frame;
static flexcan_rx_mb_config_t s_rxMbConfig;

static ServoCan_Config s_config;
static ServoCan_Channel s_channels[SERVO_CAN_CHANNEL_COUNT];
static bool s_isConfigured = false;
static void CAN_Send(uint32_t id, uint64_t data, uint8_t length);
static void CAN_StartReceive(uint32_t id);
static bool CAN_WaitReceive(uint32_t timeoutUs);
static bool SDO_ReadU32(uint8_t nodeId, uint16_t index, uint8_t subindex, uint32_t *outValue);
static void SDO_Write(uint8_t nodeId, uint16_t index, uint8_t subindex, uint32_t data, uint8_t dataSize);
static void FillPDOData(PDOData_t *pdoData, uint8_t mode, uint16_t ctrl, int32_t speed);
static void SendPDOData(uint8_t nodeId, PDOData_t *pdoData, bool logIfChanged);
static void SendRPDORepeated(uint8_t nodeId, PDOData_t *pdoData, uint32_t repeatCount, uint32_t periodUs, bool logIfChanged);
static bool IsServoOperationEnabled(uint16_t statusWord);
static bool ReadStatusWord(uint8_t nodeId, uint16_t *outValue);
static bool ReadModeOfOperation(uint8_t nodeId, int8_t *outValue);
static bool ReadControlWord(uint8_t nodeId, uint16_t *outValue);
static bool ReadActualPosition(uint8_t nodeId, int32_t *outValue);
static bool ReadActualSpeed(uint8_t nodeId, int32_t *outValue);
static bool ReadTargetVelocity(uint8_t nodeId, int32_t *outValue);
static bool ReadProfileAcceleration(uint8_t nodeId, uint32_t *outValue);
static bool ReadProfileDeceleration(uint8_t nodeId, uint32_t *outValue);
static bool ReadModeDisplay(uint8_t nodeId, int8_t *outValue);
static void WriteProfileAcceleration(uint8_t nodeId, uint32_t acc);
static void WriteProfileDeceleration(uint8_t nodeId, uint32_t dec);
static void WriteModeOfOperation(uint8_t nodeId, int8_t mode);
static void WriteControlWord(uint8_t nodeId, uint16_t controlWord);
static void WriteTargetVelocity(uint8_t nodeId, int32_t targetSpeed);
static bool WriteTargetVelocityVerified(uint8_t nodeId, int32_t targetSpeed, uint8_t retryCount);
static void ApplyProfileRampIfChanged(uint8_t nodeId);
static void EnsureProfileRampApplied(ServoCan_Channel *channel, uint32_t targetAcc, uint32_t targetDec);
static bool EnsureDriveCommandApplied(ServoCan_Channel *channel, uint32_t targetAcc, uint32_t targetDec, int32_t targetSpeed);
static void ServoCan_SendSafeStop(uint8_t nodeId);
static void ServoCan_ExitExcitationState(uint8_t nodeId);
static void ServoCan_PrimeVelocityMode(uint8_t nodeId);
static void ServoCan_LatchRuntimeCommand(ServoCan_Channel *channel, int32_t runtimeTargetSpeed);
static void ServoCan_UpdateLatchedCommand(ServoCan_Channel *channel, int32_t runtimeTargetSpeed);
static void UpdateFeedbackCache(uint8_t nodeId);
static void LogCachedFeedback(const ServoCan_Channel *channel, const char *tag);
static void LogDriveCoreState(uint8_t nodeId, const char *tag);
static void ClearRPDO1Mapping(uint8_t nodeId);
static void ConfigureRPDO1Mapping(uint8_t nodeId);
static void SetRPDO1MappingNumber(uint8_t nodeId);
static void ConfigureRPDO1Communication(uint8_t nodeId);
static void ClearRPDO2Mapping(uint8_t nodeId);
static void ConfigureRPDO2Mapping(uint8_t nodeId);
static void SetRPDO2MappingNumber(uint8_t nodeId);
static void ConfigureRPDO2Communication(uint8_t nodeId);
static void SendNMTPreOperationalCommand(uint8_t nodeId);
static void SendNMTStartNodeCommand(uint8_t nodeId);
static struct motor_attribute *ServoCan_GetAttributeForChannel(weld_opt_t *dev, uint8_t channelIndex);
static bool ServoCan_HasRuntimeCommand(const struct motor_attribute *attribute);
static void ServoCan_InitChannel(ServoCan_Channel *channel, uint8_t nodeId, const char *name);
static void ServoCan_StartChannel(ServoCan_Channel *channel);
static void ServoCan_ServiceChannel(ServoCan_Channel *channel);
static void ServoCan_SyncFeedbackToAttribute(ServoCan_Channel *channel);
static FLEXCAN_CALLBACK(ServoCan_FlexcanCallback);
static uint32_t ServoCan_GetTimeUs(void);
static void ServoCan_DebugMarkAttribute(struct motor_attribute *attribute, uint16_t code);

void ServoCan_LoadDefaultConfig(ServoCan_Config *config)
{
    if (config == NULL)
    {
        return;
    }

    config->nodeId = 1U;
    config->secondaryNodeId = 2U;
    config->targetSpeed = 1000000;
    config->targetAcc = 3000000U;
    config->targetDec = 3000000U;
    config->startupPeriodUs = 8000U;
    config->runtimeFeedbackCycles = 8U;
}

void ServoCan_InitHardware(void)
{
    flexcan_config_t flexcanConfig;
    gpio_pin_config_t gpioConfig = {kGPIO_DigitalOutput, 1, kGPIO_NoIntmode};

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

    ServoCan_InitChannel(&s_channels[0], s_config.nodeId, "rot");
    ServoCan_InitChannel(&s_channels[1], s_config.secondaryNodeId, "wire");
    s_isConfigured = true;
}

void ServoCan_Start(weld_opt_t *dev)
{
    if (!s_isConfigured)
    {
        return;
    }

    s_channels[0].attribute = ServoCan_GetAttributeForChannel(dev, 0U);
    s_channels[1].attribute = ServoCan_GetAttributeForChannel(dev, 1U);

    /* Dual-drive startup is more sensitive to power-up timing than the
     * single-drive path. Give both FD5 nodes enough time to complete boot
     * before the first SDO/PDO configuration burst; node 1 was otherwise
     * noticeably less stable than node 2 after cold power-up. */
    osDelay(1500);
    ServoCan_StartChannel(&s_channels[0]);
    osDelay(500);
    ServoCan_StartChannel(&s_channels[1]);

    UpdateFeedbackCache(s_channels[0].nodeId);
    ServoCan_SyncFeedbackToAttribute(&s_channels[0]);
    UpdateFeedbackCache(s_channels[1].nodeId);
    ServoCan_SyncFeedbackToAttribute(&s_channels[1]);

    if ((dev != NULL) && (dev->rot != NULL))
    {
        dev->rot->attribute.f_target_vel = 0.0;
        dev->rot->attribute.target_vel = 0;
        dev->rot->attribute.drive_target_vel = 0;
        dev->rot->attribute.control_mode = 0;
        dev->rot->attribute.move_mode = 0;
        dev->rot->attribute.state = move_idle;
    }
    if ((dev != NULL) && (dev->wire != NULL))
    {
        dev->wire->attribute.f_target_vel = 0.0;
        dev->wire->attribute.target_vel = 0;
        dev->wire->attribute.drive_target_vel = 0;
        dev->wire->attribute.control_mode = 0;
        dev->wire->attribute.move_mode = 0;
        dev->wire->attribute.state = move_idle;
    }

    osDelay(50);
    LOG_INFO("SMG waiting: CAN initialized, NMT started, waiting for main.py motion command\r\n");
    s_channels[0].waitingMessageShown = true;
    s_channels[1].waitingMessageShown = true;
}

void ServoCan_SetTargetSpeed(int32_t targetSpeed)
{
    s_config.targetSpeed = targetSpeed;
    FillPDOData(&s_channels[0].pdoData, SERVO_MODE_VELOCITY, CTRL_WORD_ENABLE_OPERATION, s_config.targetSpeed);
}

void ServoCan_SetTargetAcc(uint32_t targetAcc)
{
    s_config.targetAcc = targetAcc;
}

void ServoCan_SetTargetDec(uint32_t targetDec)
{
    s_config.targetDec = targetDec;
}

void ServoCan_SetModeOfOperation(int8_t modeOfOperation)
{
    s_channels[0].pdoData.mode = (uint8_t)modeOfOperation;
}

void ServoCan_SetControlWord(uint16_t controlWord)
{
    s_channels[0].pdoData.ctrl = controlWord;
}

void ServoCan_Service(weld_opt_t *dev)
{
    uint8_t i;

    if (!s_isConfigured)
    {
        return;
    }

    for (i = 0U; i < SERVO_CAN_CHANNEL_COUNT; i++)
    {
        ServoCan_Channel *channel = &s_channels[i];
        int32_t runtimeTargetSpeed = 0;
        bool shouldDrive = false;

        channel->attribute = ServoCan_GetAttributeForChannel(dev, i);
        channel->runtimeCommandActive = ServoCan_HasRuntimeCommand(channel->attribute);

        if (channel->attribute != NULL)
        {
            uint16_t debugCode = channel->runtimeCommandActive ? 0xD100U : 0xD000U;
            if (channel->attribute->control_mode == CTRL_WORD_ENABLE_OPERATION)
            {
                debugCode |= 0x0010U;
            }
            if ((channel->attribute->mode_of_operation == 3) || (channel->attribute->mode_of_operation == 9))
            {
                debugCode |= 0x0020U;
            }
            if ((channel->attribute->target_vel != 0) || (channel->attribute->f_target_vel != 0.0))
            {
                debugCode |= 0x0001U;
            }
            ServoCan_DebugMarkAttribute(channel->attribute, debugCode);
        }

        if (channel->attribute != NULL)
        {
            runtimeTargetSpeed = channel->attribute->target_vel;
            if ((runtimeTargetSpeed == 0) && (channel->attribute->f_target_vel != 0.0))
            {
                double transper = (channel->attribute->f_transper == 0.0) ? 1.0 : channel->attribute->f_transper;
                int32_t direction = (channel->attribute->direciton == 0) ? 1 : channel->attribute->direciton;
                runtimeTargetSpeed = (int32_t)(channel->attribute->f_target_vel * transper * direction);
            }
        }

        if (channel->runtimeCommandActive)
        {
            ServoCan_UpdateLatchedCommand(channel, runtimeTargetSpeed);
        }

        shouldDrive = channel->runtimeCommandActive || channel->latchedCommandValid;

        if (shouldDrive)
        {
            channel->pdoData.mode = channel->latchedMode;
            channel->pdoData.ctrl = channel->latchedCtrl;
            channel->pdoData.speed = (runtimeTargetSpeed != 0) ? runtimeTargetSpeed : channel->latchedTargetSpeed;
        }

        if (channel->runtimeCommandActive != channel->lastRuntimeCommandActive)
        {
            if (channel->runtimeCommandActive)
            {
                ServoCan_LatchRuntimeCommand(channel, runtimeTargetSpeed);
                LOG_INFO("SMG running: %s main.py motion command received, entering runtime loop\r\n", channel->name);
            }
            else if (!channel->waitingMessageShown)
            {
                LOG_INFO("SMG waiting: %s CAN initialized, NMT started, waiting for main.py motion command\r\n", channel->name);
                channel->waitingMessageShown = true;
            }
            channel->lastRuntimeCommandActive = channel->runtimeCommandActive;
        }

        if (shouldDrive && (channel->pdoData.speed != channel->lastReportedPdoSpeed))
        {
            ServoCan_LatchRuntimeCommand(channel, channel->pdoData.speed);
            LOG_INFO("ServoCan target speed latched: %s speed=%d\r\n", channel->name, channel->pdoData.speed);
            channel->lastReportedPdoSpeed = channel->pdoData.speed;
            channel->waitingMessageShown = false;
        }

        ServoCan_ServiceChannel(channel);
    }
}

static bool ServoCan_HasRuntimeCommand(const struct motor_attribute *attribute)
{
    if (attribute == NULL)
    {
        return false;
    }

    /* Protect only an explicit direct-drive window. axis0 can boot with stale
     * non-zero accel/decel defaults, so accel/decel alone must not arm CAN
     * runtime output or they get latched as a false command. */
    return (attribute->target_vel != 0) ||
           (attribute->f_target_vel != 0.0) ||
           ((attribute->control_mode == CTRL_WORD_ENABLE_OPERATION) &&
            ((attribute->mode_of_operation == 3) || (attribute->mode_of_operation == 9)));
}

void ServoCan_ReadFeedback(ServoCan_Feedback *feedback)
{
    UpdateFeedbackCache(s_channels[0].nodeId);
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

    *feedback = s_channels[0].feedback;
}

static struct motor_attribute *ServoCan_GetAttributeForChannel(weld_opt_t *dev, uint8_t channelIndex)
{
    if (dev == NULL)
    {
        return NULL;
    }

    if (channelIndex == 0U)
    {
        return (dev->rot != NULL) ? &dev->rot->attribute : NULL;
    }

    if (channelIndex == 1U)
    {
        return (dev->wire != NULL) ? &dev->wire->attribute : NULL;
    }

    return NULL;
}

static void ServoCan_InitChannel(ServoCan_Channel *channel, uint8_t nodeId, const char *name)
{
    (void)memset(channel, 0, sizeof(*channel));
    channel->nodeId = nodeId;
    channel->name = name;
    channel->lastReportedPdoSpeed = 0x7FFFFFFF;
    channel->lastAppliedTargetSpeed = 0x7FFFFFFF;
    channel->latchedMode = SERVO_MODE_VELOCITY;
    channel->latchedCtrl = CTRL_WORD_ENABLE_OPERATION;
    channel->latchedAcc = s_config.targetAcc;
    channel->latchedDec = s_config.targetDec;
    FillPDOData(&channel->pdoData, SERVO_MODE_VELOCITY, CTRL_WORD_SHUTDOWN, s_config.targetSpeed);
}

static void ServoCan_StartChannel(ServoCan_Channel *channel)
{
    if ((channel == NULL) || (channel->nodeId == 0U))
    {
        return;
    }

    SendNMTPreOperationalCommand(channel->nodeId);
    osDelay(20);
    ClearRPDO1Mapping(channel->nodeId);
    ConfigureRPDO1Mapping(channel->nodeId);
    SetRPDO1MappingNumber(channel->nodeId);
    ConfigureRPDO1Communication(channel->nodeId);
    ClearRPDO2Mapping(channel->nodeId);
    ConfigureRPDO2Mapping(channel->nodeId);
    SetRPDO2MappingNumber(channel->nodeId);
    ConfigureRPDO2Communication(channel->nodeId);
    SendNMTStartNodeCommand(channel->nodeId);
    /* Both FD5 nodes benefit from entering the prime sequence from a clean
     * zero-speed shutdown state after PDO remap. Node 1 still needs separate
     * investigation, but keep the pre-prime excitation exit symmetric so both
     * drives start from the same baseline. */
    ServoCan_ExitExcitationState(channel->nodeId);
    osDelay(50);
    ServoCan_PrimeVelocityMode(channel->nodeId);
}

static void ServoCan_ServiceChannel(ServoCan_Channel *channel)
{
    uint32_t targetAcc;
    uint32_t targetDec;
    int32_t driveTargetSpeed = 0;
    bool driveTargetValid = false;
    bool useSdoOnlyPath = false;

    if ((channel == NULL) || (channel->nodeId == 0U))
    {
        return;
    }

    if (!(channel->runtimeCommandActive || channel->latchedCommandValid))
    {
        channel->runtimeCounter = 0U;
        channel->lastAppliedTargetSpeed = 0x7FFFFFFF;
        channel->recoveryAttempted = false;
        return;
    }

    targetAcc = ((channel->attribute != NULL) && (channel->attribute->acc != 0U)) ? channel->attribute->acc : channel->latchedAcc;
    targetDec = ((channel->attribute != NULL) && (channel->attribute->dec != 0U)) ? channel->attribute->dec : channel->latchedDec;
    if (targetAcc != 0U)
    {
        s_config.targetAcc = targetAcc;
    }
    if (targetDec != 0U)
    {
        s_config.targetDec = targetDec;
    }
    useSdoOnlyPath = (channel->nodeId == s_config.nodeId);

    ApplyProfileRampIfChanged(channel->nodeId);
    EnsureProfileRampApplied(channel, targetAcc, targetDec);
    if (useSdoOnlyPath)
    {
        WriteModeOfOperation(channel->nodeId, (int8_t)channel->pdoData.mode);
        WriteControlWord(channel->nodeId, channel->pdoData.ctrl);
        if ((channel->pdoData.speed != 0) &&
            ((channel->lastAppliedTargetSpeed != channel->pdoData.speed) ||
             (channel->runtimeCounter == 0U) ||
             ((channel->runtimeCounter % 8U) == 0U)))
        {
            (void)WriteTargetVelocityVerified(channel->nodeId, channel->pdoData.speed, 3U);
            channel->lastAppliedTargetSpeed = channel->pdoData.speed;
        }
    }
    else
    {
        if ((channel->pdoData.speed != 0) &&
            ((channel->lastAppliedTargetSpeed != channel->pdoData.speed) ||
             (channel->runtimeCounter == 0U) ||
             ((channel->runtimeCounter % 16U) == 0U)))
        {
            (void)WriteTargetVelocityVerified(channel->nodeId, channel->pdoData.speed, 3U);
            channel->lastAppliedTargetSpeed = channel->pdoData.speed;
        }
        SendPDOData(channel->nodeId, &channel->pdoData, true);
    }
    driveTargetValid = ReadTargetVelocity(channel->nodeId, &driveTargetSpeed);
    if ((channel->attribute != NULL) && driveTargetValid)
    {
        uint16_t debugCode = channel->attribute->error_code;
        channel->attribute->drive_target_vel = driveTargetSpeed;
        if (driveTargetSpeed != 0)
        {
            debugCode |= 0x0008U;
        }
        ServoCan_DebugMarkAttribute(channel->attribute, debugCode);
    }
    if ((!driveTargetValid) || ((channel->runtimeCounter % 8U) == 0U))
    {
        LogDriveCoreState(channel->nodeId, "Runtime core");
    }
    if ((channel->attribute != NULL) &&
        (channel->attribute->drive_target_vel != 0))
    {
        channel->recoveryAttempted = false;
    }

    if ((channel->nodeId == s_config.nodeId) &&
        (channel->attribute != NULL) &&
        channel->runtimeCommandActive &&
        (channel->pdoData.speed != 0) &&
        (channel->attribute->drive_target_vel == 0) &&
        (!channel->recoveryAttempted || ((channel->runtimeCounter % 32U) == 0U)) &&
        (((channel->attribute->drive_mode_of_operation == 0) ||
          (channel->attribute->drive_control_word == 0x0F00U)) ||
         ((channel->attribute->drive_mode_of_operation == SERVO_MODE_VELOCITY) &&
          (channel->attribute->drive_control_word == CTRL_WORD_ENABLE_OPERATION))))
    {
        channel->recoveryAttempted = true;
        LOG_INFO("ServoCan recovery: node=%u relatch due to drive target not latched\r\n", channel->nodeId);
        EnsureProfileRampApplied(channel, targetAcc, targetDec);
        WriteModeOfOperation(channel->nodeId, (int8_t)channel->pdoData.mode);
        WriteControlWord(channel->nodeId, channel->pdoData.ctrl);
        if (channel->pdoData.speed != 0)
        {
            (void)WriteTargetVelocityVerified(channel->nodeId, channel->pdoData.speed, 3U);
            channel->lastAppliedTargetSpeed = channel->pdoData.speed;
        }
        if ((channel->attribute->drive_mode_of_operation == 0) ||
            (channel->attribute->drive_control_word == 0x0F00U))
        {
            ServoCan_PrimeVelocityMode(channel->nodeId);
        }
        if (!useSdoOnlyPath)
        {
            ServoCan_LatchRuntimeCommand(channel, channel->pdoData.speed);
        }
    }

    channel->runtimeCounter++;
    if (channel->runtimeCounter >= s_config.runtimeFeedbackCycles)
    {
        channel->runtimeCounter = 0U;
        UpdateFeedbackCache(channel->nodeId);
        ServoCan_SyncFeedbackToAttribute(channel);
        LogCachedFeedback(channel, "Runtime feedback");
    }
}

static void ServoCan_DebugMarkAttribute(struct motor_attribute *attribute, uint16_t code)
{
    if (attribute != NULL)
    {
        attribute->error_code = code;
    }
}

static void ServoCan_SyncFeedbackToAttribute(ServoCan_Channel *channel)
{
    if ((channel == NULL) || (channel->attribute == NULL))
    {
        return;
    }

    channel->attribute->actual_position = channel->feedback.actualPosition;
    channel->attribute->actual_vel = channel->feedback.actualSpeed;
    channel->attribute->status_code = channel->feedback.statusWord;
    channel->attribute->drive_mode_display = channel->feedback.modeDisplay;
    channel->attribute->drive_status_word = channel->feedback.statusWord;

    /* Keep the commanded control word owned by the A-core / parameter path.
     * Overwriting control_mode from feedback causes dual-drive startup to
     * drop out of operation enabled before both nodes finish their bring-up. */
    channel->attribute->state = channel->feedback.operationEnabled ? move_run : move_idle;
}

static void CAN_Send(uint32_t id, uint64_t data, uint8_t length)
{
    s_frame.id = FLEXCAN_ID_STD(id);
    s_frame.length = length;
    s_frame.type = (uint8_t)kFLEXCAN_FrameTypeData;
    s_frame.format = (uint8_t)kFLEXCAN_FrameFormatStandard;
    /* This SDK revision exposes payload as dataWord0/dataWord1 instead of the
     * older aggregate .data field. Keep the legacy word-wise packing because
     * the FD5 behavior regressed after we switched to explicit dataByte writes. */
    s_frame.dataWord0 = (uint32_t)(data & 0xFFFFFFFFULL);
    s_frame.dataWord1 = (uint32_t)((data >> 32) & 0xFFFFFFFFULL);

    s_txXfer.mbIdx = SERVO_CAN_TX_MB;
    s_txXfer.frame = &s_frame;

    s_txComplete = false;
    if (FLEXCAN_TransferSendNonBlocking(SERVO_CAN_DEVICE, &s_flexcanHandle, &s_txXfer) != kStatus_Success)
    {
        LOG_INFO("CAN tx start failed: id=0x%03X\r\n", id);
        return;
    }

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
    s_rxComplete = false;

    s_rxXfer.mbIdx = SERVO_CAN_RX_MB;
    s_rxXfer.frame = &s_frame;
    (void)FLEXCAN_TransferReceiveNonBlocking(SERVO_CAN_DEVICE, &s_flexcanHandle, &s_rxXfer);
}

static bool CAN_WaitReceive(uint32_t timeoutUs)
{
    uint32_t startUs = ServoCan_GetTimeUs();

    while (!s_rxComplete)
    {
        if ((ServoCan_GetTimeUs() - startUs) >= timeoutUs)
        {
            FLEXCAN_TransferAbortReceive(SERVO_CAN_DEVICE, &s_flexcanHandle, SERVO_CAN_RX_MB);
            return false;
        }
    }
    s_rxComplete = false;
    return true;
}

static bool SDO_ReadU32(uint8_t nodeId, uint16_t index, uint8_t subindex, uint32_t *outValue)
{
    uint64_t value;

    if (outValue == NULL)
    {
        return false;
    }

    value = ((uint64_t)subindex) |
            ((uint64_t)((index >> 8) & 0xFFU) << 8) |
            ((uint64_t)(index & 0xFFU) << 16) |
            ((uint64_t)0x40U << 24);

    CAN_StartReceive(0x580U + nodeId);
    CAN_Send(0x600U + nodeId, value, 8U);
    if (!CAN_WaitReceive(SERVO_CAN_RX_TIMEOUT_US))
    {
        LOG_INFO("SDO read timeout: node=%u index=0x%04X sub=0x%02X\r\n", nodeId, index, subindex);
        return false;
    }

    if (s_frame.dataByte0 == 0x80U)
    {
        LOG_INFO("SDO read abort: node=%u index=0x%04X sub=0x%02X code=0x%02X%02X%02X%02X\r\n",
                 nodeId,
                 index,
                 subindex,
                 s_frame.dataByte7,
                 s_frame.dataByte6,
                 s_frame.dataByte5,
                 s_frame.dataByte4);
        return false;
    }

    *outValue = ((uint32_t)s_frame.dataByte7) |
                ((uint32_t)s_frame.dataByte6 << 8) |
                ((uint32_t)s_frame.dataByte5 << 16) |
                ((uint32_t)s_frame.dataByte4 << 24);
    return true;
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
    if (!CAN_WaitReceive(SERVO_CAN_RX_TIMEOUT_US))
    {
        LOG_INFO("SDO write timeout: node=%u index=0x%04X sub=0x%02X data=0x%08X\r\n",
                 nodeId,
                 index,
                 subindex,
                 data);
    }
}

static void FillPDOData(PDOData_t *pdoData, uint8_t mode, uint16_t ctrl, int32_t speed)
{
    pdoData->mode = mode;
    pdoData->ctrl = ctrl;
    pdoData->speed = speed;
}

static void SendPDOData(uint8_t nodeId, PDOData_t *pdoData, bool logIfChanged)
{
    static bool firstLog = true;
    static uint8_t lastNodeId = 0U;
    static uint8_t lastMode = 0U;
    static uint16_t lastCtrl = 0U;
    static int32_t lastSpeed = 0;

    /* This FlexCAN SDK exposes dataByte0..7 in controller word order
     * (payload byte3..0, then byte7..4), not wire order. Keep the legacy
     * assignment from the known-good single-drive implementation so the
     * resulting on-bus payload is:
     * [mode, ctrl_lo, ctrl_hi, speed_lo, speed_b1, speed_b2, speed_b3, 0]. */
    pdoData->dataByte0 = (uint8_t)((uint32_t)pdoData->speed & 0xFFU);
    pdoData->dataByte1 = (uint8_t)(pdoData->ctrl >> 8);
    pdoData->dataByte2 = (uint8_t)(pdoData->ctrl & 0xFFU);
    pdoData->dataByte3 = pdoData->mode;
    pdoData->dataByte4 = 0x00U;
    pdoData->dataByte5 = (uint8_t)(((uint32_t)pdoData->speed >> 24) & 0xFFU);
    pdoData->dataByte6 = (uint8_t)(((uint32_t)pdoData->speed >> 16) & 0xFFU);
    pdoData->dataByte7 = (uint8_t)(((uint32_t)pdoData->speed >> 8) & 0xFFU);

    CAN_Send(RPDO1_BASE_COB_ID + nodeId, pdoData->data, 8U);
    if (logIfChanged &&
        (firstLog || (lastNodeId != nodeId) || (lastMode != pdoData->mode) || (lastCtrl != pdoData->ctrl) ||
         (lastSpeed != pdoData->speed)))
    {
        LOG_INFO("RPDO1 armed: node=%u mode=0x%02X ctrl=0x%04X speed=%d bytes=[%02X %02X %02X %02X %02X %02X %02X %02X]\r\n",
                 nodeId,
                 pdoData->mode,
                 pdoData->ctrl,
                 pdoData->speed,
                 pdoData->dataByte0,
                 pdoData->dataByte1,
                 pdoData->dataByte2,
                 pdoData->dataByte3,
                 pdoData->dataByte4,
                 pdoData->dataByte5,
                 pdoData->dataByte6,
                 pdoData->dataByte7);
        firstLog = false;
        lastNodeId = nodeId;
        lastMode = pdoData->mode;
        lastCtrl = pdoData->ctrl;
        lastSpeed = pdoData->speed;
    }
}

static void SendRPDORepeated(uint8_t nodeId, PDOData_t *pdoData, uint32_t repeatCount, uint32_t periodUs, bool logIfChanged)
{
    uint32_t i;

    for (i = 0U; i < repeatCount; i++)
    {
        SendPDOData(nodeId, pdoData, logIfChanged);
        SDK_DelayAtLeastUs(periodUs, SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY);
    }
}

static bool IsServoOperationEnabled(uint16_t statusWord)
{
    return (statusWord & STATUS_WORD_OPERATION_ENABLED_MASK) == STATUS_WORD_OPERATION_ENABLED_VALUE;
}

static bool ReadStatusWord(uint8_t nodeId, uint16_t *outValue)
{
    uint32_t rawValue;
    if (!SDO_ReadU32(nodeId, 0x6041U, 0x00U, &rawValue))
    {
        return false;
    }
    *outValue = (uint16_t)(rawValue & 0xFFFFU);
    return true;
}

static bool ReadModeDisplay(uint8_t nodeId, int8_t *outValue)
{
    uint32_t rawValue;
    if (!SDO_ReadU32(nodeId, 0x6061U, 0x00U, &rawValue))
    {
        return false;
    }
    *outValue = (int8_t)(rawValue & 0xFFU);
    return true;
}

static bool ReadModeOfOperation(uint8_t nodeId, int8_t *outValue)
{
    uint32_t rawValue;
    if (!SDO_ReadU32(nodeId, 0x6060U, 0x00U, &rawValue))
    {
        return false;
    }
    *outValue = (int8_t)(rawValue & 0xFFU);
    return true;
}

static bool ReadControlWord(uint8_t nodeId, uint16_t *outValue)
{
    uint32_t rawValue;
    if (!SDO_ReadU32(nodeId, 0x6040U, 0x00U, &rawValue))
    {
        return false;
    }
    *outValue = (uint16_t)(rawValue & 0xFFFFU);
    return true;
}

static bool ReadActualPosition(uint8_t nodeId, int32_t *outValue)
{
    uint32_t rawValue;
    if (!SDO_ReadU32(nodeId, 0x6064U, 0x00U, &rawValue))
    {
        return false;
    }
    *outValue = (int32_t)rawValue;
    return true;
}

static bool ReadActualSpeed(uint8_t nodeId, int32_t *outValue)
{
    uint32_t rawValue;
    if (!SDO_ReadU32(nodeId, 0x606CU, 0x00U, &rawValue))
    {
        return false;
    }
    *outValue = (int32_t)rawValue;
    return true;
}

static bool ReadTargetVelocity(uint8_t nodeId, int32_t *outValue)
{
    uint32_t rawValue;
    if (!SDO_ReadU32(nodeId, 0x60FFU, 0x00U, &rawValue))
    {
        return false;
    }
    *outValue = (int32_t)rawValue;
    return true;
}

static bool ReadProfileAcceleration(uint8_t nodeId, uint32_t *outValue)
{
    return SDO_ReadU32(nodeId, 0x6083U, 0x00U, outValue);
}

static bool ReadProfileDeceleration(uint8_t nodeId, uint32_t *outValue)
{
    return SDO_ReadU32(nodeId, 0x6084U, 0x00U, outValue);
}

static void WriteProfileAcceleration(uint8_t nodeId, uint32_t acc)
{
    SDO_Write(nodeId, 0x6083U, 0x00U, acc, 4U);
}

static void WriteProfileDeceleration(uint8_t nodeId, uint32_t dec)
{
    SDO_Write(nodeId, 0x6084U, 0x00U, dec, 4U);
}

static void WriteModeOfOperation(uint8_t nodeId, int8_t mode)
{
    SDO_Write(nodeId, 0x6060U, 0x00U, (uint32_t)(uint8_t)mode, 1U);
}

static void WriteControlWord(uint8_t nodeId, uint16_t controlWord)
{
    SDO_Write(nodeId, 0x6040U, 0x00U, controlWord, 2U);
}

static void WriteTargetVelocity(uint8_t nodeId, int32_t targetSpeed)
{
    SDO_Write(nodeId, 0x60FFU, 0x00U, (uint32_t)targetSpeed, 4U);
}

static bool WriteTargetVelocityVerified(uint8_t nodeId, int32_t targetSpeed, uint8_t retryCount)
{
    uint8_t attempt;
    int32_t actualTargetSpeed = 0;

    for (attempt = 0U; attempt < retryCount; attempt++)
    {
        WriteTargetVelocity(nodeId, targetSpeed);
        SDK_DelayAtLeastUs(2000U, SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY);
        if (ReadTargetVelocity(nodeId, &actualTargetSpeed) && (actualTargetSpeed == targetSpeed))
        {
            return true;
        }
    }

    LOG_INFO("Target velocity write not confirmed: node=%u target=%d actual=%d retries=%u\r\n",
             nodeId,
             targetSpeed,
             actualTargetSpeed,
             retryCount);
    return false;
}

static void ApplyProfileRampIfChanged(uint8_t nodeId)
{
    ServoCan_Channel *channel = NULL;
    uint32_t actualAcc;
    uint32_t actualDec;
    uint8_t i;

    for (i = 0U; i < SERVO_CAN_CHANNEL_COUNT; i++)
    {
        if (s_channels[i].nodeId == nodeId)
        {
            channel = &s_channels[i];
            break;
        }
    }
    if (channel == NULL)
    {
        return;
    }

    /* FD5 is stable when profile ramp values are written through SDO and then
     * read back for confirmation. Do not replace this with runtime RPDO2
     * payload transmission unless the drive-side behavior has been revalidated
     * on hardware and in KincoServo+. */
    if ((!channel->profileRampInitialized) || (channel->lastAcc != s_config.targetAcc))
    {
        channel->lastAcc = s_config.targetAcc;
        WriteProfileAcceleration(nodeId, s_config.targetAcc);
        if (!ReadProfileAcceleration(nodeId, &actualAcc))
        {
            actualAcc = channel->lastAcc;
        }
        LOG_INFO("SDO profile acc node=%u target=%u actual=%u\r\n", nodeId, channel->lastAcc, actualAcc);
    }

    if ((!channel->profileRampInitialized) || (channel->lastDec != s_config.targetDec))
    {
        channel->lastDec = s_config.targetDec;
        WriteProfileDeceleration(nodeId, s_config.targetDec);
        if (!ReadProfileDeceleration(nodeId, &actualDec))
        {
            actualDec = channel->lastDec;
        }
        LOG_INFO("SDO profile dec node=%u target=%u actual=%u\r\n", nodeId, channel->lastDec, actualDec);
    }

    channel->profileRampInitialized = true;
}

static void EnsureProfileRampApplied(ServoCan_Channel *channel, uint32_t targetAcc, uint32_t targetDec)
{
    uint32_t actualAcc = 0U;
    uint32_t actualDec = 0U;
    bool accValid;
    bool decValid;

    if ((channel == NULL) || (channel->nodeId == 0U))
    {
        return;
    }

    if ((targetAcc == 0U) && (targetDec == 0U))
    {
        return;
    }

    if ((channel->runtimeCounter != 0U) && ((channel->runtimeCounter % 16U) != 0U))
    {
        return;
    }

    accValid = ReadProfileAcceleration(channel->nodeId, &actualAcc);
    decValid = ReadProfileDeceleration(channel->nodeId, &actualDec);

    if (targetAcc != 0U && (!accValid || actualAcc != targetAcc))
    {
        WriteProfileAcceleration(channel->nodeId, targetAcc);
        channel->lastAcc = targetAcc;
        LOG_INFO("Ramp repair acc: node=%u expected=%u actual=%u valid=%d\r\n",
                 channel->nodeId,
                 targetAcc,
                 actualAcc,
                 accValid ? 1 : 0);
    }

    if (targetDec != 0U && (!decValid || actualDec != targetDec))
    {
        WriteProfileDeceleration(channel->nodeId, targetDec);
        channel->lastDec = targetDec;
        LOG_INFO("Ramp repair dec: node=%u expected=%u actual=%u valid=%d\r\n",
                 channel->nodeId,
                 targetDec,
                 actualDec,
                 decValid ? 1 : 0);
    }
}

static bool EnsureDriveCommandApplied(ServoCan_Channel *channel, uint32_t targetAcc, uint32_t targetDec, int32_t targetSpeed)
{
    uint32_t actualAcc = 0U;
    uint32_t actualDec = 0U;
    int32_t actualTarget = 0;
    bool accValid;
    bool decValid;
    bool targetValid;
    bool shouldRefreshTarget;
    bool changed = false;

    if ((channel == NULL) || (channel->nodeId == 0U))
    {
        return false;
    }

    EnsureProfileRampApplied(channel, targetAcc, targetDec);

    accValid = ReadProfileAcceleration(channel->nodeId, &actualAcc);
    decValid = ReadProfileDeceleration(channel->nodeId, &actualDec);
    targetValid = ReadTargetVelocity(channel->nodeId, &actualTarget);

    if (targetAcc != 0U && (!accValid || actualAcc != targetAcc))
    {
        WriteProfileAcceleration(channel->nodeId, targetAcc);
        channel->lastAcc = targetAcc;
        changed = true;
    }

    if (targetDec != 0U && (!decValid || actualDec != targetDec))
    {
        WriteProfileDeceleration(channel->nodeId, targetDec);
        channel->lastDec = targetDec;
        changed = true;
    }

    shouldRefreshTarget = (targetSpeed != 0) &&
                          ((!targetValid || actualTarget != targetSpeed) &&
                           ((channel->lastAppliedTargetSpeed != targetSpeed) ||
                            (channel->runtimeCounter == 0U) ||
                            ((channel->runtimeCounter % 16U) == 0U) ||
                            (channel->nodeId == s_config.nodeId)));
    if (shouldRefreshTarget)
    {
        (void)WriteTargetVelocityVerified(channel->nodeId, targetSpeed, 3U);
        channel->lastAppliedTargetSpeed = targetSpeed;
        changed = true;
    }

    if (changed)
    {
        accValid = ReadProfileAcceleration(channel->nodeId, &actualAcc);
        decValid = ReadProfileDeceleration(channel->nodeId, &actualDec);
        targetValid = ReadTargetVelocity(channel->nodeId, &actualTarget);
        LOG_INFO("Drive command repair: node=%u acc=%u/%u dec=%u/%u target=%d/%d valid=[%d %d %d]\r\n",
                 channel->nodeId,
                 targetAcc,
                 accValid ? actualAcc : 0U,
                 targetDec,
                 decValid ? actualDec : 0U,
                 targetSpeed,
                 targetValid ? actualTarget : 0,
                 accValid ? 1 : 0,
                 decValid ? 1 : 0,
                 targetValid ? 1 : 0);
    }

    return targetValid && (actualTarget == targetSpeed);
}

static void ServoCan_SendSafeStop(uint8_t nodeId)
{
    PDOData_t safePdo;

    FillPDOData(&safePdo, SERVO_MODE_VELOCITY, CTRL_WORD_SHUTDOWN, 0);
    SendRPDORepeated(nodeId, &safePdo, 3U, s_config.startupPeriodUs, false);
}

static void ServoCan_ExitExcitationState(uint8_t nodeId)
{
    ServoCan_SendSafeStop(nodeId);
    (void)WriteTargetVelocityVerified(nodeId, 0, 3U);
}

static void ServoCan_PrimeVelocityMode(uint8_t nodeId)
{
    PDOData_t startupPdo;
    int8_t modeDisplay;

    /* Some FD5 nodes do not accept a direct jump into runtime PDO control
     * immediately after boot. Prime the node with an explicit SDO mode write
     * and a conservative 0x06 -> 0x07 -> 0x0F zero-speed sequence first. */
    WriteModeOfOperation(nodeId, (int8_t)SERVO_MODE_VELOCITY);
    if (!ReadModeDisplay(nodeId, &modeDisplay))
    {
        modeDisplay = -1;
    }
    LOG_INFO("Startup mode prime: node=%u requested=%d display=%d\r\n",
             nodeId,
             SERVO_MODE_VELOCITY,
             modeDisplay);

    FillPDOData(&startupPdo, SERVO_MODE_VELOCITY, CTRL_WORD_SHUTDOWN, 0);
    SendRPDORepeated(nodeId, &startupPdo, 3U, s_config.startupPeriodUs, false);

    FillPDOData(&startupPdo, SERVO_MODE_VELOCITY, CTRL_WORD_SWITCH_ON, 0);
    SendRPDORepeated(nodeId, &startupPdo, 3U, s_config.startupPeriodUs, false);

    FillPDOData(&startupPdo, SERVO_MODE_VELOCITY, CTRL_WORD_ENABLE_OPERATION, 0);
    SendRPDORepeated(nodeId, &startupPdo, 3U, s_config.startupPeriodUs, false);
    LogDriveCoreState(nodeId, "Prime state");
}

static void ServoCan_LatchRuntimeCommand(ServoCan_Channel *channel, int32_t runtimeTargetSpeed)
{
    PDOData_t startupPdo;

    if ((channel == NULL) || (channel->nodeId == 0U))
    {
        return;
    }

    WriteModeOfOperation(channel->nodeId, (int8_t)SERVO_MODE_VELOCITY);
    ApplyProfileRampIfChanged(channel->nodeId);
    WriteControlWord(channel->nodeId, CTRL_WORD_SHUTDOWN);
    WriteControlWord(channel->nodeId, CTRL_WORD_SWITCH_ON);
    WriteControlWord(channel->nodeId, CTRL_WORD_ENABLE_OPERATION);
    (void)WriteTargetVelocityVerified(channel->nodeId, runtimeTargetSpeed, 3U);
    FillPDOData(&startupPdo, SERVO_MODE_VELOCITY, CTRL_WORD_ENABLE_OPERATION, runtimeTargetSpeed);
    SendRPDORepeated(channel->nodeId, &startupPdo, 3U, s_config.startupPeriodUs, false);
    LogDriveCoreState(channel->nodeId, "Latch state");
}

static void ServoCan_UpdateLatchedCommand(ServoCan_Channel *channel, int32_t runtimeTargetSpeed)
{
    if ((channel == NULL) || (channel->attribute == NULL))
    {
        return;
    }

    /* The weld task can briefly reset rot/wire runtime fields even in direct CAN
     * control mode. Keep the most recent non-zero direct-drive command latched so
     * the drive does not get a spurious 60FF=0/old-ramp update in the next cycle. */
    channel->latchedMode = ((channel->attribute->mode_of_operation == 3) || (channel->attribute->mode_of_operation == 9))
                               ? (uint8_t)channel->attribute->mode_of_operation
                               : SERVO_MODE_VELOCITY;
    channel->latchedCtrl = (channel->attribute->control_mode != 0U) ? channel->attribute->control_mode : CTRL_WORD_ENABLE_OPERATION;
    channel->latchedAcc = (channel->attribute->acc != 0U) ? channel->attribute->acc : channel->latchedAcc;
    channel->latchedDec = (channel->attribute->dec != 0U) ? channel->attribute->dec : channel->latchedDec;
    if (runtimeTargetSpeed != 0)
    {
        channel->latchedTargetSpeed = runtimeTargetSpeed;
        channel->latchedCommandValid = true;
    }
    else if ((channel->latchedAcc != 0U) || (channel->latchedDec != 0U) ||
             (channel->latchedCtrl != 0U))
    {
        channel->latchedCommandValid = true;
    }
}

static void UpdateFeedbackCache(uint8_t nodeId)
{
    uint8_t i;
    int8_t modeDisplay;
    uint16_t statusWord;
    int32_t actualPosition;
    int32_t actualSpeed;

    for (i = 0U; i < SERVO_CAN_CHANNEL_COUNT; i++)
    {
        if (s_channels[i].nodeId == nodeId)
        {
            if (ReadModeDisplay(nodeId, &modeDisplay))
            {
                s_channels[i].feedback.modeDisplay = modeDisplay;
            }
            if (ReadStatusWord(nodeId, &statusWord))
            {
                s_channels[i].feedback.statusWord = statusWord;
            }
            if (ReadActualPosition(nodeId, &actualPosition))
            {
                s_channels[i].feedback.actualPosition = actualPosition;
            }
            if (ReadActualSpeed(nodeId, &actualSpeed))
            {
                s_channels[i].feedback.actualSpeed = actualSpeed;
            }
            s_channels[i].feedback.operationEnabled = IsServoOperationEnabled(s_channels[i].feedback.statusWord);
            return;
        }
    }
}

static void LogCachedFeedback(const ServoCan_Channel *channel, const char *tag)
{
    uint32_t profileAcc;
    uint32_t profileDec;
    bool accValid;
    bool decValid;

    if ((channel == NULL) || (channel->nodeId == 0U))
    {
        return;
    }

    accValid = ReadProfileAcceleration(channel->nodeId, &profileAcc);
    decValid = ReadProfileDeceleration(channel->nodeId, &profileDec);
    if (!accValid)
    {
        profileAcc = channel->lastAcc;
    }
    if (!decValid)
    {
        profileDec = channel->lastDec;
    }

    LOG_INFO("%s[%s]: modeDisplay=%d status=0x%04X actualSpeed=%d actualPosition=%d enabled=%d profileAcc=%u profileDec=%u\r\n",
             tag,
             channel->name,
             channel->feedback.modeDisplay,
             channel->feedback.statusWord,
             channel->feedback.actualSpeed,
             channel->feedback.actualPosition,
             channel->feedback.operationEnabled ? 1 : 0,
             profileAcc,
             profileDec);
}

static void LogDriveCoreState(uint8_t nodeId, const char *tag)
{
    int8_t modeOfOperation = -1;
    int8_t modeDisplay = -1;
    uint16_t controlWord = 0U;
    uint16_t statusWord = 0U;
    int32_t targetVelocity = 0;

    (void)ReadModeOfOperation(nodeId, &modeOfOperation);
    (void)ReadModeDisplay(nodeId, &modeDisplay);
    (void)ReadControlWord(nodeId, &controlWord);
    (void)ReadStatusWord(nodeId, &statusWord);
    (void)ReadTargetVelocity(nodeId, &targetVelocity);

    if (nodeId == s_channels[0].nodeId && s_channels[0].attribute != NULL)
    {
        s_channels[0].attribute->drive_mode_of_operation = modeOfOperation;
        s_channels[0].attribute->drive_mode_display = modeDisplay;
        s_channels[0].attribute->drive_control_word = controlWord;
        s_channels[0].attribute->drive_status_word = statusWord;
        s_channels[0].attribute->drive_target_vel = targetVelocity;
    }
    if (nodeId == s_channels[1].nodeId && s_channels[1].attribute != NULL)
    {
        s_channels[1].attribute->drive_mode_of_operation = modeOfOperation;
        s_channels[1].attribute->drive_mode_display = modeDisplay;
        s_channels[1].attribute->drive_control_word = controlWord;
        s_channels[1].attribute->drive_status_word = statusWord;
        s_channels[1].attribute->drive_target_vel = targetVelocity;
    }

    LOG_INFO("%s: node=%u mode=0x%02X display=0x%02X ctrl=0x%04X status=0x%04X target=%d\r\n",
             tag,
             nodeId,
             (uint8_t)modeOfOperation,
             (uint8_t)modeDisplay,
             controlWord,
             statusWord,
             targetVelocity);
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
    SDO_Write(nodeId, 0x1400U, 0x03U, 0U, 2U);
    SDO_Write(nodeId, 0x1400U, 0x01U, (uint32_t)(RPDO1_BASE_COB_ID + nodeId), 4U);
}

static void ClearRPDO2Mapping(uint8_t nodeId)
{
    SDO_Write(nodeId, 0x1601U, 0x00U, 0x00U, 1U);
}

static void ConfigureRPDO2Mapping(uint8_t nodeId)
{
    /* Mapping is intentionally kept for documentation/alignment with the drive
     * configuration UI. Current production code must not transmit runtime
     * RPDO2 ramp data, because that path has been verified to corrupt the
     * drive-side 0x6083/0x6084 values on Kinco FD5. */
    SDO_Write(nodeId, 0x1601U, 0x01U, 0x60830020U, 4U);
    SDO_Write(nodeId, 0x1601U, 0x02U, 0x60840020U, 4U);
}

static void SetRPDO2MappingNumber(uint8_t nodeId)
{
    SDO_Write(nodeId, 0x1601U, 0x00U, 0x02U, 1U);
}

static void ConfigureRPDO2Communication(uint8_t nodeId)
{
    SDO_Write(nodeId, 0x1401U, 0x01U, (uint32_t)(0x80000000U | (RPDO2_BASE_COB_ID + nodeId)), 4U);
    SDO_Write(nodeId, 0x1401U, 0x02U, 255U, 1U);
    SDO_Write(nodeId, 0x1401U, 0x03U, 0U, 2U);
    SDO_Write(nodeId, 0x1401U, 0x01U, (uint32_t)(RPDO2_BASE_COB_ID + nodeId), 4U);
}

static void SendNMTPreOperationalCommand(uint8_t nodeId)
{
    CAN_Send(0x000U, (((uint64_t)0x80U) | ((uint64_t)nodeId << 8)) << 16, 8U);
}

static void SendNMTStartNodeCommand(uint8_t nodeId)
{
    CAN_Send(0x000U, (((uint64_t)0x01U) | ((uint64_t)nodeId << 8)) << 16, 8U);
}

static uint32_t ServoCan_GetTimeUs(void)
{
    return GetSec() * 1000000U + GetUSec();
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
