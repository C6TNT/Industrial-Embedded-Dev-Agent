

#include "fsl_debug_console.h"
#include "servo_can.h"
#include "bsp_basic_tim.h"

#define SERVO_CAN_STANDALONE_TEST (0)

static const TickType_t xServicePeriod = pdMS_TO_TICKS(4);

typedef struct
{
    bool valid;
    double fTargetVel;
    int32_t targetVel;
    uint16_t controlMode;
    int8_t modeOfOperation;
    uint32_t acc;
    uint32_t dec;
    int8_t direction;
    double transmission;
    uint8_t moveMode;
    uint8_t state;
    int32_t targetPosition;
} ServoCan_CommandSnapshot;

static bool ServoCan_IsDirectCommandActive(const struct motor_attribute *attribute)
{
    if (attribute == NULL)
    {
        return false;
    }

    /* Protect only an explicit direct-drive window. axis0 can power up with
     * stale non-zero accel/decel defaults (for example the legacy 107372 pair),
     * so accel/decel alone must not arm the snapshot path. */
    return (attribute->target_vel != 0) ||
           (attribute->f_target_vel != 0.0) ||
           ((attribute->control_mode == 15U) &&
            ((attribute->mode_of_operation == 3) || (attribute->mode_of_operation == 9)));
}

static void ServoCan_SaveDirectCommand(const struct motor_attribute *attribute, ServoCan_CommandSnapshot *snapshot)
{
    if ((attribute == NULL) || (snapshot == NULL))
    {
        return;
    }

    snapshot->valid = ServoCan_IsDirectCommandActive(attribute);
    if (!snapshot->valid)
    {
        return;
    }

    snapshot->fTargetVel = attribute->f_target_vel;
    snapshot->targetVel = attribute->target_vel;
    snapshot->controlMode = attribute->control_mode;
    snapshot->modeOfOperation = attribute->mode_of_operation;
    snapshot->acc = attribute->acc;
    snapshot->dec = attribute->dec;
    snapshot->direction = attribute->direciton;
    snapshot->transmission = attribute->f_transper;
    snapshot->moveMode = attribute->move_mode;
    snapshot->state = attribute->state;
    snapshot->targetPosition = attribute->target_position;
}

static void ServoCan_RestoreDirectCommand(struct motor_attribute *attribute, const ServoCan_CommandSnapshot *snapshot)
{
    if ((attribute == NULL) || (snapshot == NULL) || !snapshot->valid)
    {
        return;
    }

    /* weld_task may keep axis-specific state machines alive even when we are in
     * direct CAN velocity mode. Always restore the commanded mode/control/ramp
     * fields so background logic cannot silently drag one channel back to stale
     * defaults such as the legacy 107372 accel/decel pair. */
    attribute->control_mode = snapshot->controlMode;
    attribute->mode_of_operation = snapshot->modeOfOperation;
    attribute->acc = snapshot->acc;
    attribute->dec = snapshot->dec;
    attribute->direciton = snapshot->direction;
    attribute->f_transper = snapshot->transmission;

    /* For direct CAN velocity mode, treat the saved command as authoritative.
     * rot-specific background logic can transiently zero only target_vel while
     * leaving f_target_vel non-zero, which bypasses the older "both zero"
     * restore condition and leaves node1 stuck at drive-side 60FF = 0. */
    attribute->f_target_vel = snapshot->fTargetVel;
    attribute->target_vel = snapshot->targetVel;
    attribute->move_mode = snapshot->moveMode;
    attribute->state = snapshot->state;
    attribute->target_position = snapshot->targetPosition;
}

static void ServoCan_DebugMark(weld_opt_t *dev, uint16_t code)
{
    if (dev == NULL)
    {
        return;
    }

    if (dev->rot != NULL)
    {
        dev->rot->attribute.error_code = code;
    }
    if (dev->wire != NULL)
    {
        dev->wire->attribute.error_code = code;
    }
}

void can_loop(void *param)
{
    weld_opt_t *dev = (weld_opt_t *)param;
    ServoCan_CommandSnapshot rotSnapshot = {0};
    ServoCan_CommandSnapshot wireSnapshot = {0};
    ServoCan_DebugMark(dev, 0xC001U);
    PRINTF("CAN loop task entered\r\n");
    MX_TIM2_Init(); // init gpt timer
    ServoCan_DebugMark(dev, 0xC002U);
    PRINTF("CAN timer initialized\r\n");
    ServoCan_Config config;

    ServoCan_LoadDefaultConfig(&config);
    ServoCan_InitHardware();
    ServoCan_DebugMark(dev, 0xC003U);
    ServoCan_Init(&config);
    ServoCan_DebugMark(dev, 0xC004U);

    PRINTF("Start servo by ServoCan interface\r\n\r\n");

    ServoCan_Start(dev);
    ServoCan_DebugMark(dev, 0xC005U);

#if SERVO_CAN_STANDALONE_TEST
    PRINTF("Servo CAN standalone test mode enabled\r\n");
    ServoCan_SetTargetSpeed(config.targetSpeed);
    while (true)
    {
        ServoCan_Service(NULL);
        osDelay(8);
    }
#else
    while (true)
    {
        if (xSemaphoreTake(dev->xMutex, portMAX_DELAY) == pdTRUE)
        {
            if ((dev != NULL) && (dev->rot != NULL))
            {
                ServoCan_SaveDirectCommand(&dev->rot->attribute, &rotSnapshot);
            }
            if ((dev != NULL) && (dev->wire != NULL))
            {
                ServoCan_SaveDirectCommand(&dev->wire->attribute, &wireSnapshot);
            }

            dev->system->weld_task(dev);

            if ((dev != NULL) && (dev->rot != NULL))
            {
                ServoCan_RestoreDirectCommand(&dev->rot->attribute, &rotSnapshot);
            }
            if ((dev != NULL) && (dev->wire != NULL))
            {
                ServoCan_RestoreDirectCommand(&dev->wire->attribute, &wireSnapshot);
            }
            xSemaphoreGive(dev->xMutex);
        }

        /* Do not hold the RPMsg/shared parameter mutex across the CAN SDO/PDO
         * service routine. Dual-drive SDO transactions are long enough to
         * starve rpmsg_loop replies, which shows up on the A-core as
         * parameter_set read timeout (for example 0x4001/0x0A on wire). */
        ServoCan_Service(dev);
        osDelay(xServicePeriod);
    }
#endif
}
