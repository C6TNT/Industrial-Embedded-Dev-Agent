#ifndef _WELD_SYSTEM_H_
#define _WELD_SYSTEM_H_
#pragma once

#include "weld_sever/nuts_bolts.h"
#include "weld_sever/weld_position_control.h"
#include "weld_sever/weld_parameter.h"
#include "weld_sever/weld_current.h"
#include "weld_sever/weld_gas.h"
#include "weld_sever/weld_coollant.h"
#include "weld_sever/weld_arc.h"
#include "weld_sever/weld_rot.h"
#include "weld_sever/weld_wire.h"
#include "weld_sever/weld_yaw.h"
#include "weld_sever/weld_angle.h"
#include "weld_sever/weld_monitor.h"
#include "weld_sever/weld_jog.h"
#include "weld_sever/weld_kinematic.h"
#include "ethercat_loop.h"
#define USE_RTOS 1

#if USE_RTOS == 1
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "fsl_debug_console.h"
#endif

typedef struct weld_opt weld_opt_t;
typedef struct weld_gas weld_gas_t;
typedef struct weld_current weld_current_t;
typedef struct weld_coollant weld_coollant_t;
typedef struct weld_arc weld_arc_t;
typedef struct weld_rot weld_rot_t;
typedef struct weld_wire weld_wire_t;
typedef struct weld_yaw weld_yaw_t;
typedef struct weld_angle weld_angle_t;
typedef struct weld_parameter weld_parameter_t;
typedef struct weld_monitor weld_monitor_t;
typedef struct weld_joint_axis weld_joint_axis_t;
typedef struct weld_position_control weld_position_control_t;
typedef struct weld_jog weld_jog_t;
typedef struct weld_kinematic weld_kinematic_t;
typedef struct ethercat_control ethercat_control_t;
/*0.设备类型*/
#define USE_FIVE_AXIS_TUBE_PLATE 0
#define USE_SIX_AXIS_ROBOT 1
#if USE_SIX_AXIS_ROBOT == 1
#define JOINT_MAX_COUNT 6 // 参与联动的轴的总数量
#elif USE_FIVE_AXIS_TUBE_PLATE == 1
#define JOINT_MAX_COUNT 5
#endif
/*0.CAN总线启用*/
#define USE_CAN 1
/*1.IO模块*/
#define USE_STM32F767 0
#define USE_GELIIO 0
/*2.机器人驱动器类型*/
#define USE_GELI 0
#define USE_SV660N 1
/*3.轴数量*/
#define AXIS 6
#define WELD_AXIS 5
/*4.从站物理序列*/
#if WELD_AXIS > 0
#define YAW 2
#define ROT 0
#define WIRE 1
#define ARC 3
#define ANGLE 4
#endif
#if USE_STM32F767 == 1
#define STM32F767_IO 8
#endif
#if USE_GELIIO == 1
#define GELI_IO 2
#endif
#if AXIS > 0
#define JOINT_0 0
#define JOINT_1 1
#define JOINT_2 2
#define JOINT_3 3
#define JOINT_4 4
#define JOINT_5 5
#endif
#define QUEUE_SIZE 16 // 缓冲数量
#ifndef QUICK_COMMUNICATION_PROTOCOL
#define QUICK_COMMUNICATION_PROTOCOL 0
#endif
#ifndef COMMON_COMMUNICATION_PROTOCOL
#define COMMON_COMMUNICATION_PROTOCOL 1
#endif
#ifndef COMMON_COMMUNICATION_SPECIAL
#define COMMON_COMMUNICATION_SPECIAL 1
#endif

// #if QUICK_COMMUNICATION_PROTOCOL == 1 && COMMON_COMMUNICATION_PROTOCOL == 1
// #error "please choose only one communication protocol "
// #endif

#define USE_TUBE_SHEET 0
#define USE_TUBE_TUBE 1

#if USE_TUBE_SHEET == 1
#define STATE_IDLE_WELD 0
#define STATE_PRE_GAS_WELD bit(0)
#define STATE_PRE_WELD bit(1)
#define STATE_RISE_WELD bit(2)
#define STATE_RUN_WELD bit(3)
#define STATE_DOWN_WELD bit(4)
#define STATE_BACK_WELD bit(5)
#define STATE_BACK_DOWN_WELD bit(6)
#define STATE_DELAY_GAS_WELD bit(7)
#define STATE_LIFT_WELD bit(8)
#define STATE_ALARM bit(9)
#define STATE_RESET bit(10)
#define STATE_ARC_WELD bit(11)
#endif
#if USE_TUBE_TUBE == 1
#define STATE_IDLE_WELD 0
#define STATE_RESET bit(0)
#define STATE_ALARM bit(1)
#define STATE_PRE_GAS_WELD bit(2)
#define STATE_ARC_WELD bit(3)
#define STATE_PRE_WELD bit(4)
#define STATE_RISE_WELD bit(5)
#define STATE_RUN_WELD bit(6)
#define STATE_DOWN_WELD bit(7)
#define STATE_TRANSITION_WELD bit(8)
#define STATE_DELAY_GAS_WELD bit(9)
#define STATE_LIFT_WELD bit(10)
#define STATE_SECOND_START bit(11)
#define STATE_SECOND_START_TRANSITION bit(12)
#define STATE_GO_HOME bit(13)

#endif

#define EXEC_STATE_RESET bit(0)
#define EXEC_STATE_ESTOP bit(1)
#define EXEC_STATE_STOP bit(2)
#define EXEC_STATE_START bit(3)
#define EXEC_STATE_SECOND_START bit(4)
#define EXEC_STATE_ANGLE_POSITION_ZERO bit(6)

typedef uint_fast16_t sys_state_t; //!< See \ref sys_state

typedef struct
{
  volatile uint32_t sys_tick;
  sys_state_t state;
  uint16_t prep_state;
  uint16_t exec_state;
} weld_task_block_t;

typedef struct
{
  sys_state_t (*state_get)(weld_opt_t *dev);
  uint32_t (*sys_tick_get)();
  void (*weld_task)(weld_opt_t *dev);
} weld_system_t;

#pragma pack(push, 1)

/*-------------------电机参数 & IO模块---------------*/
#if USE_SV660N == 1
typedef struct
{
  uint16_t controlWord;
  int32_t targetPosition;
  int32_t targetVel;
  uint32_t acc;
  uint32_t dec;
  int8_t modeofOperation;

} motor_out;

struct dataSend
{
#if AXIS > 0
  motor_out motorOut[AXIS];
#endif
#if WELD_AXIS > 0
  motor_out motorOutWeld[WELD_AXIS];
#endif
#if USE_STM32F767
  uint16_t DO_767[MAX_DO_767];
  uint16_t AO_767[MAX_AO_767];
#endif
#if USE_GELIIO
  uint8_t DO_GELI[MAX_DO_GELI];
#endif
};

union send_buf_to_motor_t
{
  char inf[sizeof(struct dataSend)];
  struct dataSend sendbuf;
};

typedef struct
{
  uint16_t statusWord;
  uint16_t errorCode;
  int32_t actualPosition;
  int8_t modeofOperationDisplay;
  uint32_t limitState;
} motor_in;

struct dataRecv
{
#if AXIS > 0
  motor_in motorIn[AXIS];
#endif
#if WELD_AXIS > 0
  motor_in motorInWeld[WELD_AXIS];
#endif
#if USE_STM32F767
  uint16_t DI_767[MAX_DI_767];
  uint16_t AI_767[MAX_AI_767];
#endif
#if USE_GELIIO
  uint8_t DI_GELI[MAX_DI_GELI];
#endif
};

union recv_buf_to_a53_t
{
  char inf[sizeof(struct dataRecv)];
  struct dataRecv recvbuf;
};
#elif USE_GELI == 1

typedef struct
{
  uint16_t controlWord;
  int32_t targetPosition;
  int16_t torque_offset;
} motor_out;

typedef struct
{
  uint16_t controlWord;
  int32_t targetPosition;
  int32_t targetVel;
  uint32_t acc;
  uint32_t dec;
  int8_t modeofOperation;

} weld_motor_out;

struct dataSend
{
#if AXIS > 0
  motor_out motorOut[AXIS];
#endif
#if USE_GELIIO
  uint8_t DO_GELI[MAX_DO_GELI];
#endif
#if WELD_AXIS > 0
  weld_motor_out motorOutWeld[WELD_AXIS];
#endif
#if USE_STM32F767
  uint16_t DO_767[MAX_DO_767];
  uint16_t AO_767[MAX_AO_767];
#endif
};

union send_buf_to_motor_t
{
  char inf[sizeof(struct dataSend)];
  struct dataSend sendbuf;
};

typedef struct
{
  uint16_t statusWord;
  int32_t actualPosition;
  uint16_t errorCode;
  int16_t actual_torque;
  int32_t actual_velocity;
  int16_t accumulated;
  int16_t warning;
} motor_in;

typedef struct
{
  uint16_t statusWord;
  uint16_t errorCode;
  int32_t actualPosition;
  int8_t modeofOperationDisplay;
  uint32_t limitState;
} weld_motor_in;

struct dataRecv
{
#if AXIS > 0
  motor_in motorIn[AXIS];
#endif
#if USE_GELIIO
  uint8_t DI_GELI[MAX_DI_GELI];
#endif
#if WELD_AXIS > 0
  weld_motor_in motorInWeld[WELD_AXIS];
#endif
#if USE_STM32F767
  uint16_t DI_767[MAX_DI_767];
  uint16_t AI_767[MAX_AI_767];
#endif
};

union recv_buf_to_a53_t
{
  char inf[sizeof(struct dataRecv)];
  struct dataRecv recvbuf;
};
#endif
#pragma pack(pop)

struct weld_opt
{

  weld_parameter_t *parameter;
  weld_system_t *system;
  weld_current_t *current;
  weld_gas_t *gas;
  weld_coollant_t *coollant;
  weld_arc_t *arc;
  weld_rot_t *rot;
  weld_wire_t *wire;
  weld_yaw_t *yaw;
  weld_angle_t *angle;
  weld_monitor_t *monitor;
  weld_joint_axis_t *joint;
  weld_position_control_t *control;
  weld_jog_t *jog;
  weld_kinematic_t *kinematic;
  ethercat_control_t *ethercat_control;
#if QUICK_COMMUNICATION_PROTOCOL
  uint8_t flag_rpmsg;
#endif
#if COMMON_COMMUNICATION_PROTOCOL
  SemaphoreHandle_t xMutex;
  SemaphoreHandle_t ETHMutex;

  union recv_buf_to_a53_t recv_buf_to_a53;
  union send_buf_to_motor_t send_buf_to_motor;
  bool rpmsg_handler_flag;
  int inOP;
#endif

#if QUICK_COMMUNICATION_PROTOCOL == 1
  union recv_buf_to_a53_t quick_recv_buf_to_a53;
  union send_buf_to_motor_t quick_send_buf_to_motor[QUEUE_SIZE];
  uint8_t queue_size;
  // 入队索引
  uint32_t write_index;
  // 出队索引
  uint32_t read_index;
  uint32_t Tpdo_size;
  uint32_t Rpdo_size;

#endif
};

weld_opt_t *weld_task_init();
void debug_for_weld(weld_opt_t *dev);
#endif
