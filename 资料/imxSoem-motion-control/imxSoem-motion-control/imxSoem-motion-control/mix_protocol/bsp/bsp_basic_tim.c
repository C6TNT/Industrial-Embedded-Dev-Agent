#include "bsp_basic_tim.h"

// static uint32_t soemSec=0;

// void GPT2_IRQHandler(void)
// {
//    	GPT_ClearStatusFlags(GPT2, kGPT_OutputCompare1Flag);
// 	soemSec++;
// /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F, Cortex-M7, Cortex-M7F Store immediate overlapping
//   exception return operation might vector to incorrect interrupt */
//     SDK_ISR_EXIT_BARRIER;
// }
extern int64_t ticktime;

void MX_TIM2_Init(void)
{
	uint32_t gptFreq;
	gpt_config_t gptConfig;

	CLOCK_SetRootMux(kCLOCK_RootGpt2, kCLOCK_GptRootmuxSysPll1Div20); /* Set GPT1 source to SysPLL1 Div20 40MHZ */
	CLOCK_SetRootDivider(kCLOCK_RootGpt2, 1U, 40U);					  /* Set root clock to 40MHZ / 40 = 1MHZ */

	//        CLOCK_SetRootMux(kCLOCK_RootGpt1, kCLOCK_GptRootmuxSysPll1Div2); /* Set GPT1 source to SYSTEM PLL1 DIV2 400MHZ */
	//        CLOCK_SetRootDivider(kCLOCK_RootGpt1, 1U, 4U);                   /* Set root clock to 400MHZ / 4 = 100MHZ */

	GPT_GetDefaultConfig(&gptConfig);

	GPT_Init(GPT2, &gptConfig);

	GPT_SetClockDivider(GPT2, 1);

	/* GPT frequency is divided by 3 inside module */
	// gptFreq = 0xFFF13D80U;
	gptFreq = cycleTime / 1000;
	/* Set both GPT modules to 1 second duration */
	GPT_SetOutputCompareValue(GPT2, kGPT_OutputCompare_Channel1, gptFreq);
	NVIC_SetPriority(GPT2_IRQn, 2);
	GPT_EnableInterrupts(GPT2, kGPT_OutputCompare1InterruptEnable);
	EnableIRQ(GPT2_IRQn);
	GPT_StartTimer(GPT2);
}

// uint32_t GetSec(void)
// {
//     return GPT2->CNT/1000000+soemSec*4294;
// }

// uint32_t GetUSec(void)
// {
//     return GPT2->CNT%1000000;
// }
uint32_t GetSec(void)
{
	return (ticktime + GPT2->CNT) / 1000000;
}

uint32_t GetUSec(void)
{
	return (ticktime + GPT2->CNT) % 1000000; // soemSec 1  =  4ms
}

void delayus(uint32_t usdelay)
{
	unsigned long oldcnt, newcnt;
	unsigned long tcntvalue = 0;

	oldcnt = GPT2->CNT;
	while (1)
	{
		newcnt = GPT2->CNT;
		if (newcnt != oldcnt)
		{
			if (newcnt > oldcnt)
				tcntvalue += newcnt - oldcnt;
			else
				tcntvalue += 0XFFFFFFFF - oldcnt + newcnt;
			oldcnt = newcnt;
			if (tcntvalue >= usdelay)
				break;
		}
	}
}

void delayms(uint32_t msdelay)
{
	int i = 0;
	for (i = 0; i < msdelay; i++)
	{
		delayus(1000);
	}
}

/*********************************************END OF FILE**********************/
