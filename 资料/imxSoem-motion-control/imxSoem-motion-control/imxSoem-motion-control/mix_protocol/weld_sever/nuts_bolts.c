#include "nuts_bolts.h"
#include "fsl_debug_console.h"
void detectRisingEdges(uint16_t currentState, volatile uint16_t *previousState, volatile uint16_t *flags)
{
    // 计算上升沿：当前状态为 1 且前一状态为 0
    uint16_t risingEdges = currentState & (~(*previousState));

    // 更新标志位
    *flags |= risingEdges;

    // 更新前一状态
    *previousState = currentState;
}

uint32_t getCycle(uint32_t milliseconds)
{
    return (uint32_t)(milliseconds / ((float)cycleTime / 1000 / 1000));
}

uint32_t linearFunction(uint32_t start_x, uint32_t start_y, uint32_t end_x, uint32_t end_y, uint32_t current_x)
{
    // 处理 x 超出范围的情况
    if (current_x <= start_x)
    {
        return start_y;
    }
    if (current_x >= end_x)
    {
        return end_y;
    }
    if (end_x == start_x)
    {
        return 0;
    }
    // 计算斜率（这里进行整数除法）
    float slope = ((float)end_y - (float)start_y) / ((float)end_x - (float)start_x);
    // 计算截距
    float intercept = (float)start_y - slope * (float)start_x;
    // 计算当前的 y 值
    uint32_t current_y = slope * (float)current_x + intercept;

    return (uint32_t)current_y;
}

int32_t YawlinearFunction(int32_t start_x, int32_t start_y, int32_t end_x, int32_t end_y, int32_t current_x)
{
    // 处理 x 超出范围的情况
    if (current_x <= start_x)
    {
        return start_y;
    }
    if (current_x >= end_x)
    {
        return end_y;
    }

    if (end_x == start_x)
    {
        return 0;
    }
    // 计算斜率（这里进行整数除法）
    float slope = ((float)end_y - (float)start_y) / ((float)end_x - (float)start_x);
    // 计算截距
    float intercept = (float)start_y - slope * (float)start_x;
    // 计算当前的 y 值
    int32_t current_y = slope * (float)current_x + intercept;

    return (int32_t)current_y;
}

int32_t YawlinearFunction_back(int32_t start_x, int32_t start_y, int32_t end_x, int32_t end_y, int32_t current_x)
{
    // 处理 x 超出范围的情况
    if (current_x >= start_x)
    {
        return start_y;
    }
    if (current_x <= end_x)
    {
        return end_y;
    }

    if (end_x == start_x)
    {
        return 0;
    }
    // 计算斜率（这里进行整数除法）
    float slope = ((float)end_y - (float)start_y) / ((float)end_x - (float)start_x);
    // 计算截距
    float intercept = (float)start_y - slope * (float)start_x;
    // 计算当前的 y 值
    int32_t current_y = slope * (float)current_x + intercept;

    return (int32_t)current_y;
}

double get_remain_millimeters(double origin_position, double target_position)
{
    double diff = target_position - origin_position;
    // 手动判断正负：负数则取相反数，非负数直接返回
    return (diff < 0) ? -diff : diff;
}

double get_unit_directional_vector(double origin_position, double target_position)
{
    double diff = target_position - origin_position;
    double abs_diff;

    // 手动计算绝对值
    abs_diff = (diff < 0) ? -diff : diff;

    // 处理接近零的情况（避免除零，考虑浮点数精度）
    if (abs_diff < 1e-9)
    {
        return 0.0;
    }

    return diff / abs_diff;
}