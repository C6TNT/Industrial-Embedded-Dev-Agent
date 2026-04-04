#ifndef MOTION_CONTROL_LIB_H
#define MOTION_CONTROL_LIB_H

#include "motion_control_lib_global.h"
#include "global.h"
extern "C"{
int32_t OpenRpmsg(SYS_HANDLE *ihandle);
int32_t BusCmd_InitBus(SYS_HANDLE *ihandle);

/*****************************  机械参数 *****************************/
int32_t Direct_SetAxisEnable(SYS_HANDLE *ihandle,int32_t iaxis,int32_t iValue);
int32_t Direct_GetAxisEnable(SYS_HANDLE *ihandle,int32_t iaxis,int32_t *piValue);
int32_t Direct_GetAType(SYS_HANDLE *ihandle,int32_t iaxis,int32_t *piValue);
int32_t Direct_SetAType(SYS_HANDLE *ihandle,int32_t iaxis,int32_t iValue);
int32_t Direct_SetSingleTransper(SYS_HANDLE *ihandle,int32_t iaxis,double fValue);
int32_t Direct_GetSingleTransper(SYS_HANDLE *ihandle,int32_t iaxis,double *pfValue);
int32_t Direct_SetAccel(SYS_HANDLE *ihandle,int32_t iaxis,double fValue);
int32_t Direct_GetAccel(SYS_HANDLE *ihandle,int32_t iaxis,double *pfValue);
int32_t Direct_GetDecel(SYS_HANDLE *ihandle,int32_t iaxis,double *pfValue);
int32_t Direct_SetDecel(SYS_HANDLE *ihandle,int32_t iaxis,double fValue);
int32_t Direct_GetEncoder(SYS_HANDLE *ihandle,int32_t iaxis,int32_t *piValue);
int32_t Direct_GetAxisStatus(SYS_HANDLE *ihandle,int32_t iaxis,int32_t *piValue);
int32_t Direct_SetDirection(SYS_HANDLE *ihandle,int32_t iaxis,int32_t iValue);
int32_t Direct_GetDirection(SYS_HANDLE *ihandle,int32_t iaxis,int32_t *piValue);
int32_t Direct_GetErrorCode(SYS_HANDLE *ihandle,int32_t iaxis,int32_t *piValue);
int32_t Direct_GetStatusCode(SYS_HANDLE *ihandle,int32_t iaxis,int32_t *piValue);
int32_t Direct_SetSingleDectime(SYS_HANDLE *ihandle,int32_t iaxis,double fValue);
int32_t Direct_SetSingleAcctime(SYS_HANDLE *ihandle,int32_t iaxis,double fValue);
int32_t Direct_SetSingleInitpos(SYS_HANDLE *ihandle,int32_t iaxis,int32_t iValue);
/*****************************  运动指令 *****************************/
int32_t Direct_SetSingleParam(SYS_HANDLE *ihandle,int32_t iaxis,double targetlength,double targetvel);

/*****************************  正逆解 *****************************/
int32_t Direct_SetMoveL_quat(SYS_HANDLE *ihandle,double* coordinate,double targetvel);
int32_t Direct_SetMoveL_matrix(SYS_HANDLE *ihandle,double* coordinate,double targetvel);
int32_t Direct_SetMoveL_RPY(SYS_HANDLE *ihandle,double* coordinate,double targetvel);
int32_t Direct_SetMoveJ_Joint_Angle(SYS_HANDLE *ihandle,double* coordinate,double targetvel);
int32_t Direct_SetToolCoordinateSystem_posMatrix(SYS_HANDLE *ihandle,double* coordinate);
int32_t Direct_GetToolCoordinateSystem_posMatrix(SYS_HANDLE *ihandle,double *pfValue);
int32_t Direct_GetCurrentpos_quat(SYS_HANDLE *ihandle,double *pfValue);
int32_t Direct_GetCurrentpos_matrix(SYS_HANDLE *ihandle,double *pfValue);
int32_t Direct_GetCurrentpos_RPY(SYS_HANDLE *ihandle,double *pfValue);
int32_t Direct_GetCurrentpos_Joint_Angle(SYS_HANDLE *ihandle,double *pfValue);
int32_t Direct_SetdhParam(SYS_HANDLE *ihandle,double* coordinate);
int32_t Direct_GetdhParam(SYS_HANDLE *ihandle,double *pfValue);
int32_t Direct_GetMoveLState(SYS_HANDLE *ihandle,int32_t *piValue);


/*****************************  焊接轴机械参数 *****************************/
int32_t Weld_Direct_SetAxisEnable(SYS_HANDLE *ihandle,int32_t iaxis,int32_t iValue);//使能 0-旋转 1-送丝 2-横摆 3-弧长 4-角摆
int32_t Weld_Direct_GetAxisEnable(SYS_HANDLE *ihandle,int32_t iaxis,int32_t *piValue);//获取使能状态
int32_t Weld_Direct_GetAType(SYS_HANDLE *ihandle,int32_t iaxis,int32_t *piValue);//
int32_t Weld_Direct_SetAType(SYS_HANDLE *ihandle,int32_t iaxis,int32_t iValue);//设置轴模式 66-8模式（位置） 65-9模式（速度）
int32_t Weld_Direct_SetSingleTransper(SYS_HANDLE *ihandle,int32_t iaxis,double fValue);//设置传动比
int32_t Weld_Direct_GetSingleTransper(SYS_HANDLE *ihandle,int32_t iaxis,double *pfValue);
int32_t Weld_Direct_SetAccel(SYS_HANDLE *ihandle,int32_t iaxis,double fValue);//加速度
int32_t Weld_Direct_GetAccel(SYS_HANDLE *ihandle,int32_t iaxis,double *pfValue);
int32_t Weld_Direct_GetDecel(SYS_HANDLE *ihandle,int32_t iaxis,double *pfValue);
int32_t Weld_Direct_SetDecel(SYS_HANDLE *ihandle,int32_t iaxis,double fValue);//减速度
int32_t Weld_Direct_GetEncoder(SYS_HANDLE *ihandle,int32_t iaxis,int32_t *piValue);//读取当前轴位置：dec
int32_t Weld_Direct_GetAxisStatus(SYS_HANDLE *ihandle,int32_t iaxis,int32_t *piValue);//获取轴运动状态
int32_t Weld_Direct_SetDirection(SYS_HANDLE *ihandle,int32_t iaxis,int32_t iValue);//方向
int32_t Weld_Direct_GetDirection(SYS_HANDLE *ihandle,int32_t iaxis,int32_t *piValue);
int32_t Weld_Direct_GetErrorCode(SYS_HANDLE *ihandle,int32_t iaxis,int32_t *piValue);//错误字
int32_t Weld_Direct_GetStatusCode(SYS_HANDLE *ihandle,int32_t iaxis,int32_t *piValue);//状态字
int32_t Weld_Direct_SetSingleDectime(SYS_HANDLE *ihandle,int32_t iaxis,double fValue);//减速时间
int32_t Weld_Direct_SetSingleAcctime(SYS_HANDLE *ihandle,int32_t iaxis,double fValue);//加速时间
int32_t Weld_Direct_SetSingleInitpos(SYS_HANDLE *ihandle,int32_t iaxis,int32_t iValue);//轴初始值


int32_t Weld_SetParameter_Interpolation_mode(SYS_HANDLE *ihandle, int32_t iValue);  //旋转轴运行模式（长直线/椭圆）
int32_t Weld_SetParameter_section_overall(SYS_HANDLE *ihandle, int32_t iValue);//总区间数
int32_t Weld_SetParameter_rot_down_distance(SYS_HANDLE *ihandle, double fValue);//衰减距离
int32_t Weld_GetParameter_section_rt(SYS_HANDLE *ihandle, int32_t *piValue);//当前区间数
int32_t Weld_SetParameter_rot_down_vel(SYS_HANDLE *ihandle, double fValue);//衰减速度
int32_t Weld_GetParameter_rot_down_vel(SYS_HANDLE *ihandle, double *pfValue);
int32_t Weld_SetSystem_write_system_state(SYS_HANDLE *ihandle,int32_t iValue);//边沿触发信号
int32_t Weld_SetSystem_switch_signal(SYS_HANDLE *ihandle,int64_t iValue);//开关量
int32_t Weld_SetParameter_arc_mode(SYS_HANDLE *ihandle,int32_t iValue);//起弧方式
int32_t Weld_SetParameter_arc_forward_speed(SYS_HANDLE *ihandle,double fValue,int32_t mode);//碰工件速度
int32_t Weld_SetParameter_arc_backward_speed(SYS_HANDLE *ihandle,double fValue,int32_t mode);//碰工件提升速度
int32_t Weld_SetParameter_arc_backward_distance(SYS_HANDLE *ihandle,double fValue,int32_t mode);//碰工件高度
int32_t Weld_SetParameter_track_speed(SYS_HANDLE *ihandle,double fValue);//跟踪速度
int32_t Weld_SetParameter_arc_weld_lift_distance(SYS_HANDLE *ihandle,double fValue);//焊后抬升高度
int32_t Weld_SetParameter_wire_pre_start_distance(SYS_HANDLE *ihandle,double fValue);//送丝开始相对位置
int32_t Weld_SetParameter_wire_pre_end_distance(SYS_HANDLE *ihandle,double fValue);//送丝结束相对位置
int32_t Weld_SetParameter_wire_pre_time(SYS_HANDLE *ihandle,double fValue);//提前送丝时间
int32_t Weld_SetParameter_wire_pre_vel(SYS_HANDLE *ihandle,double fValue);//提前送丝速度
int32_t Weld_SetParameter_wire_extract_time(SYS_HANDLE *ihandle,double fValue);//抽丝时间
int32_t Weld_SetParameter_wire_extract_vel(SYS_HANDLE *ihandle,double fValue);//抽丝速度
int32_t Weld_SetParameter_current_pre_time(SYS_HANDLE *ihandle,double fValue);//预熔时间
int32_t Weld_SetParameter_current_pre(SYS_HANDLE *ihandle,double fValue);//预熔电流
int32_t Weld_SetParameter_current_rise_time(SYS_HANDLE *ihandle,double fValue);//爬升时间
int32_t Weld_SetParameter_current_arc(SYS_HANDLE *ihandle,double fValue,int32_t mode);//起弧电流
int32_t Weld_SetParameter_gas_pre_time(SYS_HANDLE *ihandle,double fValue);//提前送气时间
int32_t Weld_SetParameter_gas_delay_time(SYS_HANDLE *ihandle,double fValue);//滞后送气时间
int32_t Weld_SetParameter_track_delay(SYS_HANDLE *ihandle,double fValue);//跟踪延时
/*****************************  焊接轴区间参数 *****************************/

int32_t Weld_SetParameter_Interval_peak_time(SYS_HANDLE *ihandle,int32_t section, double fValue);//区间峰值电流持续时间
int32_t Weld_SetParameter_Interval_base_time(SYS_HANDLE *ihandle,int32_t section,double fValue);//区间基值电流持续时间
int32_t Weld_SetParameter_Interval_peak_current(SYS_HANDLE *ihandle,int32_t section,double fValue);//区间峰值电流
int32_t Weld_SetParameter_Interval_base_current(SYS_HANDLE *ihandle,int32_t section,double fValue);//区间基值电流
int32_t Weld_SetParameter_Interval_peak_track_vol(SYS_HANDLE *ihandle,int32_t section,double fValue);//区间峰值跟踪电压
int32_t Weld_SetParameter_Interval_base_track_vol(SYS_HANDLE *ihandle,int32_t section,double fValue);//区间基值跟踪电压
int32_t Weld_SetParameter_Interval_peak_wire_vel(SYS_HANDLE *ihandle,int32_t section,double fValue);//区间峰值送丝速度
int32_t Weld_SetParameter_Interval_peak_pluse_current(SYS_HANDLE *ihandle,int32_t section,double fValue);//区间叠加尖脉冲电流
int32_t Weld_SetParameter_Interval_yaw_vel(SYS_HANDLE *ihandle,int32_t section,double fValue);//横摆速度
int32_t Weld_SetParameter_Interval_yaw_distance(SYS_HANDLE *ihandle,int32_t section,double fValue);//横摆宽度
int32_t Weld_SetParameter_Interval_yaw_forward_time(SYS_HANDLE *ihandle,int32_t section,double fValue);//区间左边缘停留时间
int32_t Weld_SetParameter_Interval_yaw_backward_time(SYS_HANDLE *ihandle,int32_t section,double fValue);//区间右边缘停留时间
int32_t Weld_SetParameter_Interval_peak_rot_vel(SYS_HANDLE *ihandle,int32_t section, double fValue);//峰值阶段旋转速度
int32_t Weld_SetParameter_Interval_base_rot_vel(SYS_HANDLE *ihandle,int32_t section, double fValue);//基值阶段旋转速度
int32_t Weld_SetParameter_Interval_rot_distance(SYS_HANDLE *ihandle,int32_t section,double fValue);//区间旋转距离
int32_t Weld_SetParameter_Interval_base_wire_vel(SYS_HANDLE *ihandle,int32_t section,double fValue);//基值送丝速度
int32_t Weld_SetParameter_Interval_angle_position(SYS_HANDLE *ihandle,int32_t section,double fValue);//区间角度位置
int32_t Weld_SetParameter_Interval_rot_direction(SYS_HANDLE *ihandle,int32_t section,double fValue);//区间旋转方向
int32_t Weld_SetParameter_Interval_track_mode(SYS_HANDLE *ihandle,int32_t section,double fValue);//区间跟踪模式


/*****************************  IO模块 *****************************/
int32_t Weld_SetParameter_AO(SYS_HANDLE *ihandle,int32_t section,uint16_t iValue); //设置模拟量输出
int32_t Weld_GetParameter_AI(SYS_HANDLE *ihandle,int32_t section,uint16_t *piValue);
int32_t Weld_SetParameter_DO(SYS_HANDLE *ihandle,int32_t section,uint16_t iValue);// 数字量
int32_t Weld_GetParameter_DI(SYS_HANDLE *ihandle,int32_t section,uint16_t *piValue);
int32_t Weld_SetParameter_DO_GELI(SYS_HANDLE *ihandle,int32_t section,uint8_t iValue);
int32_t Weld_GetParameter_DI_GELI(SYS_HANDLE *ihandle,int32_t section,uint8_t *piValue);
/*****************************  焊接运动指令 *****************************/
int32_t Weld_Direct_SetTargetVel(SYS_HANDLE *ihandle,int32_t iaxis,double fValue);
int32_t Weld_Direct_SetSingleParam(SYS_HANDLE *ihandle,int32_t iaxis,double targetlength,double targetvel);
int32_t Weld_Direct_GetActualVel(SYS_HANDLE *ihandle,int32_t iaxis,int32_t *piValue);
int32_t Weld_Direct_GetCommandTargetVel(SYS_HANDLE *ihandle,int32_t iaxis,int32_t *piValue);
int32_t Weld_Direct_GetDriveTargetVel(SYS_HANDLE *ihandle,int32_t iaxis,int32_t *piValue);
int32_t Weld_Direct_GetDriveMode(SYS_HANDLE *ihandle,int32_t iaxis,int32_t *piValue);
int32_t Weld_Direct_GetDriveDisplay(SYS_HANDLE *ihandle,int32_t iaxis,int32_t *piValue);
int32_t Weld_Direct_GetDriveControlWord(SYS_HANDLE *ihandle,int32_t iaxis,int32_t *piValue);
int32_t Weld_Direct_GetDriveStatusWord(SYS_HANDLE *ihandle,int32_t iaxis,int32_t *piValue);

}
#endif // MOTION_CONTROL_LIB_H
