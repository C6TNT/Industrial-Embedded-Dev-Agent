#include "rpmsg.h"
#include "error.h"
#include "global.h"
#include <stdio.h>
#include "string.h"

#define MAX_SECTION 32
#define RPMSG_WAIT_RETRY_COUNT 200

static int32_t wait_for_expected_frame(union rpmsg_single_frame *response,
                                       uint8_t expectedType,
                                       uint16_t expectedBaseIndex,
                                       uint16_t expectedOffectIndex)
{
    for (int retry = 0; retry < RPMSG_WAIT_RETRY_COUNT; ++retry)
    {
        int32_t ret = rpmsg_read(response->inf, sizeof(response->inf));
        if (ret == 1)
        {
            if ((response->frame.type == expectedType) &&
                (response->frame.baseIndex == expectedBaseIndex) &&
                (response->frame.offectIndex == expectedOffectIndex))
            {
                return 1;
            }

            printf("rpmsg response mismatch type=%u base=0x%X off=0x%X expected type=%u base=0x%X off=0x%X\n",
                   response->frame.type,
                   response->frame.baseIndex,
                   response->frame.offectIndex,
                   expectedType,
                   expectedBaseIndex,
                   expectedOffectIndex);
        }
        else if (ret == -1)
        {
            return -1;
        }
        else if (ret == -2)
        {
            return -2;
        }

        usleep(1000);
    }

    return -2;
}

int32_t parameter_set(SYS_HANDLE *ihandle,uint16_t Index1,uint16_t Index2,uint8_t len,double *data,int32_t errorcode){
    int32_t ret;
    union rpmsg_single_frame single_rpmsg_read = {{0}};
    union rpmsg_single_frame single_rpmsg_write = {{0}};
    single_rpmsg_write.frame.type = 6;
    single_rpmsg_write.frame.baseIndex = Index1;
    single_rpmsg_write.frame.offectIndex = Index2;
    single_rpmsg_write.frame.len = 32;
    for(int i = 0; i < len; i++) {
        single_rpmsg_write.frame.data[i] = data[i];
    }
    ret = rpmsg_write(single_rpmsg_write.inf, sizeof(single_rpmsg_write.inf));
    if(ret == -1){
        printf("parameter_set write failed base=0x%X off=0x%X\n", Index1, Index2);
        return WriteRpmsg_FailedWrite;
    }
    ret = wait_for_expected_frame(&single_rpmsg_read,
                                  single_rpmsg_write.frame.type,
                                  Index1,
                                  Index2);
    if(ret == -1){
        printf("parameter_set read failed base=0x%X off=0x%X\n", Index1, Index2);
        return ReadRpmsg_FailedRead;
    }
    else if(ret == -2){
        printf("parameter_set read timeout base=0x%X off=0x%X\n", Index1, Index2);
        return ReadRpmsg_ReadTimeOut;
    }
    else if(single_rpmsg_read.frame.returnValue == -1){
        printf("parameter_set rejected base=0x%X off=0x%X len=%u first=%.3f returnValue=%d\n",
               Index1,
               Index2,
               len,
               data[0],
               single_rpmsg_read.frame.returnValue);
        return errorcode;
    }
    return Status_Ok;
}

int32_t parameter_get(SYS_HANDLE *ihandle,uint16_t Index1,uint16_t Index2,uint8_t len,double *data,int32_t errorcode){
    int32_t ret;
    union rpmsg_single_frame single_rpmsg_read = {{0}};
    union rpmsg_single_frame single_rpmsg_write = {{0}};
    single_rpmsg_write.frame.type = 5;
    single_rpmsg_write.frame.baseIndex = Index1;
    single_rpmsg_write.frame.offectIndex = Index2;
    single_rpmsg_write.frame.len = 32;
    ret = rpmsg_write(single_rpmsg_write.inf, sizeof(single_rpmsg_write.inf));
    if(ret == -1)return WriteRpmsg_FailedWrite;
    ret = wait_for_expected_frame(&single_rpmsg_read,
                                  single_rpmsg_write.frame.type,
                                  Index1,
                                  Index2);
    if(ret == -1)return ReadRpmsg_FailedRead;
    else if(ret == -2)return ReadRpmsg_ReadTimeOut;
    else if(single_rpmsg_read.frame.returnValue == -1)return errorcode;
    for(int i = 0; i < len; i++) {
        data[i] = single_rpmsg_read.frame.data[i];
    }
    return Status_Ok;
}

int32_t BusCmd_InitBus(SYS_HANDLE *ihandle)
{
    int32_t ret;

    union rpmsg_single_frame single_rpmsg_read = {{0}};
    union rpmsg_single_frame single_rpmsg_write = {{0}};
    single_rpmsg_write.frame.type = 6;
    single_rpmsg_write.frame.baseIndex = 0x500;
    single_rpmsg_write.frame.offectIndex = 0x0;
    single_rpmsg_write.frame.len = 32;
    single_rpmsg_write.frame.data[0] = 0;
    ret = rpmsg_write(single_rpmsg_write.inf, sizeof(single_rpmsg_write.inf));
    if (ret == -1)
        return WriteRpmsg_FailedWrite;

    ret = wait_for_expected_frame(&single_rpmsg_read,
                                  single_rpmsg_write.frame.type,
                                  single_rpmsg_write.frame.baseIndex,
                                  single_rpmsg_write.frame.offectIndex);
    if (ret == -1)
        return ReadRpmsg_FailedRead;
    else if (ret == -2)
        return ReadRpmsg_ReadTimeOut;
    else if (single_rpmsg_read.frame.returnValue == -1)
        return InitBus_InitFailed;
    ihandle->InitBusEnable = 1;
    return Status_Ok;
}

int32_t OpenRpmsg(SYS_HANDLE *ihandle)
{
    int32_t ret;
    ret = rpmsg_init();
    ret = rpmsg_clear();
    if (ret == -1)
        return OpenRpmsg_FailedOpen;
    ihandle->InitBusEnable = 0;

    return Status_Ok;
}

/*****************************  机械参数 *****************************/

int32_t Direct_SetAxisEnable(SYS_HANDLE *ihandle, int32_t iaxis, int32_t iValue)
{
    int32_t ret;
    double data[1];

    if (iValue)
    {
        data[0] = 0x86;
        ret = parameter_set(ihandle,(iaxis + 0x4005),0x6,1,data,Direct_SetAxisEnableFailed);
        usleep(10000);
        data[0] = 0x6;
        ret = parameter_set(ihandle,(iaxis + 0x4005),0x6,1,data,Direct_SetAxisEnableFailed);
        usleep(10000);
        data[0] = 0x7;
        ret = parameter_set(ihandle,(iaxis + 0x4005),0x6,1,data,Direct_SetAxisEnableFailed);
        usleep(10000);
        data[0] = 0xf;
        ret = parameter_set(ihandle,(iaxis + 0x4005),0x6,1,data,Direct_SetAxisEnableFailed);
    }
    else
    {

        data[0] = 0x86;
        ret = parameter_set(ihandle,(iaxis + 0x4005),0x6,1,data,Direct_SetAxisEnableFailed);
        usleep(10000);
        data[0] = 0x6;
        ret = parameter_set(ihandle,(iaxis + 0x4005),0x6,1,data,Direct_SetAxisEnableFailed);
    }
    return ret;
}

int32_t Direct_GetAxisEnable(SYS_HANDLE *ihandle, int32_t iaxis, int32_t *piValue)
{
    int32_t ret;
    double data[1];
    ret = parameter_get(ihandle,(iaxis + 0x4005),0x6,1,data,Direct_GetAxisEnableFailed);
    *piValue = data[0];
    return ret;
}

int32_t Direct_GetAType(SYS_HANDLE *ihandle, int32_t iaxis, int32_t *piValue)
{
    int32_t ret;
    double data[1];
    ret = parameter_get(ihandle,(iaxis + 0x4005),0x7,1,data,Direct_GetAxisEnableFailed);
    *piValue = data[0];
    return ret;
}

int32_t Direct_SetAType(SYS_HANDLE *ihandle, int32_t iaxis, int32_t iValue)
{
    int32_t ret;
    double data[1];
    printf("AType : %d \r\n",iValue);
    switch (iValue)
    {
    case 65:
        data[0] = 8;
        ret = parameter_set(ihandle,(iaxis + 0x4005),0x7,1,data,Direct_SetAxisEnableFailed);
        break;
    case 66:
        data[0] = 9;
        ret = parameter_set(ihandle,(iaxis + 0x4005),0x7,1,data,Direct_SetAxisEnableFailed);
        break;
    }
    return ret;
}

int32_t Direct_SetSingleTransper(SYS_HANDLE *ihandle, int32_t iaxis, double fValue)
{
    return parameter_set(ihandle,(iaxis + 0x4005),0xb,1,&fValue,Direct_SetTransperFailed);
}

int32_t Direct_GetSingleTransper(SYS_HANDLE *ihandle, int32_t iaxis, double *pfValue)
{
    return parameter_get(ihandle,(iaxis + 0x4005),0xb,1,pfValue,Direct_SetTransperFailed);
}

int32_t Direct_SetAccel(SYS_HANDLE *ihandle, int32_t iaxis, double fValue)
{
    return parameter_set(ihandle,(iaxis + 0x4005),0x8,1,&fValue,Direct_SetAccelFailed);
}

int32_t Direct_GetAccel(SYS_HANDLE *ihandle, int32_t iaxis, double *pfValue)
{
    return parameter_get(ihandle,(iaxis + 0x4005),0x8,1,pfValue,Direct_GetAccelFailed);
}

int32_t Direct_GetDecel(SYS_HANDLE *ihandle, int32_t iaxis, double *pfValue)
{
    return parameter_get(ihandle,(iaxis + 0x4005),0x9,1,pfValue,Direct_GetDecelFailed);
}

int32_t Direct_SetDecel(SYS_HANDLE *ihandle, int32_t iaxis, double fValue)
{
    return parameter_set(ihandle,(iaxis + 0x4005),0x9,1,&fValue,Direct_SetDecelFailed);
}

int32_t Direct_GetEncoder(SYS_HANDLE *ihandle, int32_t iaxis, int32_t *piValue)
{
    int32_t ret;
    double data[1];
    ret = parameter_get(ihandle,(iaxis + 0x4005),0x0,1,data,Direct_GetEncoderFailed);
    *piValue = data[0];
    return ret;
}

int32_t Direct_GetAxisStatus(SYS_HANDLE *ihandle, int32_t iaxis, int32_t *piValue)
{
    int32_t ret;
    double data[1];
    ret = parameter_get(ihandle,(iaxis + 0x4005),0x4,1,data,Direct_GetAxisStatusFailed);
    *piValue = data[0];
    return ret;
}

int32_t Direct_SetDirection(SYS_HANDLE *ihandle, int32_t iaxis, int32_t iValue)
{
    int32_t ret;
    double data[1];
    data[0] = iValue;
    ret = parameter_set(ihandle,(iaxis + 0x4005),0xa,1,data,Direct_SetDirectionFailed);
    return ret;
}

int32_t Direct_GetDirection(SYS_HANDLE *ihandle, int32_t iaxis, int32_t *piValue)
{

    int32_t ret;
    double data[1];
    ret = parameter_get(ihandle,(iaxis + 0x4005),0xa,1,data,Direct_GetDirectionFailed);
    *piValue = data[0];
    return ret;
}

int32_t Direct_GetErrorCode(SYS_HANDLE *ihandle, int32_t iaxis, int32_t *piValue)
{

    int32_t ret;
    double data[1];
    ret = parameter_get(ihandle,(iaxis + 0x4005),0x3,1,data,Direct_GetErrorCodeFailed);
    *piValue = data[0];
    return ret;
}

int32_t Direct_GetStatusCode(SYS_HANDLE *ihandle, int32_t iaxis, int32_t *piValue)
{
    int32_t ret;
    double data[1];
    ret = parameter_get(ihandle,(iaxis + 0x4005),0x2,1,data,Direct_GetStatusCodeFailed);
    *piValue = data[0];
    return ret;
}

int32_t Direct_SetSingleDectime(SYS_HANDLE *ihandle, int32_t iaxis, double fValue)
{
    return parameter_set(ihandle,(iaxis + 0x4005),0xd,1,&fValue,Direct_SetDectimeFailed);
}

int32_t Direct_SetSingleAcctime(SYS_HANDLE *ihandle, int32_t iaxis, double fValue)
{
    return parameter_set(ihandle,(iaxis + 0x4005),0xc,1,&fValue,Direct_SetAcceltimeFailed);
}

int32_t Direct_SetSingleInitpos(SYS_HANDLE *ihandle, int32_t iaxis, int32_t iValue)
{
    int32_t ret;
    double data[1];
    data[0] = iValue;
    ret = parameter_set(ihandle,(iaxis + 0x4005),0xe,1,data,Direct_SetInitposFailed);
    return ret;
}

int32_t Direct_SetSingleParam(SYS_HANDLE *ihandle, int32_t iaxis, double targetlength, double targetvel)
{
    double data[2] = {targetlength,targetvel};
    return parameter_set(ihandle,(iaxis + 0x4005),0x10,2,data,Direct_SetSingleParamFailed);
}

/*****************************  运动指令 *****************************/

int32_t Direct_SetMoveL_quat(SYS_HANDLE *ihandle, double *coordinate, double targetvel)
{
    double data[8] = {coordinate[0],coordinate[1],coordinate[2],coordinate[3],coordinate[4],coordinate[5],coordinate[6],targetvel};
    return parameter_set(ihandle,0x5000,0x0,8,data,Direct_SetTargetpos_quatFailed);
}

int32_t Direct_SetMoveL_matrix(SYS_HANDLE *ihandle, double *coordinate, double targetvel)
{
    double data[13] = {coordinate[0],coordinate[1],coordinate[2],coordinate[3],coordinate[4],coordinate[5],coordinate[6],coordinate[7],coordinate[8],coordinate[9],coordinate[10],coordinate[11],targetvel};
    return parameter_set(ihandle,0x5000,0x1,13,data,Direct_SetTargetpos_matrixFailed);
}

int32_t Direct_SetMoveL_RPY(SYS_HANDLE *ihandle, double *coordinate, double targetvel)
{
    double data[7] = {coordinate[0],coordinate[1],coordinate[2],coordinate[3],coordinate[4],coordinate[5],targetvel};
    return parameter_set(ihandle,0x5000,0x2,7,data,Direct_SetTargetpos_RPYFailed);
}

int32_t Direct_SetMoveJ_Joint_Angle(SYS_HANDLE *ihandle, double *coordinate, double targetvel)
{
    double data[7] = {coordinate[0],coordinate[1],coordinate[2],coordinate[3],coordinate[4],coordinate[5],targetvel};
    return parameter_set(ihandle,0x5000,0x3,7,data,Direct_SetTargetpos_JointFailed);
}

int32_t Direct_SetToolCoordinateSystem_posMatrix(SYS_HANDLE *ihandle, double *coordinate)
{
    double data[3] = {coordinate[0],coordinate[1],coordinate[2]};
    return parameter_set(ihandle,0x5000,0x4,3,data,Direct_SetTargetpos_ToolFailed);
}

int32_t Direct_GetToolCoordinateSystem_posMatrix(SYS_HANDLE *ihandle, double *pfValue)
{
    return parameter_get(ihandle,0x5000,0x4,3,pfValue,Direct_GetTargetpos_ToolFailed);
}

int32_t Direct_SetdhParam(SYS_HANDLE *ihandle, double *coordinate)
{
    double data[4] = {coordinate[0],coordinate[1],coordinate[2],coordinate[3]};
    return parameter_set(ihandle,0x5000,0x5,4,data,Direct_SetdhParamFailed);
}

int32_t Direct_GetdhParam(SYS_HANDLE *ihandle, double *pfValue)
{
    return parameter_get(ihandle,0x5000,0x5,4,pfValue,Direct_GetdhParamFailed);
}

int32_t Direct_GetMoveLState(SYS_HANDLE *ihandle, int32_t *piValue)
{
    int32_t ret;
    double data[1];
    ret = parameter_get(ihandle,0x5000,0x6,1,data,Direct_GetMovelStateFailed);
    *piValue = data[0];
    return ret;
}

int32_t Direct_GetCurrentpos_quat(SYS_HANDLE *ihandle, double *pfValue)
{
    return parameter_get(ihandle,0x5000,0x7,7,pfValue,Direct_GetTargetpos_quatFailed);
}

int32_t Direct_GetCurrentpos_matrix(SYS_HANDLE *ihandle, double *pfValue)
{
    return parameter_get(ihandle,0x5000,0x8,12,pfValue,Direct_GetTargetpos_matrixFailed);
}

int32_t Direct_GetCurrentpos_RPY(SYS_HANDLE *ihandle, double *pfValue)
{
    return parameter_get(ihandle,0x5000,0x9,6,pfValue,Direct_GetTargetpos_RPYFailed);
}

int32_t Direct_GetCurrentpos_Joint_Angle(SYS_HANDLE *ihandle, double *pfValue)
{
    return parameter_get(ihandle,0x5000,0xa,6,pfValue,Direct_GetTargetpos_JointFailed);
}


/*****************************  焊接机械参数 *****************************/

int32_t Weld_Direct_SetAxisEnable(SYS_HANDLE *ihandle, int32_t iaxis, int32_t iValue)
{
    int32_t ret;
    double data[1];

    if (iValue)
    {
        data[0] = 0x86;
        ret = parameter_set(ihandle,(iaxis + 0x4000),0x6,1,data,Direct_SetAxisEnableFailed);
        usleep(10000);
        data[0] = 0x6;
        ret = parameter_set(ihandle,(iaxis + 0x4000),0x6,1,data,Direct_SetAxisEnableFailed);
        usleep(10000);
        data[0] = 0x7;
        ret = parameter_set(ihandle,(iaxis + 0x4000),0x6,1,data,Direct_SetAxisEnableFailed);
        usleep(10000);
        data[0] = 0xf;
        ret = parameter_set(ihandle,(iaxis + 0x4000),0x6,1,data,Direct_SetAxisEnableFailed);
    }
    else
    {
        data[0] = 0x86;
        ret = parameter_set(ihandle,(iaxis + 0x4000),0x6,1,data,Direct_SetAxisEnableFailed);
        usleep(10000);
        data[0] = 0x6;
        ret = parameter_set(ihandle,(iaxis + 0x4000),0x6,1,data,Direct_SetAxisEnableFailed);
    }
    return ret;
}

int32_t Weld_Direct_GetAxisEnable(SYS_HANDLE *ihandle, int32_t iaxis, int32_t *piValue)
{
    int32_t ret;
    double data[1];
    ret = parameter_get(ihandle,(iaxis + 0x4000),0x6,1,data,Direct_GetAxisEnableFailed);
    *piValue = data[0];
    return ret;
}

int32_t Weld_Direct_GetAType(SYS_HANDLE *ihandle, int32_t iaxis, int32_t *piValue)
{
    int32_t ret;
    double data[1];
    ret = parameter_get(ihandle,(iaxis + 0x4000),0x7,1,data,Direct_GetAxisEnableFailed);
    *piValue = data[0];
    return ret;
}

int32_t Weld_Direct_SetAType(SYS_HANDLE *ihandle, int32_t iaxis, int32_t iValue)
{
    int32_t ret;
    double data[1];
    switch (iValue)
    {
    case 3:
        data[0] = 3;
        ret = parameter_set(ihandle,(iaxis + 0x4000),0x7,1,data,Direct_SetAxisEnableFailed);
        break;
    case 65:
        data[0] = 8;
        ret = parameter_set(ihandle,(iaxis + 0x4000),0x7,1,data,Direct_SetAxisEnableFailed);
        break;
    case 66:
        data[0] = 9;
        ret = parameter_set(ihandle,(iaxis + 0x4000),0x7,1,data,Direct_SetAxisEnableFailed);
        break;
    default:
        data[0] = iValue;
        ret = parameter_set(ihandle,(iaxis + 0x4000),0x7,1,data,Direct_SetAxisEnableFailed);
        break;
    }
    return ret;
}

int32_t Weld_Direct_SetSingleTransper(SYS_HANDLE *ihandle, int32_t iaxis, double fValue)
{
    return parameter_set(ihandle,(iaxis + 0x4000),0xb,1,&fValue,Direct_SetTransperFailed);
}

int32_t Weld_Direct_GetSingleTransper(SYS_HANDLE *ihandle, int32_t iaxis, double *pfValue)
{
    return parameter_get(ihandle,(iaxis + 0x4000),0xb,1,pfValue,Direct_SetTransperFailed);
}

int32_t Weld_Direct_SetAccel(SYS_HANDLE *ihandle, int32_t iaxis, double fValue)
{
    return parameter_set(ihandle,(iaxis + 0x4000),0x8,1,&fValue,Direct_SetAccelFailed);
}

int32_t Weld_Direct_GetAccel(SYS_HANDLE *ihandle, int32_t iaxis, double *pfValue)
{
    return parameter_get(ihandle,(iaxis + 0x4000),0x8,1,pfValue,Direct_GetAccelFailed);
}

int32_t Weld_Direct_GetDecel(SYS_HANDLE *ihandle, int32_t iaxis, double *pfValue)
{
    return parameter_get(ihandle,(iaxis + 0x4000),0x9,1,pfValue,Direct_GetDecelFailed);
}

int32_t Weld_Direct_SetDecel(SYS_HANDLE *ihandle, int32_t iaxis, double fValue)
{
    return parameter_set(ihandle,(iaxis + 0x4000),0x9,1,&fValue,Direct_SetDecelFailed);
}

int32_t Weld_Direct_GetEncoder(SYS_HANDLE *ihandle, int32_t iaxis, int32_t *piValue)
{
    int32_t ret;
    double data[1];
    ret = parameter_get(ihandle,(iaxis + 0x4000),0x0,1,data,Direct_GetEncoderFailed);
    *piValue = data[0];
    return ret;
}

int32_t Weld_Direct_GetActualVel(SYS_HANDLE *ihandle, int32_t iaxis, int32_t *piValue)
{
    int32_t ret;
    double data[1];
    ret = parameter_get(ihandle,(iaxis + 0x4000),0x10,1,data,Weld_Direct_GetActualVelFailed);
    *piValue = data[0];
    return ret;
}

int32_t Weld_Direct_GetAxisStatus(SYS_HANDLE *ihandle, int32_t iaxis, int32_t *piValue)
{
    int32_t ret;
    double data[1];
    ret = parameter_get(ihandle,(iaxis + 0x4000),0x4,1,data,Direct_GetAxisStatusFailed);
    *piValue = data[0];
    return ret;
}

int32_t Weld_Direct_SetDirection(SYS_HANDLE *ihandle, int32_t iaxis, int32_t iValue)
{
    int32_t ret;
    double data[1];
    data[0] = iValue;
    ret = parameter_set(ihandle,(iaxis + 0x4000),0xa,1,data,Direct_SetDirectionFailed);
    return ret;
}

int32_t Weld_Direct_GetDirection(SYS_HANDLE *ihandle, int32_t iaxis, int32_t *piValue)
{

    int32_t ret;
    double data[1];
    ret = parameter_get(ihandle,(iaxis + 0x4000),0xa,1,data,Direct_GetDirectionFailed);
    *piValue = data[0];
    return ret;
}

int32_t Weld_Direct_GetErrorCode(SYS_HANDLE *ihandle, int32_t iaxis, int32_t *piValue)
{

    int32_t ret;
    double data[1];
    ret = parameter_get(ihandle,(iaxis + 0x4000),0x3,1,data,Direct_GetErrorCodeFailed);
    *piValue = data[0];
    return ret;
}

int32_t Weld_Direct_GetStatusCode(SYS_HANDLE *ihandle, int32_t iaxis, int32_t *piValue)
{
    int32_t ret;
    double data[1];
    ret = parameter_get(ihandle,(iaxis + 0x4000),0x2,1,data,Direct_GetStatusCodeFailed);
    *piValue = data[0];
    return ret;
}

int32_t Weld_Direct_GetCommandTargetVel(SYS_HANDLE *ihandle, int32_t iaxis, int32_t *piValue)
{
    int32_t ret;
    double data[1];
    ret = parameter_get(ihandle,(iaxis + 0x4000),0x12,1,data,Weld_Direct_GetCommandTargetVelFailed);
    *piValue = data[0];
    return ret;
}

int32_t Weld_Direct_SetSingleDectime(SYS_HANDLE *ihandle, int32_t iaxis, double fValue)
{
    return parameter_set(ihandle,(iaxis + 0x4000),0xd,1,&fValue,Direct_SetDectimeFailed);
}

int32_t Weld_Direct_SetSingleAcctime(SYS_HANDLE *ihandle, int32_t iaxis, double fValue)
{
    return parameter_set(ihandle,(iaxis + 0x4000),0xc,1,&fValue,Direct_SetAcceltimeFailed);
}

int32_t Weld_Direct_SetSingleInitpos(SYS_HANDLE *ihandle, int32_t iaxis, int32_t iValue)
{
    int32_t ret;
    double data[1];
    data[0] = iValue;
    ret = parameter_set(ihandle,(iaxis + 0x4000),0xe,1,data,Direct_SetInitposFailed);
    return ret;
}

int32_t Weld_Direct_SetTargetVel(SYS_HANDLE *ihandle, int32_t iaxis, double fValue)
{
    return parameter_set(ihandle,(iaxis + 0x4000),0x5,1,&fValue,Direct_SetSingleParamFailed);
}

int32_t Weld_Direct_SetSingleParam(SYS_HANDLE *ihandle, int32_t iaxis, double targetlength, double targetvel)
{
    double data[2] = {targetlength,targetvel};
    return parameter_set(ihandle,(iaxis + 0x4000),0x10,2,data,Direct_SetSingleParamFailed);
}

int32_t Weld_SetParameter_Interpolation_mode(SYS_HANDLE *ihandle, int8_t iValue){
    int32_t ret;
    double data[1];
    data[0] = iValue;
    ret = parameter_set(ihandle,0x1000,0x0,1,data,Weld_Direct_SetRot_Interpolation_modeFailed);
    return ret;
}

int32_t Weld_SetParameter_section_overall(SYS_HANDLE *ihandle, int32_t iValue){
    int32_t ret;
    double data[1];
    data[0] = iValue;
    ret = parameter_set(ihandle,0x1000,0x1,1,data,Weld_Direct_SetParameter_section_overallFailed);
    return ret;
}

int32_t Weld_SetParameter_rot_down_distance(SYS_HANDLE *ihandle, double fValue){
    int32_t ret;
    ret = parameter_set(ihandle,0x1000,0x2,1,&fValue,Weld_SetParameter_Rot_down_distanceFailed);
    return ret;
}

int32_t Weld_GetParameter_section_rt(SYS_HANDLE *ihandle, int32_t *piValue)
{
    int32_t ret;
    double data[1];
    ret = parameter_get(ihandle,0x1000,0x3,1,data,Weld_GetParameter_section_rtFailed);
    *piValue = data[0];
    return ret;
}

int32_t Weld_SetParameter_rot_down_vel(SYS_HANDLE *ihandle, double fValue){
    int32_t ret;
    ret = parameter_set(ihandle,0x1000,0x4,1,&fValue,Weld_Direct_SetParameter_rot_down_velFailed);
    return ret;
}

int32_t Weld_SetSystem_write_system_state(SYS_HANDLE *ihandle,int32_t iValue){
    int32_t ret;
    double data[1];
    data[0] = iValue;
    ret = parameter_set(ihandle,0x1000,0x5,1,data,Weld_SetSystem_write_system_stateFailed);
    return ret;
}

int32_t Weld_SetSystem_switch_signal(SYS_HANDLE *ihandle,int64_t iValue){
    int32_t ret;
    double data[1];
    data[0] = iValue;
    ret = parameter_set(ihandle,0x1000,0x6,1,data,Weld_Direct_SetSystem_switch_signalFailed);
    return ret;
}

int32_t Weld_SetParameter_arc_mode(SYS_HANDLE *ihandle,int32_t iValue){
    int32_t ret;
    double data[1];
    data[0] = iValue;
    ret = parameter_set(ihandle,0x1000,0x7,1,data,Weld_Direct_SetParameter_arc_modeFailed);
    return ret;
}

int32_t Weld_SetParameter_arc_forward_speed(SYS_HANDLE *ihandle,double fValue,int32_t mode){
    int32_t ret;
    ret = parameter_set(ihandle,0x1000,0x8,1,&fValue,Weld_Direct_SetParameter_arc_forward_speedFailed);
    return ret;
}

int32_t Weld_SetParameter_arc_backward_speed(SYS_HANDLE *ihandle,double fValue,int32_t mode){
    int32_t ret;
    ret = parameter_set(ihandle,0x1000,0x9,1,&fValue,Weld_Direct_SetParameter_arc_backward_speedFailed);
    return ret;
}

int32_t Weld_SetParameter_arc_backward_distance(SYS_HANDLE *ihandle,double fValue,int32_t mode){
    int32_t ret;
    ret = parameter_set(ihandle,0x1000,0xa,1,&fValue,Weld_Direct_SetParameter_arc_backward_distanceFailed);
    return ret;
}

int32_t Weld_SetParameter_track_speed(SYS_HANDLE *ihandle,double fValue){
    int32_t ret;
    ret = parameter_set(ihandle,0x1000,0xb,1,&fValue,Weld_Direct_SetParameter_track_speedFailed);
    return ret;
}

int32_t Weld_SetParameter_arc_weld_lift_distance(SYS_HANDLE *ihandle,double fValue){
    int32_t ret;
    ret = parameter_set(ihandle,0x1000,0xc,1,&fValue,Weld_Direct_SetParameter_arc_weld_lift_distanceFailed);
    return ret;
}

int32_t Weld_SetParameter_wire_pre_start_distance(SYS_HANDLE *ihandle,double fValue){
    int32_t ret;
    ret = parameter_set(ihandle,0x1000,0xd,1,&fValue,Weld_Direct_SetParameter_wire_pre_start_distanceFailed);
    return ret;
}

int32_t Weld_SetParameter_wire_pre_end_distance(SYS_HANDLE *ihandle,double fValue){
    int32_t ret;
    ret = parameter_set(ihandle,0x1000,0xe,1,&fValue,Weld_SetParameter_wire_pre_end_distanceFailed);
    return ret;
}

int32_t Weld_SetParameter_wire_pre_time(SYS_HANDLE *ihandle,double fValue){
    int32_t ret;
    double value = fValue * 1000;
    ret = parameter_set(ihandle,0x1000,0xf,1,&value,Weld_Direct_SetParameter_wire_pre_timeFailed);
    return ret;
}

int32_t Weld_SetParameter_wire_pre_vel(SYS_HANDLE *ihandle,double fValue){
    int32_t ret;
    ret = parameter_set(ihandle,0x1000,0x10,1,&fValue,Weld_Direct_SetParameter_wire_pre_velFailed);
    return ret;
}

int32_t Weld_SetParameter_wire_extract_time(SYS_HANDLE *ihandle,double fValue){
    int32_t ret;
    double value = fValue * 1000;
    ret = parameter_set(ihandle,0x1000,0x11,1,&value,Weld_Direct_SetParameter_wire_extract_timeFailed);
    return ret;
}

int32_t Weld_SetParameter_wire_extract_vel(SYS_HANDLE *ihandle,double fValue){
    int32_t ret;
    ret = parameter_set(ihandle,0x1000,0x12,1,&fValue,Weld_Direct_SetParameter_wire_extract_velFailed);
    return ret;
}

int32_t Weld_SetParameter_current_pre_time(SYS_HANDLE *ihandle,double fValue){
    int32_t ret;
    double value = fValue*1000;
    ret = parameter_set(ihandle,0x1000,0x13,1,&value,Weld_Direct_SetParameter_current_pre_timeFailed);
    return ret;
}

int32_t Weld_SetParameter_current_pre(SYS_HANDLE *ihandle,double fValue){
    int32_t ret;
    double value = fValue*65535/500;
    ret = parameter_set(ihandle,0x1000,0x14,1,&value,Weld_Direct_SetParameter_current_preFailed);
    return ret;
}

int32_t Weld_SetParameter_current_rise_time(SYS_HANDLE *ihandle,double fValue){
    int32_t ret;
    double value = fValue*1000;
    ret = parameter_set(ihandle,0x1000,0x15,1,&value,Weld_Direct_SetParameter_current_rise_timeFailed);
    return ret;
}

int32_t Weld_SetParameter_current_arc(SYS_HANDLE *ihandle,double fValue,int32_t mode){
    int32_t ret;
    double value = fValue*65535/500;
    ret = parameter_set(ihandle,0x1000,0x16,1,&value,Weld_Direct_SetParameter_current_arcFailed);
    return ret;
}

int32_t Weld_SetParameter_gas_pre_time(SYS_HANDLE *ihandle,double fValue){
    int32_t ret;
    double value = fValue*1000;
    ret = parameter_set(ihandle,0x1000,0x17,1,&value,Weld_Direct_SetParameter_gas_pre_timeFailed);
    return ret;
}

int32_t Weld_SetParameter_gas_delay_time(SYS_HANDLE *ihandle,double fValue){
    int32_t ret;
    double value = fValue*1000;
    ret = parameter_set(ihandle,0x1000,0x18,1,&value,Weld_Direct_SetParameter_gas_delay_timeFailed);
    return ret;
}

int32_t Weld_SetParameter_track_delay(SYS_HANDLE *ihandle,double fValue){
    int32_t ret;
    ret = parameter_set(ihandle,0x1205,0x3,1,&fValue,Weld_Direct_SetParameter_track_delayFailed);
    return ret;
}
/*****************************  IO模块 *****************************/
int32_t Weld_SetParameter_AO(SYS_HANDLE *ihandle,int32_t section,uint16_t iValue){
    int32_t ret;
    double data[1];
    data[0] = iValue;
    ret = parameter_set(ihandle,0x1001,section,1,data,Weld_SetParameter_AOFailed);
    return ret;
}

int32_t Weld_GetParameter_AI(SYS_HANDLE *ihandle,int32_t section,uint16_t *piValue){
    int32_t ret;
    double data[1];
    ret = parameter_get(ihandle,0x1002,section,1,data,Weld_GetParameter_AIFailed);
    *piValue = data[0];
    return ret;
}

int32_t Weld_SetParameter_DO(SYS_HANDLE *ihandle,int32_t section,uint16_t iValue){
    int32_t ret;
    double data[1];
    data[0] = iValue;
    ret = parameter_set(ihandle,0x1003,section,1,data,Weld_SetParameter_DOFailed);
    return ret;
}

int32_t Weld_GetParameter_DI(SYS_HANDLE *ihandle,int32_t section,uint16_t *piValue){
    int32_t ret;
    double data[1];
    ret = parameter_get(ihandle,0x1004,section,1,data,Weld_GetParameter_DIFailed);
    *piValue = data[0];
    return ret;
}

int32_t Weld_SetParameter_DO_GELI(SYS_HANDLE *ihandle,int32_t section,uint8_t iValue){
    int32_t ret;
    double data[1];
    data[0] = iValue;
    ret = parameter_set(ihandle,0x1005,section,1,data,Weld_SetParameter_DOFailed);
    return ret;
}

int32_t Weld_GetParameter_DI_GELI(SYS_HANDLE *ihandle,int32_t section,uint8_t *piValue){
    int32_t ret;
    double data[1];
    ret = parameter_get(ihandle,0x1006,section,1,data,Weld_GetParameter_DIFailed);
    *piValue = data[0];
    return ret;
}



/*****************************  焊接轴区间参数 *****************************/



int32_t Weld_SetParameter_Interval_peak_time(SYS_HANDLE *ihandle,int32_t section, double fValue){
    int32_t ret;
    int32_t section_index = section + 0x1100;
    double value = fValue*1000;
    ret = parameter_set(ihandle,section_index,0x2,1,&value,Weld_Direct_SetParameter_interval_peak_timeFailed);
    return ret;
}

int32_t Weld_SetParameter_Interval_base_time(SYS_HANDLE *ihandle,int32_t section,double fValue){
    int32_t ret;
    int32_t section_index = section + 0x1100;
    double value = fValue*1000;
    ret = parameter_set(ihandle,section_index,0x3,1,&value,Weld_Direct_SetParameter_interval_base_timeFailed);
    return ret;
}

int32_t Weld_SetParameter_Interval_peak_current(SYS_HANDLE *ihandle,int32_t section,double fValue){
    int32_t ret;
    int32_t section_index = section + 0x1100;
    double value = fValue*65535/500;
    ret = parameter_set(ihandle,section_index,0x0,1,&value,Weld_Direct_SetParameter_interval_peak_currentFailed);
    return ret;
}

int32_t Weld_SetParameter_Interval_base_current(SYS_HANDLE *ihandle,int32_t section,double fValue){
    int32_t ret;
    int32_t section_index = section + 0x1100;
    double value = fValue*65535/500;
    ret = parameter_set(ihandle,section_index,0x1,1,&value,Weld_Direct_SetParameter_interval_base_currentFailed);
    return ret;
}

int32_t Weld_SetParameter_Interval_peak_track_vol(SYS_HANDLE *ihandle,int32_t section,double fValue){
    int32_t ret;
    int32_t section_index = section + 0x1100;
    double value = fValue*10000/60;
    ret = parameter_set(ihandle,section_index,0x4,1,&value,Weld_Direct_SetParameter_interval_peak_track_volFailed);
    return ret;
}

int32_t Weld_SetParameter_Interval_base_track_vol(SYS_HANDLE *ihandle,int32_t section,double fValue){
    int32_t ret;
    int32_t section_index = section + 0x1100;
    double value = fValue*10000/60;
    ret = parameter_set(ihandle,section_index,0x5,1,&value,Weld_Direct_SetParameter_interval_base_track_volFailed);
    return ret;
}

int32_t Weld_SetParameter_Interval_peak_wire_vel(SYS_HANDLE *ihandle,int32_t section,double fValue){
    int32_t ret;
    int32_t section_index = section + 0x1100;
    ret = parameter_set(ihandle,section_index,0x6,1,&fValue,Weld_Direct_SetParameter_interval_peak_wire_velFailed);
    return ret;
}

int32_t Weld_SetParameter_Interval_peak_pluse_current(SYS_HANDLE *ihandle,int32_t section,double fValue){
    int32_t ret;
    int32_t section_index = section + 0x1100;
    double value = fValue*65535/500;
    ret = parameter_set(ihandle,section_index,0x7,1,&value,Weld_Direct_SetParameter_interval_peak_pluse_currentFailed);
    return ret;
}

int32_t Weld_SetParameter_Interval_yaw_vel(SYS_HANDLE *ihandle,int32_t section,double fValue){
    int32_t ret;
    int32_t section_index = section + 0x1100;
    ret = parameter_set(ihandle,section_index,0x8,1,&fValue,Weld_Direct_SetParameter_interval_yaw_velFailed);
    return ret;
}

int32_t Weld_SetParameter_Interval_yaw_distance(SYS_HANDLE *ihandle,int32_t section,double fValue){
    int32_t ret;
    int32_t section_index = section + 0x1100;
    ret = parameter_set(ihandle,section_index,0x9,1,&fValue,Weld_Direct_SetParameter_interval_yaw_distanceFailed);
    return ret;
}

int32_t Weld_SetParameter_Interval_yaw_forward_time(SYS_HANDLE *ihandle,int32_t section,double fValue){
    int32_t ret;
    int32_t section_index = section + 0x1100;
    double value = fValue*1000;
    ret = parameter_set(ihandle,section_index,0xa,1,&value,Weld_Direct_SetParameter_interval_yaw_forward_timeFailed);
    return ret;
}

int32_t Weld_SetParameter_Interval_yaw_backward_time(SYS_HANDLE *ihandle,int32_t section,double fValue){
    int32_t ret;
    int32_t section_index = section + 0x1100;
    double value = fValue*1000;
    ret = parameter_set(ihandle,section_index,0xb,1,&value,Weld_Direct_SetParameter_interval_yaw_backward_timeFailed);
    return ret;
}

int32_t Weld_SetParameter_Interval_peak_rot_vel(SYS_HANDLE *ihandle,int32_t section, double fValue){
    int32_t ret;
    int32_t section_index = section + 0x1100;
    ret = parameter_set(ihandle,section_index,0xc,1,&fValue,Weld_SetParameter_Interval_peak_rot_velFailed);
    return ret;
}

int32_t Weld_SetParameter_Interval_base_rot_vel(SYS_HANDLE *ihandle, int32_t section,double fValue){
    int32_t ret;
    int32_t section_index = section + 0x1100;
    ret = parameter_set(ihandle,section_index,0xd,1,&fValue,Weld_Direct_SetParameter_interval_base_rot_velFailed);
    return ret;
}

int32_t Weld_SetParameter_Interval_rot_distance(SYS_HANDLE *ihandle,int32_t section,double fValue){
    int32_t ret;
    int32_t section_index = section + 0x1100;
    ret = parameter_set(ihandle,section_index,0xe,1,&fValue,Weld_Direct_SetParameter_interval_rot_distanceFailed);
    return ret;
}

int32_t Weld_SetParameter_Interval_base_wire_vel(SYS_HANDLE *ihandle,int32_t section,double fValue){
    int32_t ret;
    int32_t section_index = section + 0x1100;
    ret = parameter_set(ihandle,section_index,0xf,1,&fValue,Weld_Direct_SetParameter_interval_base_wire_velFailed);
    return ret;
}

int32_t Weld_SetParameter_Interval_angle_position(SYS_HANDLE *ihandle,int32_t section,double fValue){
    int32_t ret;
    int32_t section_index = section + 0x1100;
    ret = parameter_set(ihandle,section_index,0x10,1,&fValue,Weld_Direct_SetParameter_interval_angle_positionFailed);
    return ret;
}

int32_t Weld_SetParameter_Interval_rot_direction(SYS_HANDLE *ihandle,int32_t section,double fValue){
    int32_t ret;
    int32_t section_index = section + 0x1100;
    ret = parameter_set(ihandle,section_index,0x11,1,&fValue,Weld_Direct_SetParameter_interval_rot_directionFailed);
    return ret;
}

int32_t Weld_SetParameter_Interval_track_mode(SYS_HANDLE *ihandle,int32_t section,double fValue){
    int32_t ret;
    int32_t section_index = section + 0x1100;
    ret = parameter_set(ihandle,section_index,0x12,1,&fValue,Weld_Direct_SetParameter_interval_track_modeFailed);
    return ret;
}
