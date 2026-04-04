#ifndef _WELD_PARAMETER_H_
#define _WELD_PARAMETER_H_
#include "weld_sever/nuts_bolts.h"
#include "weld_sever/weld_system.h"
#define MAX_SECTION 32

typedef struct weld_opt weld_opt_t;
typedef struct weld_parameter weld_parameter_t;
typedef struct
{
  double tube_diameter;            // 管直径
  int32_t ec_slave_heart_beat_wkc; // 该数据禁止访问
  int8_t rot_interpolation_mode;
  uint16_t write_system_state;                      // 1000  0
  uint16_t weld_mode;                               // 1001  0
  uint32_t IO_max_current;                          // 1002  0
  uint16_t read_system_state;                       // 1003  0
  uint16_t section_overall;                         // 1200  0
  uint16_t section_rt;                              // 1200  1
  uint16_t section_rt_speed_change;                 // 1200  2
  int32_t interval_peak_current[MAX_SECTION];       // 0x1100-?  0
  int32_t interval_base_current[MAX_SECTION];       // 0x1100-?  1
  int32_t interval_peak_pluse_current[MAX_SECTION]; // 0x1100-?  7
  int32_t interval_peak_time[MAX_SECTION];          // 0x1100-?  2
  int32_t interval_base_time[MAX_SECTION];          // 0x1100-?  3
  int32_t interval_peak_pluse_time[MAX_SECTION];    // 0x1100-?  f
  int32_t interval_peak_track_vol[MAX_SECTION];     // 0x1100-?  4
  int32_t interval_base_track_vol[MAX_SECTION];     // 0x1100-?  5
  double interval_peak_rot_vel[MAX_SECTION];        // 0x1100-? d
  double interval_base_rot_vel[MAX_SECTION];        // 0x1100-? e
  double interval_rot_distance[MAX_SECTION];        // 0x1100-? f
  double interval_peak_wire_vel[MAX_SECTION];       // 0x1100-?
  double interval_base_wire_vel[MAX_SECTION];       // 0x1100-?
  int32_t interval_yaw_vel[MAX_SECTION];            // 0x1100-? 8
  int32_t interval_yaw_distance[MAX_SECTION];       // 0x1100-? 9
  int32_t interval_yaw_forward_time[MAX_SECTION];   // 0x1100-? a
  int32_t interval_yaw_backward_time[MAX_SECTION];  // 0x1100-? b
  int32_t interval_angle_position[MAX_SECTION];     // 0X1100 10
  int32_t interval_rot_direction[MAX_SECTION];      // 0x1100 11
  uint8_t interval_track_mode[MAX_SECTION];         // 0x1100 12 跟踪模式  默认为0 峰值跟踪
  uint16_t switch_sinal;                            // 100b 0
  uint16_t AO_767[MAX_AO_767];
  uint16_t DO_767[MAX_DO_767];
  uint16_t AI_767[MAX_AI_767];
  uint16_t DI_767[MAX_DI_767];
  uint8_t DO_GELI[MAX_DO_GELI];
  uint8_t DI_GELI[MAX_DI_GELI];
  uint32_t current_rise_time;   // 1203 3
  uint32_t down_time;           // 1203 4
  uint32_t current_pre;         // 1203 1
  uint32_t current_pre_time;    // 1203 2
  uint32_t gas_pre_time;        // 1202 0
  uint32_t gas_delay_time;      // 1202 1
  uint32_t gas_alarm_time;      // 1202 2
  uint32_t coollant_alarm_time; //?????
  double arc_forward_speed;     // 1203 6
  double arc_backward_speed;    // 1203 7
  double arc_backward_distance; // 1203 8
  double rot_down_vel;          // 1201 1
  double rot_down_distance;
  double track_speed;                 // 1205 2  跟踪速度
  uint32_t current_arc;               // 1203 5
  uint32_t arc_weld_lift_distance;    // 1203 9
  double wire_pre_vel;                // 1206 1
  uint32_t wire_pre_time;             // 1206 0
  double wire_extract_vel;            // 1206 3
  uint32_t wire_extract_time;         // 1206 4
  double wire_pre_start_distance;     // 1206 5
  double wire_extract_start_distance; // 1206 6
  uint32_t yaw_ahead_vel;             // 1207 0
  uint32_t yaw_ahead_distance;        // 1207 1
  uint32_t yaw_ahead_time;            // 1207 2
  int32_t yaw_center_positiopn;       // 1203 a
  int32_t angle_zero_position;        // 1203 c
  uint32_t alarm_state;               // 1011 0
  uint32_t arc_mode;                  // 1203 b
  uint32_t track_delay_time;

} weld_parameter_table_t;

typedef struct
{
  uint16_t base_index[RPMSG_PROTOCOL_MAX_LEN];
  uint8_t offect_index[RPMSG_PROTOCOL_MAX_LEN];
} protocol_block_t;

struct weld_parameter
{
  weld_parameter_table_t *table;
  uint8_t (*table_write)(weld_opt_t *dev, uint16_t Index1, uint8_t Index2, uint16_t len, double *data);
  uint8_t (*table_read)(weld_opt_t *dev, uint16_t Index1, uint8_t Index2, uint16_t len, double *data);
};

extern weld_parameter_t weld_parameter;

#endif
