#ifndef __BASIC_TIM_H
#define	__BASIC_TIM_H

#include "fsl_gpt.h"
#include "weld_sever/nuts_bolts.h"
#define configGPT_CLOCK_HZ                                                                  \
    (CLOCK_GetPllFreq(kCLOCK_SystemPll1Ctrl) / (CLOCK_GetRootPreDivider(kCLOCK_RootGpt2)) / \
     (CLOCK_GetRootPostDivider(kCLOCK_RootGpt2)) / 20)

     

void MX_TIM2_Init(void);

uint32_t GetSec(void);
uint32_t GetUSec(void);

void delayus(uint32_t usdelay);
void delayms(uint32_t msdelay);

#endif /* __BASIC_TIM_H */

