#include "bsp/ethernet_rtl8211f.h"

static phy_rtl8211f_resource_t g_phy_resource;

/*! @brief MAC transfer. */
enet_handle_t g_handle;

/*! @brief The MAC address for ENET device. */

/*! @brief PHY status. */
static phy_handle_t phyHandle;

#define EXAMPLE_ENET ENET1
#define EXAMPLE_PHY_ADDRESS 0x01U
#define EXAMPLE_PHY_INTERFACE_RGMII
#define EXAMPLE_PHY_OPS &phyrtl8211f_ops
#define EXAMPLE_PHY_RESOURCE &g_phy_resource
#define EXAMPLE_CLOCK_FREQ CLOCK_GetFreq(kCLOCK_EnetIpgClk)
#define ENET_RXBD_NUM (6)
#define ENET_TXBD_NUM (6)
#define ENET_RXBUFF_SIZE (ENET_FRAME_MAX_FRAMELEN)
#define ENET_TXBUFF_SIZE (ENET_FRAME_MAX_FRAMELEN)
#ifndef APP_ENET_BUFF_ALIGNMENT
#define APP_ENET_BUFF_ALIGNMENT ENET_BUFF_ALIGNMENT
#endif
#ifndef PHY_AUTONEGO_TIMEOUT_COUNT
#define PHY_AUTONEGO_TIMEOUT_COUNT (300000)
#endif
#ifndef PHY_STABILITY_DELAY_US
#define PHY_STABILITY_DELAY_US (0U)
#endif
#ifndef EXAMPLE_PHY_LINK_INTR_SUPPORT
#define EXAMPLE_PHY_LINK_INTR_SUPPORT (0U)
#endif

/* @TEST_ANCHOR */

#ifndef MAC_ADDRESS
#define MAC_ADDRESS \
    {               \
        0x54, 0x27, 0x8d, 0x00, 0x00, 0x00}
#else
#define USER_DEFINED_MAC_ADDRESS
#endif

static uint8_t g_macAddr[6] = MAC_ADDRESS;
/*! @brief Buffer descriptors should be in non-cacheable region and should be align to "ENET_BUFF_ALIGNMENT". */
AT_NONCACHEABLE_SECTION_ALIGN(enet_rx_bd_struct_t g_rxBuffDescrip[ENET_RXBD_NUM], ENET_BUFF_ALIGNMENT);
AT_NONCACHEABLE_SECTION_ALIGN(enet_tx_bd_struct_t g_txBuffDescrip[ENET_TXBD_NUM], ENET_BUFF_ALIGNMENT);
/*! @brief The data buffers can be in cacheable region or in non-cacheable region.
 * If use cacheable region, the alignment size should be the maximum size of "CACHE LINE SIZE" and "ENET_BUFF_ALIGNMENT"
 * If use non-cache region, the alignment size is the "ENET_BUFF_ALIGNMENT".
 */
SDK_ALIGN(uint8_t g_rxDataBuff[ENET_RXBD_NUM][SDK_SIZEALIGN(ENET_RXBUFF_SIZE, APP_ENET_BUFF_ALIGNMENT)],
          APP_ENET_BUFF_ALIGNMENT);
SDK_ALIGN(uint8_t g_txDataBuff[ENET_TXBD_NUM][SDK_SIZEALIGN(ENET_TXBUFF_SIZE, APP_ENET_BUFF_ALIGNMENT)],
          APP_ENET_BUFF_ALIGNMENT);

static void MDIO_Init(void)
{
    (void)CLOCK_EnableClock(s_enetClock[ENET_GetInstance(EXAMPLE_ENET)]);
    ENET_SetSMI(EXAMPLE_ENET, EXAMPLE_CLOCK_FREQ, false);
}

static status_t MDIO_Write(uint8_t phyAddr, uint8_t regAddr, uint16_t data)
{
    return ENET_MDIOWrite(EXAMPLE_ENET, phyAddr, regAddr, data);
}

static status_t MDIO_Read(uint8_t phyAddr, uint8_t regAddr, uint16_t *pData)
{
    return ENET_MDIORead(EXAMPLE_ENET, phyAddr, regAddr, pData);
}

#if (defined(EXAMPLE_PHY_LINK_INTR_SUPPORT) && (EXAMPLE_PHY_LINK_INTR_SUPPORT))
static bool linkChange = false;
#endif
void eth_init_rtl8211()
{
    enet_config_t config;
    phy_config_t phyConfig = {0};
    bool link = false;
    bool autonego = false;
    phy_speed_t speed;
    phy_duplex_t duplex;
    status_t status;
    volatile uint32_t count = 0;
    gpio_pin_config_t gpio_config = {kGPIO_DigitalOutput, 0, kGPIO_NoIntmode};
    GPIO_PinInit(GPIO4, 2, &gpio_config);
    GPIO_WritePinOutput(GPIO4, 2, 0);
    SDK_DelayAtLeastUs(10000, SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY);
    GPIO_WritePinOutput(GPIO4, 2, 1);
    SDK_DelayAtLeastUs(30000, SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY);
    EnableIRQ(ENET1_MAC0_Rx_Tx_Done1_IRQn);
    EnableIRQ(ENET1_MAC0_Rx_Tx_Done2_IRQn);

    MDIO_Init();
    g_phy_resource.read = MDIO_Read;
    g_phy_resource.write = MDIO_Write;
    PRINTF("\r\nENET example start.\r\n");

    /* Prepare the buffer configuration. */
    enet_buffer_config_t buffConfig[] = {{
        ENET_RXBD_NUM,
        ENET_TXBD_NUM,
        SDK_SIZEALIGN(ENET_RXBUFF_SIZE, APP_ENET_BUFF_ALIGNMENT),
        SDK_SIZEALIGN(ENET_TXBUFF_SIZE, APP_ENET_BUFF_ALIGNMENT),
        &g_rxBuffDescrip[0],
        &g_txBuffDescrip[0],
        &g_rxDataBuff[0][0],
        &g_txDataBuff[0][0],
        true,
        true,
        NULL,
    }};

    /* Get default configuration. */
    /*
     * config.miiMode = kENET_RmiiMode;
     * config.miiSpeed = kENET_MiiSpeed100M;
     * config.miiDuplex = kENET_MiiFullDuplex;
     * config.rxMaxFrameLen = ENET_FRAME_MAX_FRAMELEN;
     */
    ENET_GetDefaultConfig(&config);
    // config.macSpecialConfig = kENET_ControlPromiscuousEnable;

    /* The miiMode should be set according to the different PHY interfaces. */
#ifdef EXAMPLE_PHY_INTERFACE_RGMII
    config.miiMode = kENET_RgmiiMode;
#else
    config.miiMode = kENET_RmiiMode;
#endif
    phyConfig.phyAddr = EXAMPLE_PHY_ADDRESS;
    phyConfig.autoNeg = true;
    phyConfig.ops = EXAMPLE_PHY_OPS;
    phyConfig.resource = EXAMPLE_PHY_RESOURCE;
#if (defined(EXAMPLE_PHY_LINK_INTR_SUPPORT) && (EXAMPLE_PHY_LINK_INTR_SUPPORT))
    phyConfig.intrType = kPHY_IntrActiveLow;
#endif

    /* Initialize PHY and wait auto-negotiation over. */
    PRINTF("Wait for PHY init...\r\n");

    do
    {
        status = PHY_Init(&phyHandle, &phyConfig);
        if (status == kStatus_Success)
        {
            PRINTF("Wait for PHY link up...\r\n");
            /* Wait for auto-negotiation success and link up */
            count = PHY_AUTONEGO_TIMEOUT_COUNT;
            do
            {
                PHY_GetLinkStatus(&phyHandle, &link);
                if (link)
                {
                    PHY_GetAutoNegotiationStatus(&phyHandle, &autonego);
                    if (autonego)
                    {
                        break;
                    }
                }
            } while (--count);
            if (!autonego)
            {
                PRINTF("PHY Auto-negotiation failed. Please check the cable connection and link partner setting.\r\n");
            }
        }
    } while (!(link && autonego));

#if PHY_STABILITY_DELAY_US
    /* Wait a moment for PHY status to be stable. */
    SDK_DelayAtLeastUs(PHY_STABILITY_DELAY_US, SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY);
#endif

    /* Get the actual PHY link speed and set in MAC. */
    PHY_GetLinkSpeedDuplex(&phyHandle, &speed, &duplex);
    config.miiSpeed = (enet_mii_speed_t)speed;
    config.miiDuplex = (enet_mii_duplex_t)duplex;

#ifndef USER_DEFINED_MAC_ADDRESS
    /* Set special address for each chip. */
    SILICONID_ConvertToMacAddr(&g_macAddr);
#endif

    /* Init the ENET. */
    ENET_Init(EXAMPLE_ENET, &g_handle, &config, &buffConfig[0], &g_macAddr[0], EXAMPLE_CLOCK_FREQ);
    ENET_ActiveRead(EXAMPLE_ENET);

#ifdef MCMGR_USED
    /* Initialize MCMGR before calling its API */
    (void)MCMGR_Init();
#endif /* MCMGR_USED */
}