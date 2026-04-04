#ifndef WELD_WIRE_H
#define WELD_WIRE_H

#include "weld_sever/weld_system.h"
#include "weld_sever/weld_motor.h"
typedef struct weld_opt weld_opt_t;
typedef struct weld_wire weld_wire_t;

typedef enum
{
    wire_curise_idle,
    wire_curise_work_ready,
    wire_curise_work,
} wire_curise_state_t;

typedef enum
{
    wire_pre_idle,
    wire_pre_work,
    wire_pre_finish,
} wire_pre_state_t;

typedef enum
{
    wire_back_idle,
    wire_back_work,
    wire_back_extract,
} wire_back_state_t;

typedef enum
{
    wire_extract_idle,
    wire_extract_work,
    wire_extract_finish,
} wire_extract_state_t;

typedef struct
{
    wire_curise_state_t state;
    uint8_t (*weld_wire_moving)(weld_opt_t *dev);
    int32_t rot_end_position;
    int32_t rot_start_position;
    uint16_t start_section;
    uint16_t end_section;
} wire_curise_t;

typedef struct
{
    wire_extract_state_t state;
    uint8_t (*weld_wire_extract)(weld_opt_t *dev);
    uint32_t stop_tick;
} wire_extract_t;

typedef struct
{
    wire_pre_state_t state;
    uint8_t (*weld_wire_pre)(weld_opt_t *dev);
    uint32_t stop_tick;
} wire_pre_t;

typedef struct
{
    wire_back_state_t state;
    uint8_t (*weld_wire_back)(weld_opt_t *dev);
    int32_t rot_end_position;
} wire_back_t;

struct weld_wire
{
    struct motor_attribute attribute;
    wire_curise_t *curise;
    wire_pre_t *pre;
    wire_extract_t *extract;
    bool enable;
    void (*check)(weld_opt_t *dev);
    void (*estop)(weld_opt_t *dev);
    void (*reset)(weld_opt_t *dev);
};
extern weld_wire_t weld_wire;
#endif // WELD_WIRE_H
