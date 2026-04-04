#ifndef SERVO_CAN_H_
#define SERVO_CAN_H_

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Servo CAN 运行配置。
 *
 * 用于配置节点号、目标速度、启动阶段发送周期和运行期反馈回读周期。
 */
typedef struct
{
    uint8_t nodeId;
    int32_t targetSpeed;
    uint32_t startupPeriodUs;
    uint32_t runtimeFeedbackCycles;
} ServoCan_Config;

/**
 * @brief 通过 SDO 回读得到的伺服反馈。
 *
 * 当前版本反馈内容包括模式显示值、状态字、实际速度、实际位置，
 * 以及根据状态字计算出的是否进入 Operation Enabled 状态。
 */
typedef struct
{
    int8_t modeDisplay;
    uint16_t statusWord;
    int32_t actualSpeed;
    int32_t actualPosition;
    bool operationEnabled;
} ServoCan_Feedback;

/**
 * @brief 加载默认配置。
 *
 * 默认参数：
 * - nodeId = 1
 * - targetSpeed = 1000000
 * - startupPeriodUs = 8000
 * - runtimeFeedbackCycles = 125
 *
 * @param[out] config 配置结构体指针。
 */
void ServoCan_LoadDefaultConfig(ServoCan_Config *config);

/**
 * @brief 初始化当前 EVK 工程使用的板级资源和 FlexCAN1。
 *
 * 如果后续移植到别的工程或板卡，通常优先调整这个函数。
 */
void ServoCan_InitHardware(void);

/**
 * @brief 保存运行配置并准备内部 RPDO 数据。
 *
 * @param[in] config 配置结构体指针。
 */
void ServoCan_Init(const ServoCan_Config *config);

/**
 * @brief 执行一次完整的 RPDO 启动流程。
 *
 * 启动顺序固定为：
 * 0x06 -> 0x07 -> 0x0F
 *
 * 启动过程中会插入少量 SDO 回读，用于确认模式显示值、状态字、
 * 实际速度和实际位置。
 */
void ServoCan_Start(void);

/**
 * @brief 更新运行期 RPDO 使用的目标速度。
 *
 * @param[in] targetSpeed 新的目标速度。
 */
void ServoCan_SetTargetSpeed(int32_t targetSpeed);

/**
 * @brief 执行一次运行期服务。
 *
 * 每次调用发送一帧 RPDO；当累计次数达到 runtimeFeedbackCycles 时，
 * 额外做一次 SDO 回读并刷新内部反馈缓存。
 */
void ServoCan_Service(void);

/**
 * @brief 立即通过 SDO 回读一次最新反馈。
 *
 * @param[out] feedback 输出反馈结构体指针。
 */
void ServoCan_ReadFeedback(ServoCan_Feedback *feedback);

/**
 * @brief 获取推荐的周期服务调用间隔，单位为微秒。
 */
uint32_t ServoCan_GetServicePeriodUs(void);

/**
 * @brief 读取最近一次缓存的反馈，不触发新的 SDO 通信。
 *
 * @param[out] feedback 输出反馈结构体指针。
 */
void ServoCan_GetCachedFeedback(ServoCan_Feedback *feedback);

#endif
