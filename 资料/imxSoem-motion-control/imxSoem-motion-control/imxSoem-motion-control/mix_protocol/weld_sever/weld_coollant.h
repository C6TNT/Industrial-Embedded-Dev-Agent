#ifndef WELD_COOLLANT_H
#define WELD_COOLLANT_H

#include "weld_sever/weld_system.h"
typedef struct weld_opt weld_opt_t;
typedef struct weld_coollant weld_coollant_t;
typedef enum
{
    work_coollant_idle, // 初始化参数
    work_coollant_work,
    work_coollant_finsh,
} work_coollant_state_t;

typedef enum
{
    end_coollant_idle, // 初始化参数
    end_coollant_work,
    end_coollant_finsh,
} finish_coollant_state_t;

typedef enum
{
    alarm_coollant_idle, // 初始化参数
    alarm_coollant_work,
    alarm_coollant_finsh,
} alarm_coollant_state_t;

typedef enum
{
    check_coollant_idle, // 初始化参数
    check_coollant_work,
} work_coollant_check_state_t;

typedef struct
{
    uint8_t (*weld_coollant_finish)(weld_opt_t *dev);
    finish_coollant_state_t state;
    uint32_t stop_tick;
} weld_coollant_finish_t;

typedef struct
{
    uint8_t (*weld_coollant_work)(weld_opt_t *dev);

    work_coollant_state_t state;
} weld_coollant_work_t;

typedef struct
{
    uint8_t (*weld_coollant_alarm)(weld_opt_t *dev);
    uint32_t stop_tick;
    alarm_coollant_state_t state;
} weld_coollant_alarm_t;

typedef struct
{
    void (*weld_coollant_check)(weld_opt_t *dev);
    work_coollant_check_state_t state;
} weld_coollant_check_t;

struct weld_coollant
{
    weld_coollant_alarm_t *alarm;
    weld_coollant_work_t *work;
    weld_coollant_finish_t *finish;
    weld_coollant_check_t *check;
    void (*reset)(weld_opt_t *dev);
};

extern weld_coollant_t weld_coollant;

#endif // WELD_GAS_H
