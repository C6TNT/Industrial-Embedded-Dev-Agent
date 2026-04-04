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
}SYS_HANDLE;
#endif // GLOBAL_H
