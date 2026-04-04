#ifndef _NUTS_BOLTS_H_
#define _NUTS_BOLTS_H_
#include "stdint.h"
#include "stddef.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "stdbool.h"
#include <math.h>
#ifndef bit
#define bit(n) (1UL << (n))
#endif
#define RPMSG_PROTOCOL_MAX_LEN 26
#define cycleTime 4000000U
#define POSITION_ACCURACY 800 // 位置误差精度  +-100dec 以内认为到达位置
#define TRACK_VOL_ACCURACY 10 // 跟踪电压误差精度  +-100
#define MY_INT32_MAX 1073741823
/*IO点定义*/
#define MAX_DO_767 4
#define MAX_DI_767 5
#define MAX_AO_767 15
#define MAX_AI_767 6

#define MAX_DO_GELI 8
#define MAX_DI_GELI 12

#define SET_BIT_TURE(value, bit) ((value) = (value) | (1 << (bit)))
#define SET_BIT_FALSE(value, bit) ((value) = (value) & ~(1 << (bit)))

#define SAFE_ABS(x) ((x) < 0 ? -(x) : (x))

#define SAFE_POW(x) ((x) * (x))

#define CHECK_BIT_TURE(value, bit) (((value) & (1 << (bit))) != 0)
// 检测指定位置的位是否为 0
#define CHECK_BIT_FALSE(value, bit) (((value) & (1 << (bit))) == 0)
#define OPEN 1
#define CLOSE 0

#define CHECK_SECTION(index, max_index) (index == max_index ? 1 : 0)

/*------------调试用电机传动比--------------*/
#define rot_trans_per 1000.0f * 286.66f           // dec/r
#define wire_trans_per 131072.0f * 29.6f / 64.37f // dec/mm
#define arc_trans_per 40000.0f                    // dec/mm
#define yaw_trans_per 40000.0f                    // dec/mm
#define angle_trans_per 40000.0f                  // dec/mm
/*------------模拟量转换关系--------------*/
#define AO1_trans_per 65535.0f / 500.0f
#define AI2_trans_per 10000.0f / 60.0f
/*------------调试用上位机参数--------------*/
// 时间单位统一为 ms  速度单位统一为dec/s 长度单位统一为 dec
#define db_section_overall 3      // 总区间数
#define db_section_speed_change 5 // 换向区间

#define db_interval_rot_distance 30.0f * (M_PI / 180.0f) / (2.0f * M_PI) * rot_trans_per  // 度
#define db_interval_peak_rot_vel 110.0f / 60.0f / 44.5f / (2.0f * M_PI) * rot_trans_per   // mm/min  直径89mm 半径44.5mm
#define db_interval_base_rot_vel 110.0f / 60.0f / 44.5f / (2.0f * M_PI) * rot_trans_per   // mm/min  直径89mm 半径44.5mm
#define db_down_time 5.0f * 1000.0f                                                       // 秒
#define db_rot_down_vel 100.0f / 60.0f / 44.5f / (2.0f * M_PI) * rot_trans_per            // mm/min  直径89mm 半径44.5mm
#define db_rot_transition_vel 100.0f / 60.0f / 44.5f / (2.0f * M_PI) * rot_trans_per      // mm/min  直径89mm 半径44.5mm
#define db_rot_transition_distance 3.0f * (M_PI / 180.0f) / (2.0f * M_PI) * rot_trans_per // 度

#define db_interval_yaw_vel 800.0f / 60.0f * yaw_trans_per // mm/min
#define db_interval_yaw_distance 1.5f * yaw_trans_per      // mm
#define db_interval_yaw_forward_time 1000                  // 秒
#define db_interval_yaw_backward_time 1000                 // 秒
#define db_yaw_ahead_vel 300.0f / 60.0f * yaw_trans_per    // mm/min
#define db_yaw_ahead_distance 1.0f * yaw_trans_per         // mm
// #define db_yaw_ahead_start_position 3.0f * (M_PI / 180.0f) / (2.0f * M_PI) * rot_trans_per // 度
#define db_yaw_ahead_time 2000   // 秒
#define db_yaw_center_position 0 //

#define db_current_arc 15 * AO1_trans_per           // 电流模拟量
#define db_current_pre_time 2000                    // 秒
#define db_current_pre 40 * AO1_trans_per           // 电流模拟量
#define db_current_rise_time 100                    // 秒
#define db_interval_peak_current 80 * AO1_trans_per // 电流模拟量
#define db_interval_base_current 40 * AO1_trans_per // 电流模拟量
#define db_interval_peak_time 200                   // 秒
#define db_interval_base_time 400                   // 秒
#define db_current_final 20 * AO1_trans_per         // 电流模拟量

#define db_interval_peak_wire_vel 1000.0f / 60.0f * wire_trans_per                             // mm/min
#define db_interval_base_wire_vel 1000.0f / 60.0f * wire_trans_per                             // mm/min
#define db_wire_pre_vel 800.0f / 60.0f * wire_trans_per                                        // mm/min
#define db_wire_pre_time 100                                                                   // 秒
#define db_wire_extract_vel 1000.0f / 60.0f * wire_trans_per                                   // mm/min
#define db_wire_extract_time 300                                                               // 秒
#define db_wire_pre_start_distance 1.0f * (M_PI / 180.0f) / (2.0f * M_PI) * rot_trans_per      // 度
#define db_wire_extract_start_distance 89.5f * (M_PI / 180.0f) / (2.0f * M_PI) * rot_trans_per // 度

#define db_arc_forward_speed 80.0f / 60.0f * arc_trans_per   // mm/min
#define db_arc_backward_speed 200.0f / 60.0f * arc_trans_per // mm/min
#define db_arc_backward_distance 2.0f * arc_trans_per        // mm
#define db_arc_weld_lift_distance 4.0f * arc_trans_per       // mm
#define db_arc_mode 0                                        // 0高频 1提升

#define db_gas_pre_time 2000                // 秒
#define db_gas_delay_time 2000              // 秒
#define db_gas_alarm_time 600 * 1000        // 秒
#define db_coollant_alarm_time 600 * 1000   // 秒
#define db_interval_angle_position_0 10000  // dec
#define db_interval_angle_position_1 -10000 // dec
#define db_interval_angle_position_2 10000  // dec
#define db_interval_angle_position_3 -10000 // dec
#define db_interval_angle_position_4 10000  // dec

#define db_interval_peak_track_vol 8.0f * AI2_trans_per
#define db_interval_base_track_vol 8.0f * AI2_trans_per
#define db_track_mode 0
#define db_track_speed 70.0f / 60.0f * arc_trans_per

#define db_rot_go_home_vel 450.0f / 60.0f / 44.5f / (2.0f * M_PI) * rot_trans_per // mm/min  直径89mm 半径44.5mm

void detectRisingEdges(uint16_t currentState, volatile uint16_t *previousState, volatile uint16_t *flags);
uint32_t getCycle(uint32_t milliseconds);
uint32_t linearFunction(uint32_t start_x, uint32_t start_y, uint32_t end_x, uint32_t end_y, uint32_t current_x);
int32_t YawlinearFunction(int32_t start_x, int32_t start_y, int32_t end_x, int32_t end_y, int32_t current_x);
int32_t YawlinearFunction_back(int32_t start_x, int32_t start_y, int32_t end_x, int32_t end_y, int32_t current_x);
uint8_t plan_block_get_next_index(uint8_t current_index);
double get_unit_directional_vector(double origin_position, double target_position);
double get_remain_millimeters(double origin_position, double target_position);
#endif
