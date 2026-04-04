#ifndef WELD_MONITOR_H
#define WELD_MONITOR_H

#include "weld_sever/weld_system.h"
typedef struct weld_opt weld_opt_t;
typedef struct weld_monitor weld_monitor_t;
#define ALARM_IDLE 0
#define ALARM_FAIL_ARC bit(0)
#define ALARM_SHORT_CIRCUIT bit(1)
#define ALARM_BREAK_ARC bit(2)
#define ALARM_MACHINE_ERROR bit(3)
#define ALARM_HAND_SHORT_CIRCUIT bit(4)
#define ALARM_ETHERCAT_DISCONNNECT bit(5)
typedef enum
{
    arc_protect_move_idle, // 初始化参数
    arc_protect_move_work,
    arc_protect_move_finsh,
} arc_protect_move_state_t;

typedef enum
{
    monitor_alarm_idle,
    monitor_alarm_wait,
} monitor_alarm_state_t;

typedef struct
{
    monitor_alarm_state_t state;
    void (*monitor_alarm)(weld_opt_t *dev);
} monitor_alarm_t;

typedef struct
{
    arc_protect_move_state_t state;
    uint8_t (*arc_protect_move)(weld_opt_t *dev);
    int32_t end_position;
} arc_protect_move_t;

typedef struct
{
    void (*monitor_work)(weld_opt_t *dev);
    bool arc_protect_move_enable;
} monitor_work_t;

struct weld_monitor
{
    arc_protect_move_t *arc;
    monitor_work_t *work;
    monitor_alarm_t *alarm;
    void (*reset)(weld_opt_t *dev);
};
extern weld_monitor_t weld_monitor;
#endif // WELD_WIRE_H
