#ifndef _WELD_CURRENT_H_
#define _WELD_CURRENT_H_
#include "weld_sever/nuts_bolts.h"
#include "weld_sever/weld_system.h"
typedef struct weld_opt weld_opt_t;
typedef struct weld_current weld_current_t;

typedef enum
{
    high_freq_weld_idle, // 初始化参数
    high_freq_weld_work,
    high_freq_weld_check,
    high_freq_weld_finsh,
} high_freq_weld_state_t;

typedef enum
{
    lift_arc_weld_idle, // 初始化参数
    lift_arc_weld_forward,
    lift_arc_weld_first_end,
    lift_arc_weld_backward,
    lift_arc_weld_check,
    lift_arc_weld_second_end,
} lift_arc_weld_state_t;

typedef enum
{
    peak_state,
    base_state,
    peak_pulse_state, // 尖脉冲
    peak_base_idle,
} peak_base_state_t;

typedef enum
{
    pre_weld_idle, // 初始化参数
    pre_weld_work,
    pre_weld_finsh,
} pre_weld_state_t;

typedef enum
{
    rise_weld_idle, // 初始化参数
    rise_weld_work,
    rise_weld_finsh,
} rise_weld_state_t;

typedef enum
{
    down_weld_idle, // 初始化参数
    down_weld_work,
    down_weld_finsh,
} down_weld_state_t;

typedef enum
{
    curise_weld_idle, // 初始化参数
    curise_weld_work,
} curise_weld_state_t;

typedef struct
{
    uint8_t (*weld_current_high_freq)(weld_opt_t *dev);
    uint32_t stop_tick;
    uint32_t alarm_tick_out;
    high_freq_weld_state_t state;
} weld_current_high_freq_weld_t;

typedef struct
{
    uint8_t (*weld_current_lift_arc)(weld_opt_t *dev);
    int32_t arc_backward_end_position;
    lift_arc_weld_state_t state;
} weld_current_lift_arc_weld_t;

typedef struct
{
    void (*weld_current_peak_base_change)(weld_opt_t *dev);
    bool enable;
    uint32_t stop_tick;
    peak_base_state_t state;

} weld_current_peak_base_change_t;

typedef struct
{
    uint8_t (*weld_current_pre)(weld_opt_t *dev);
    uint32_t stop_tick;
    pre_weld_state_t state;
} weld_current_pre_weld_t;

typedef struct
{
    uint8_t (*weld_current_rise)(weld_opt_t *dev);
    uint32_t stop_tick;
    rise_weld_state_t state;
} weld_current_rise_weld_t;

typedef struct
{
    uint8_t (*weld_current_down)(weld_opt_t *dev);
    uint32_t stop_tick;
    down_weld_state_t state;
} weld_current_down_weld_t;

typedef struct
{
    uint8_t (*weld_current_curise)(weld_opt_t *dev);
    curise_weld_state_t state;
} weld_current_curise_weld_t;

struct weld_current
{
    weld_current_peak_base_change_t *peak_base_change;
    weld_current_pre_weld_t *pre;
    weld_current_rise_weld_t *rise;
    weld_current_curise_weld_t *curise;
    weld_current_down_weld_t *down;
    weld_current_high_freq_weld_t *high_freq;
    weld_current_lift_arc_weld_t *lift_arc;
    bool enable;
    void (*reset)(weld_opt_t *dev);
    void (*estop)(weld_opt_t *dev);
};

extern weld_current_t weld_current;
#endif
