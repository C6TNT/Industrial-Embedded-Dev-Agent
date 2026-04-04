#ifndef UPS_H_
#define UPS_H_
#include "fsl_gpio.h"
#include "fsl_common.h"

#define UPS_LIMIT_GPIO_PORT GPIO5
#define UPS_LIMIT_GPIO_PIN 24
#define UPS_OUTPUT_GPIO_PORT GPIO5
#define UPS_OUTPUT_GPIO_PIN 25

void UPS_GPIO_Init(void);
int UPS_power_check(void);
#endif /* RSC_TABLE_H_ */
