#ifndef WELD_GAS_H
#define WELD_GAS_H

#include "weld_sever/weld_system.h"
typedef struct weld_opt weld_opt_t;
typedef struct weld_gas weld_gas_t;
typedef enum
{
    pre_gas_idle, // 初始化参数
    pre_gas_work,
    pre_gas_finsh,
} pre_gas_state_t;

typedef enum
{
    check_gas_idle, // 初始化参数
    check_gas_work,
} check_gas_state_t;

typedef enum
{
    lag_gas_idle, // 初始化参数
    lag_gas_work,
    lag_gas_finsh,
} lag_gas_state_t;

typedef enum
{
    alarm_gas_idle, // 初始化参数
    alarm_gas_work,
    alarm_gas_finsh,
} alarm_gas_state_t;

typedef struct
{
    uint8_t (*weld_gas_pre)(weld_opt_t *dev);
    uint32_t stop_tick;
    pre_gas_state_t state;
} weld_gas_pre_t;

typedef struct
{
    uint8_t (*weld_gas_lag)(weld_opt_t *dev);
    uint32_t stop_tick;
    lag_gas_state_t state;
} weld_gas_lag_t;

typedef struct
{
    uint8_t (*weld_gas_alarm)(weld_opt_t *dev);
    uint32_t stop_tick;
    alarm_gas_state_t state;
} weld_gas_alarm_t;

typedef struct
{
    void (*weld_gas_check)(weld_opt_t *dev);
    check_gas_state_t state;
} weld_gas_check_t;

struct weld_gas
{
    weld_gas_alarm_t *alarm;
    weld_gas_lag_t *lag;
    weld_gas_pre_t *pre;
    weld_gas_check_t *check;
    void (*reset)(weld_opt_t *dev);
};

extern weld_gas_t weld_gas;

#endif // WELD_GAS_H
