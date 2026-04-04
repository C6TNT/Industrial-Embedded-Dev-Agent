#ifndef GLOBAL_H
#define GLOBAL_H
#include "time.h"
#include "stdint.h"
#include "math.h"
#include "unistd.h"
#define READ_DELAY_TIME 10000
#define INT32_MAX_COUNT 2147483647
#define MAX_AXIS 20
#define WELD_MAX_AXIS 5
typedef struct{
    bool InitBusEnable;
    bool ATypeSet[MAX_AXIS];
    bool WeldATypeSet[WELD_MAX_AXIS];
    int32_t weldTransper[WELD_MAX_AXIS];
    int32_t transper[MAX_AXIS];
    bool PythonEnable;
    int32_t PythonAxisNum;
}SYS_HANDLE;
#endif // GLOBAL_H
