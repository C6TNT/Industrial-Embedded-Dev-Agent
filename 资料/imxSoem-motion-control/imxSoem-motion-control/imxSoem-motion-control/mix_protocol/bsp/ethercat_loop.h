#ifndef ETHERCAT_LOOP_H_
#define ETHERCAT_LOOP_H_
#include "weld_sever/weld_system.h"
typedef struct ethercat_control ethercat_control_t;
typedef struct weld_opt weld_opt_t;
struct ethercat_control
{
    int (*ec_home_set)(weld_opt_t *dev, uint8_t mode);
};


extern ethercat_control_t ethercat_control;
void ethercat_loop_task(void *param);
void ecatcheck(void *param);
#endif /* RSC_TABLE_H_ */
