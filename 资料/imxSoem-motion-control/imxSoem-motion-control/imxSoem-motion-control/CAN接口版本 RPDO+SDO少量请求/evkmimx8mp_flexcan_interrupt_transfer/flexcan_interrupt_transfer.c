/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2022 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_debug_console.h"
#include "servo_can.h"

int main(void)
{
    ServoCan_Config config;

    ServoCan_LoadDefaultConfig(&config);
    ServoCan_InitHardware();
    ServoCan_Init(&config);

    PRINTF("Start servo by ServoCan interface\r\n\r\n");

    ServoCan_Start();

    while (true)
    {
        ServoCan_Service();
        SDK_DelayAtLeastUs(ServoCan_GetServicePeriodUs(), SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY);
    }
}
