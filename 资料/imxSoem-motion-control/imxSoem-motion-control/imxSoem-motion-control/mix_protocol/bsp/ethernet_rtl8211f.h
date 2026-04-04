#ifndef _ETHERNET_RTL8211F_H_
#define _ETHERNET_RTL8211F_H_
#include "fsl_enet.h"
#include "fsl_phy.h"
#include "fsl_phyrtl8211f.h"
#include "fsl_gpio.h"
#include "fsl_debug_console.h"
#include "fsl_silicon_id.h"
void eth_init_rtl8211();
extern enet_handle_t g_handle;
#endif
