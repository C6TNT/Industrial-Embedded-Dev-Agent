#ifndef MOTION_CONTROL_LIB_H
#define MOTION_CONTROL_LIB_H

#include "motion_control_lib_global.h"
#include "global.h"
extern "C"{
    int32_t OpenRpmsg(SYS_HANDLE *ihandle);
    int32_t BusCmd_InitBus(SYS_HANDLE *ihandle);
    int32_t Direct_SetAxisEnable(SYS_HANDLE *ihandle,int32_t iaxis,int32_t iValue);
    int32_t Direct_GetAxisEnable(SYS_HANDLE *ihandle,int32_t iaxis,int32_t *piValue);
    int32_t Direct_SetAType(SYS_HANDLE *ihandle,int32_t iaxis,int32_t iValue);
    int32_t Direct_GetAType(SYS_HANDLE *ihandle,int32_t iaxis,int32_t *piValue);
    int32_t Direct_GetUnits(SYS_HANDLE *ihandle,int32_t iaxis,int32_t *piValue);
    int32_t Direct_SetUnits(SYS_HANDLE *ihandle,int32_t iaxis,int32_t iValue);
    int32_t Direct_GetAccel(SYS_HANDLE *ihandle,int32_t iaxis,float *pfValue);
    int32_t Direct_SetAccel(SYS_HANDLE *ihandle,int32_t iaxis,float fValue);
    int32_t Direct_GetDecel(SYS_HANDLE *ihandle,int32_t iaxis,float *pfValue);
    int32_t Direct_SetDecel(SYS_HANDLE *ihandle,int32_t iaxis,float fValue);
    int32_t Direct_GetSpeed(SYS_HANDLE *ihandle,int32_t iaxis,float *pfValue);
    int32_t Direct_SetSpeed(SYS_HANDLE *ihandle,int32_t iaxis,float fValue);
    int32_t Direct_SetDpos(SYS_HANDLE *ihandle,int32_t iaxis,float fValue);
    int32_t Direct_GetDpos(SYS_HANDLE *ihandle,int32_t iaxis,float *pfValue);
    int32_t Direct_GetMpos(SYS_HANDLE *ihandle,int32_t iaxis,float *pfValue);
    int32_t Direct_GetEncoder(SYS_HANDLE *ihandle,int32_t iaxis,float *pfValue);
    int32_t Direct_GetAxisStatus(SYS_HANDLE *ihandle,int32_t iaxis,int32_t *piValue);
    int32_t Direct_GetModbusDpos(SYS_HANDLE *ihandle,int32_t imaxaxises,float *pfValue);
    int32_t Direct_GetModbusMpos(SYS_HANDLE *ihandle,int32_t imaxaxises,float *pfValue);
    int32_t Direct_GetDirection(SYS_HANDLE *ihandle,int32_t iaxis,float *pfValue);
    int32_t Direct_SetDirection(SYS_HANDLE *ihandle,int32_t iaxis,float fValue);
    int32_t Direct_Single_MoveAbs(SYS_HANDLE *ihandle,int32_t iaxis,float fValue);
    int32_t Direct_Single_Move(SYS_HANDLE *ihandle,int32_t iaxis,float fValue);
    int32_t Direct_GetIfIdle(SYS_HANDLE *ihandle,int32_t iaxis,int32_t *piValue);
    int32_t Direct_Single_Vmove(SYS_HANDLE *ihandle,int32_t iaxis,int32_t iValue);
    int32_t Direct_Single_Cancel(SYS_HANDLE *ihandle,int32_t iaxis,int32_t iValue);
    int32_t Direct_GetMtype(SYS_HANDLE *ihandle,int32_t iaxis,int32_t *piValue);
    int32_t Direct_MoveAbs(SYS_HANDLE *ihandle,int32_t iaxis,float fValue);
    int32_t Direct_Move(SYS_HANDLE *ihandle,int32_t iaxis,float fValue);
    int32_t Direct_Link_Move(SYS_HANDLE *ihandle,int32_t imaxaxises,int32_t *piAxislist,float *pfDisancelist);
    int32_t Direct_Link_MoveAbs(SYS_HANDLE *ihandle,int32_t imaxaxises,int32_t *piAxislist,float *pfDisancelist);


    int32_t Weld_Direct_SetAxisEnable(SYS_HANDLE *ihandle,int32_t iaxis,int32_t iValue);
    int32_t Weld_Direct_GetAxisEnable(SYS_HANDLE *ihandle,int32_t iaxis,int32_t *piValue);
    int32_t Weld_Direct_SetAType(SYS_HANDLE *ihandle,int32_t iaxis,int32_t iValue);
    int32_t Weld_Direct_GetAType(SYS_HANDLE *ihandle,int32_t iaxis,int32_t *piValue);
    int32_t Weld_Direct_SetUnits(SYS_HANDLE *ihandle,int32_t iaxis,int32_t iValue);
    int32_t Weld_Direct_GetUnits(SYS_HANDLE *ihandle,int32_t iaxis,int32_t *piValue);
    int32_t Weld_Direct_GetAccel(SYS_HANDLE *ihandle,int32_t iaxis,float *pfValue);
    int32_t Weld_Direct_SetAccel(SYS_HANDLE *ihandle,int32_t iaxis,float fValue);
    int32_t Weld_Direct_GetDecel(SYS_HANDLE *ihandle,int32_t iaxis,float *pfValue);
    int32_t Weld_Direct_SetDecel(SYS_HANDLE *ihandle,int32_t iaxis,float fValue);
    int32_t Weld_Direct_GetSpeed(SYS_HANDLE *ihandle,int32_t iaxis,float *pfValue);
    int32_t Weld_Direct_SetSpeed(SYS_HANDLE *ihandle,int32_t iaxis,float fValue);
    int32_t Weld_Direct_GetMpos(SYS_HANDLE *ihandle,int32_t iaxis,float *pfValue);
    int32_t Weld_Direct_GetModbusMpos(SYS_HANDLE *ihandle,int32_t imaxaxises,float *pfValue);
    int32_t Weld_Direct_SetDirection(SYS_HANDLE *ihandle,int32_t iaxis,float fValue);
    int32_t Weld_Direct_GetActualVel(SYS_HANDLE *ihandle,int32_t iaxis,int32_t *piValue);
    int32_t Weld_Direct_GetCommandTargetVel(SYS_HANDLE *ihandle,int32_t iaxis,int32_t *piValue);
    int32_t Weld_Direct_GetDriveTargetVel(SYS_HANDLE *ihandle,int32_t iaxis,int32_t *piValue);

    int32_t Weld_SetParameter_Interval_peak_current(SYS_HANDLE *ihandle,int32_t section,int32_t iValue);
    int32_t Weld_SetParameter_Interval_base_current(SYS_HANDLE *ihandle,int32_t section,int32_t iValue);
    int32_t Weld_SetParameter_Interval_peak_time(SYS_HANDLE *ihandle,int32_t section,int32_t iValue);
    int32_t Weld_SetParameter_Interval_base_time(SYS_HANDLE *ihandle,int32_t section,int32_t iValue);
    int32_t Weld_SetParameter_Interval_peak_track_vol(SYS_HANDLE *ihandle,int32_t section,int32_t iValue);
    int32_t Weld_SetParameter_Interval_base_track_vol(SYS_HANDLE *ihandle,int32_t section,int32_t iValue);
    int32_t Weld_SetParameter_Interval_peak_wire_vel(SYS_HANDLE *ihandle,int32_t section,int32_t iValue);
    int32_t Weld_SetParameter_Interval_peak_pluse_current(SYS_HANDLE *ihandle,int32_t section,int32_t iValue);
    int32_t Weld_SetParameter_Interval_yaw_vel(SYS_HANDLE *ihandle,int32_t section,int32_t iValue);
    int32_t Weld_SetParameter_Interval_yaw_distance(SYS_HANDLE *ihandle,int32_t section,int32_t iValue);
    int32_t Weld_SetParameter_Interval_yaw_forward_time(SYS_HANDLE *ihandle,int32_t section,int32_t iValue);
    int32_t Weld_SetParameter_Interval_yaw_backward_time(SYS_HANDLE *ihandle,int32_t section,int32_t iValue);
    int32_t Weld_SetParameter_Interval_peak_rot_vel(SYS_HANDLE *ihandle,int32_t section,int32_t iValue);
    int32_t Weld_SetParameter_Interval_base_rot_vel(SYS_HANDLE *ihandle,int32_t section,int32_t iValue);
    int32_t Weld_SetParameter_Interval_rot_distance(SYS_HANDLE *ihandle,int32_t section,int32_t iValue);
    int32_t Weld_SetParameter_Interval_base_wire_vel(SYS_HANDLE *ihandle,int32_t section,int32_t iValue);
    int32_t Weld_SetParameter_Interval_angle_position(SYS_HANDLE *ihandle,int32_t section,int32_t iValue);
    int32_t Weld_SetParameter_Interval_rot_direction(SYS_HANDLE *ihandle,int32_t section,int32_t iValue);
    int32_t Weld_SetParameter_Interval_track_mode(SYS_HANDLE *ihandle,int32_t section,int32_t iValue);
    int32_t Weld_SetParameter_section_overall(SYS_HANDLE *ihandle,int32_t iValue);
    int32_t Weld_SetParameter_rot_down_vel(SYS_HANDLE *ihandle,int32_t iValue);
    int32_t Weld_SetParameter_gas_pre_time(SYS_HANDLE *ihandle,int32_t iValue);
    int32_t Weld_SetParameter_gas_delay_time(SYS_HANDLE *ihandle,int32_t iValue);
    int32_t Weld_SetParameter_gas_alarm_time(SYS_HANDLE *ihandle,int32_t iValue);
    int32_t Weld_SetParameter_current_pre(SYS_HANDLE *ihandle,int32_t iValue);
    int32_t Weld_SetParameter_current_pre_time(SYS_HANDLE *ihandle,int32_t iValue);
    int32_t Weld_SetParameter_current_rise_time(SYS_HANDLE *ihandle,int32_t iValue);
    int32_t Weld_SetParameter_down_time(SYS_HANDLE *ihandle,int32_t iValue);
    int32_t Weld_SetParameter_current_arc(SYS_HANDLE *ihandle,int32_t iValue);
    int32_t Weld_SetParameter_arc_forward_speed(SYS_HANDLE *ihandle,int32_t iValue);
    int32_t Weld_SetParameter_arc_backward_speed(SYS_HANDLE *ihandle,int32_t iValue);
    int32_t Weld_SetParameter_arc_backward_distance(SYS_HANDLE *ihandle,int32_t iValue);
    int32_t Weld_SetParameter_arc_weld_lift_distance(SYS_HANDLE *ihandle,int32_t iValue);
    int32_t Weld_SetParameter_arc_mode(SYS_HANDLE *ihandle,int32_t iValue);
    int32_t Weld_SetParameter_angle_zero_position(SYS_HANDLE *ihandle,int32_t iValue);
    int32_t Weld_SetParameter_track_speed(SYS_HANDLE *ihandle,int32_t iValue);
    int32_t Weld_SetParameter_track_delay(SYS_HANDLE *ihandle,int32_t iValue);
    int32_t Weld_SetParameter_wire_pre_time(SYS_HANDLE *ihandle,int32_t iValue);
    int32_t Weld_SetParameter_wire_pre_vel(SYS_HANDLE *ihandle,int32_t iValue);
    int32_t Weld_SetParameter_wire_extract_vel(SYS_HANDLE *ihandle,int32_t iValue);
    int32_t Weld_SetParameter_wire_extract_time(SYS_HANDLE *ihandle,int32_t iValue);
    int32_t Weld_SetParameter_wire_pre_start_distance(SYS_HANDLE *ihandle,int32_t iValue);
    int32_t Weld_SetParameter_wire_extract_start_distance(SYS_HANDLE *ihandle,int32_t iValue);
    int32_t Weld_SetParameter_yaw_ahead_vel(SYS_HANDLE *ihandle,int32_t iValue);
    int32_t Weld_SetParameter_yaw_ahead_distance(SYS_HANDLE *ihandle,int32_t iValue);
    int32_t Weld_SetParameter_yaw_ahead_time(SYS_HANDLE *ihandle,int32_t iValue);
    int32_t Weld_SetParameter_coollant_alarm_time(SYS_HANDLE *ihandle,int32_t iValue);
    int32_t Weld_GetParameter_current_output_val(SYS_HANDLE *ihandle,int32_t *piValue);
    int32_t Weld_GetParameter_current_output_status(SYS_HANDLE *ihandle,int32_t *piValue);
    int32_t Weld_GetParameter_voltage_input_val(SYS_HANDLE *ihandle,int32_t *piValue);
    int32_t Weld_GetParameter_coollant_input_pluse(SYS_HANDLE *ihandle,int32_t *piValue);
    int32_t Weld_GetParameter_DI1(SYS_HANDLE *ihandle,int32_t *piValue);
    int32_t Weld_GetParameter_section_rt(SYS_HANDLE *ihandle,int32_t *piValue);
    int32_t Weld_GetSystem_read_status(SYS_HANDLE *ihandle,int32_t *piValue);
    int32_t Weld_SetSystem_write_status(SYS_HANDLE *ihandle,int32_t iValue);
    int32_t Weld_SetSystem_switch_signal(SYS_HANDLE *ihandle,int32_t iValue);
    int32_t Weld_GetSystem_alarm(SYS_HANDLE *ihandle,int32_t *piValue);


    int32_t Python_Init(SYS_HANDLE *ihandle);
    int32_t Python_SetDH(SYS_HANDLE *ihandle,int32_t axisNum,double joint_params[][3]);
    int32_t Python_GetMatrix(SYS_HANDLE *ihandle,int32_t axisNum,double *theta,double *matrix);
    int32_t Python_Kinematics(SYS_HANDLE *ihandle,int32_t axisNum,double* target_matrix, double* current_theta, double* solved_theta);
}
#endif // MOTION_CONTROL_LIB_H
