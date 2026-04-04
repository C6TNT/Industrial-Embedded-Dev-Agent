
#include "flexspi_nor.h"

flexspi_device_config_t deviceconfig = {
    .flexspiRootClk = 12000000,
    .flashSize = FLASH_SIZE,
    .CSIntervalUnit = kFLEXSPI_CsIntervalUnit1SckCycle,
    .CSInterval = 2,
    .CSHoldTime = 3,
    .CSSetupTime = 3,
    .dataValidTime = 0,
    .columnspace = 0,
    .enableWordAddress = 0,
    .AWRSeqIndex = 0,
    .AWRSeqNumber = 0,
    .ARDSeqIndex = NOR_CMD_LUT_SEQ_IDX_READ,
    .ARDSeqNumber = 1,
    .AHBWriteWaitUnit = kFLEXSPI_AhbWriteWaitUnit2AhbCycle,
    .AHBWriteWaitInterval = 0,
};

const uint32_t customLUT[CUSTOM_LUT_LENGTH] = {
    /* Fast read quad mode -SDR */
    [4 * NOR_CMD_LUT_SEQ_IDX_READ + 0] =
        FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x13, kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x18),
    [4 * NOR_CMD_LUT_SEQ_IDX_READ + 1] =
        FLEXSPI_LUT_SEQ(kFLEXSPI_Command_READ_SDR, kFLEXSPI_1PAD, 0x04, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x00),

    /* Read status register */
    [4 * NOR_CMD_LUT_SEQ_IDX_READSTATUSREG] =
        FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x05, kFLEXSPI_Command_READ_SDR, kFLEXSPI_1PAD, 0x02),
    /* Read ID */
    [4 * NOR_CMD_LUT_SEQ_IDX_READID] =
        FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x9F, kFLEXSPI_Command_READ_SDR, kFLEXSPI_1PAD, 0x04),

    /*  Write Enable 4 pad */
    [4 * NOR_CMD_LUT_SEQ_IDX_WRITEENABLE_OPI] =
        FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x06, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x00),

    /*  Write Enable */
    [4 * NOR_CMD_LUT_SEQ_IDX_WRITEENABLE] =
        FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x06, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0),

    /*  Erase Sector */
    [4 * NOR_CMD_LUT_SEQ_IDX_ERASESECTOR] =
        FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x20, kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x18),

    /*  Program */
    [4 * NOR_CMD_LUT_SEQ_IDX_PAGEPROGRAM_QUAD] =
        FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x02, kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x18),
    [4 * NOR_CMD_LUT_SEQ_IDX_PAGEPROGRAM_QUAD + 1] =
        FLEXSPI_LUT_SEQ(kFLEXSPI_Command_WRITE_SDR, kFLEXSPI_1PAD, 0x04, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0),

    /* Enter Quad mode */
    [4 * NOR_CMD_LUT_SEQ_IDX_ENABLEQUAD] =
        FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x35, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x00),

    /*  Dummy write, do nothing when AHB write command is triggered. */
    [4 * NOR_CMD_LUT_SEQ_IDX_WRITE] =
        FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),
};

void flexspi_nor_flash_init(FLEXSPI_Type *base)
{
    flexspi_config_t config;
    /* To store custom's LUT table in local. */
    uint32_t tempLUT[CUSTOM_LUT_LENGTH] = {0x00U};

    /* Copy LUT information from flash region into RAM region, because LUT update maybe corrupt read sequence(LUT[0])
     * and load wrong LUT table from FLASH region. */
    memcpy(tempLUT, customLUT, sizeof(tempLUT));

    CLOCK_SetRootMux(kCLOCK_RootQspi, kCLOCK_QspiRootmuxSysPll1Div8); /* Set QSPI source to SYSTEM PLL1 DIV8 100MHZ */
    CLOCK_SetRootDivider(kCLOCK_RootQspi, 1U, 2U);                    /* Set root clock to 100MHZ / 2 = 50MHZ */

    /*Get FLEXSPI default settings and configure the flexspi. */
    FLEXSPI_GetDefaultConfig(&config);

    /*Set AHB buffer size for reading data through AHB bus. */
    config.ahbConfig.enableAHBPrefetch = true;
    config.ahbConfig.enableAHBBufferable = true;
    config.ahbConfig.enableReadAddressOpt = true;
    config.ahbConfig.enableAHBCachable = true;
    config.rxSampleClock = EXAMPLE_FLEXSPI_RX_SAMPLE_CLOCK;
    FLEXSPI_Init(base, &config);

    /* Configure flash settings according to serial flash feature. */
    FLEXSPI_SetFlashConfig(base, &deviceconfig, FLASH_PORT);

    /* Update LUT table. */
    FLEXSPI_UpdateLUT(base, 0, tempLUT, CUSTOM_LUT_LENGTH);

    /* Do software reset. */
    FLEXSPI_SoftwareReset(base);
}

status_t flexspi_nor_write_enable(FLEXSPI_Type *base, uint32_t baseAddr)
{
    flexspi_transfer_t flashXfer;
    status_t status;

    /* Write enable */
    flashXfer.deviceAddress = baseAddr;
    flashXfer.port = FLASH_PORT;
    flashXfer.cmdType = kFLEXSPI_Command;
    flashXfer.SeqNumber = 1;
    flashXfer.seqIndex = NOR_CMD_LUT_SEQ_IDX_WRITEENABLE;

    status = FLEXSPI_TransferBlocking(base, &flashXfer);

    return status;
}

status_t flexspi_nor_wait_bus_busy(FLEXSPI_Type *base)
{
    /* Wait status ready. */
    bool isBusy;
    uint32_t readValue;
    status_t status;
    flexspi_transfer_t flashXfer;

    flashXfer.deviceAddress = 0;
    flashXfer.port = FLASH_PORT;
    flashXfer.cmdType = kFLEXSPI_Read;
    flashXfer.SeqNumber = 1;
    flashXfer.seqIndex = NOR_CMD_LUT_SEQ_IDX_READSTATUSREG;
    flashXfer.data = &readValue;
    flashXfer.dataSize = 1;

    do
    {
        status = FLEXSPI_TransferBlocking(base, &flashXfer);

        if (status != kStatus_Success)
        {
            return status;
        }
        if (FLASH_BUSY_STATUS_POL)
        {
            if (readValue & (1U << FLASH_BUSY_STATUS_OFFSET))
            {
                isBusy = true;
            }
            else
            {
                isBusy = false;
            }
        }
        else
        {
            if (readValue & (1U << FLASH_BUSY_STATUS_OFFSET))
            {
                isBusy = false;
            }
            else
            {
                isBusy = true;
            }
        }

    } while (isBusy);

    return status;
}

status_t flexspi_nor_get_vendor_id(FLEXSPI_Type *base, uint8_t *vendorId)
{
    uint32_t temp;
    flexspi_transfer_t flashXfer;
    flashXfer.deviceAddress = 0;
    flashXfer.port = FLASH_PORT;
    flashXfer.cmdType = kFLEXSPI_Read;
    flashXfer.SeqNumber = 1;
    flashXfer.seqIndex = NOR_CMD_LUT_SEQ_IDX_READID;
    flashXfer.data = &temp;
    flashXfer.dataSize = 1;

    status_t status = FLEXSPI_TransferBlocking(base, &flashXfer);

    *vendorId = temp;

    /* Do software reset or clear AHB buffer directly. */
#if defined(FSL_FEATURE_SOC_OTFAD_COUNT) && defined(FLEXSPI_AHBCR_CLRAHBRXBUF_MASK) && \
    defined(FLEXSPI_AHBCR_CLRAHBTXBUF_MASK)
    base->AHBCR |= FLEXSPI_AHBCR_CLRAHBRXBUF_MASK | FLEXSPI_AHBCR_CLRAHBTXBUF_MASK;
    base->AHBCR &= ~(FLEXSPI_AHBCR_CLRAHBRXBUF_MASK | FLEXSPI_AHBCR_CLRAHBTXBUF_MASK);
#else
    FLEXSPI_SoftwareReset(base);
#endif

    return status;
}

status_t flexspi_nor_flash_erase_sector(FLEXSPI_Type *base, uint32_t address)
{
    status_t status;
    flexspi_transfer_t flashXfer;

#if defined(CACHE_MAINTAIN) && CACHE_MAINTAIN
    flexspi_cache_status_t cacheStatus;
    flexspi_nor_disable_cache(&cacheStatus);
#endif

    /* Write enable */
    flashXfer.deviceAddress = address;
    flashXfer.port = FLASH_PORT;
    flashXfer.cmdType = kFLEXSPI_Command;
    flashXfer.SeqNumber = 1;
    flashXfer.seqIndex = NOR_CMD_LUT_SEQ_IDX_WRITEENABLE;

    status = FLEXSPI_TransferBlocking(base, &flashXfer);

    if (status != kStatus_Success)
    {
        return status;
    }

    flashXfer.deviceAddress = address;
    flashXfer.port = FLASH_PORT;
    flashXfer.cmdType = kFLEXSPI_Command;
    flashXfer.SeqNumber = 1;
    flashXfer.seqIndex = NOR_CMD_LUT_SEQ_IDX_ERASESECTOR;
    status = FLEXSPI_TransferBlocking(base, &flashXfer);

    if (status != kStatus_Success)
    {
        return status;
    }

    status = flexspi_nor_wait_bus_busy(base);

    /* Do software reset. */
    FLEXSPI_SoftwareReset(base);

#if defined(CACHE_MAINTAIN) && CACHE_MAINTAIN
    flexspi_nor_enable_cache(cacheStatus);
#endif

    return status;
}

void flash_init()
{
    status_t status;
    uint8_t vendorID = 0;
    flexspi_nor_flash_init(EXAMPLE_FLEXSPI);

    PRINTF("\r\nFLEXSPI example started!\r\n");

    status = flexspi_nor_get_vendor_id(EXAMPLE_FLEXSPI, &vendorID);
    if (status != kStatus_Success)
    {
        return;
    }
    PRINTF("Vendor ID: 0x%d\r\n", vendorID);

    // status = flexspi_nor_enable_quad_mode(EXAMPLE_FLEXSPI);
    // if (status != kStatus_Success)
    // {
    //     return;
    // }
    // PRINTF("enable quad mode success\r\n");
}
status_t flexspi_nor_flash_page_program(FLEXSPI_Type *base, uint32_t dstAddr, const uint32_t *src)
{
    status_t status;
    flexspi_transfer_t flashXfer;

#if defined(CACHE_MAINTAIN) && CACHE_MAINTAIN
    flexspi_cache_status_t cacheStatus;
    flexspi_nor_disable_cache(&cacheStatus);
#endif

    /* To make sure external flash be in idle status, added wait for busy before program data for
        an external flash without RWW(read while write) attribute.*/
    status = flexspi_nor_wait_bus_busy(base);

    if (kStatus_Success != status)
    {
        return status;
    }

    /* Write enable. */
    status = flexspi_nor_write_enable(base, dstAddr);

    if (status != kStatus_Success)
    {
        return status;
    }

    /* Prepare page program command */
    flashXfer.deviceAddress = dstAddr;
    flashXfer.port = FLASH_PORT;
    flashXfer.cmdType = kFLEXSPI_Write;
    flashXfer.SeqNumber = 1;
    flashXfer.seqIndex = NOR_CMD_LUT_SEQ_IDX_PAGEPROGRAM_QUAD;
    flashXfer.data = (uint32_t *)src;
    flashXfer.dataSize = FLASH_PAGE_SIZE;
    status = FLEXSPI_TransferBlocking(base, &flashXfer);

    if (status != kStatus_Success)
    {
        return status;
    }

    status = flexspi_nor_wait_bus_busy(base);

    /* Do software reset or clear AHB buffer directly. */
#if defined(FSL_FEATURE_SOC_OTFAD_COUNT) && defined(FLEXSPI_AHBCR_CLRAHBRXBUF_MASK) && \
    defined(FLEXSPI_AHBCR_CLRAHBTXBUF_MASK)
    base->AHBCR |= FLEXSPI_AHBCR_CLRAHBRXBUF_MASK | FLEXSPI_AHBCR_CLRAHBTXBUF_MASK;
    base->AHBCR &= ~(FLEXSPI_AHBCR_CLRAHBRXBUF_MASK | FLEXSPI_AHBCR_CLRAHBTXBUF_MASK);
#else
    FLEXSPI_SoftwareReset(base);
#endif

#if defined(CACHE_MAINTAIN) && CACHE_MAINTAIN
    flexspi_nor_enable_cache(cacheStatus);
#endif

    return status;
}

status_t FlexSPI_NorFlash_Buffer_Read(FLEXSPI_Type *base,
                                      uint32_t address,
                                      uint8_t *dst,
                                      uint16_t dataSize)
{
    status_t status;
    flexspi_transfer_t flashXfer;

    /* 设置传输结构体 */
    flashXfer.deviceAddress = address;
    flashXfer.port = kFLEXSPI_PortA1;
    flashXfer.cmdType = kFLEXSPI_Read;
    flashXfer.SeqNumber = 1;
    flashXfer.seqIndex = NOR_CMD_LUT_SEQ_IDX_READ;
    flashXfer.data = (uint32_t *)dst;
    flashXfer.dataSize = dataSize;

    status = FLEXSPI_TransferBlocking(base, &flashXfer);

    return status;
}

int32_t flash_weld_parameter_write(weld_parameter_table_t *weld_parameter)
{
    status_t status;
    uint32_t dataSize = sizeof(weld_parameter_table_t);
    uint8_t *dataPtr = (uint8_t *)weld_parameter;
    uint32_t remaining = dataSize;
    uint32_t currentAddr = WELD_PARAMETER_ADDR;
    status = flexspi_nor_flash_erase_sector(EXAMPLE_FLEXSPI, WELD_PARAMETER_SECTOR * SECTOR_SIZE);

    if (status != kStatus_Success)
    {
        PRINTF("Erase sector failure !\r\n");
        return -1;
    }
    while (remaining > 0)
    {
        uint32_t writeSize = (remaining > FLASH_PAGE_SIZE) ? FLASH_PAGE_SIZE : remaining;

        // 调用页编程函数（已适配非Quad模式）
        status = flexspi_nor_flash_page_program(EXAMPLE_FLEXSPI, currentAddr, (uint32_t *)dataPtr);
        if (status != kStatus_Success)
        {
            PRINTF("Page program failed at 0x%X!\r\n", currentAddr);
            return -4;
        }
        // 等待W25Q内部编程完成（确保数据写入稳定）
        status = flexspi_nor_wait_bus_busy(EXAMPLE_FLEXSPI);
        if (status != kStatus_Success)
        {
            PRINTF("Wait for W25Q busy failed!\r\n");
            return -5;
        }

        // 更新地址和剩余长度
        remaining -= writeSize;
        currentAddr += writeSize;
        dataPtr += writeSize;
    }
    return 0;
}

int32_t flash_weld_parameter_read(weld_parameter_table_t *weld_parameter)
{
    status_t status;
    uint32_t dataSize = sizeof(weld_parameter_table_t);
    uint8_t *dataPtr = (uint8_t *)weld_parameter;
    uint32_t remaining = dataSize;
    uint32_t currentAddr = WELD_PARAMETER_ADDR;

    // 检查目标缓冲区是否有效
    if (weld_parameter == NULL)
    {
        PRINTF("Invalid buffer pointer!\r\n");
        return -1;
    }

    while (remaining > 0)
    {
        // 计算本次读取大小，不超过页大小
        uint32_t readSize = (remaining > FLASH_PAGE_SIZE) ? FLASH_PAGE_SIZE : remaining;

        // 调用底层读取函数
        status = FlexSPI_NorFlash_Buffer_Read(EXAMPLE_FLEXSPI, currentAddr, dataPtr, readSize);
        if (status != kStatus_Success)
        {
            PRINTF("Read failed at address 0x%X!\r\n", currentAddr);
            return -2;
        }

        // 更新地址和剩余长度
        remaining -= readSize;
        currentAddr += readSize;
        dataPtr += readSize;
    }

    return 0;
}

int32_t flash_axis_parameter_write(weld_joint_axis_t *axis_parameter)
{
    status_t status;
    axis_parameter_t write_in[JOINT_MAX_COUNT] = {0};
    for (int i = 0; i < JOINT_MAX_COUNT; i++)
    {
        write_in[i].acc = axis_parameter[i].attribute.acc;
        write_in[i].dec = axis_parameter[i].attribute.dec;
    }
    uint32_t dataSize = sizeof(axis_parameter_t) * JOINT_MAX_COUNT;
    uint8_t *dataPtr = (uint8_t *)write_in;
    uint32_t remaining = dataSize;
    uint32_t currentAddr = AXIS_PARAMETER_ADDR;
    status = flexspi_nor_flash_erase_sector(EXAMPLE_FLEXSPI, AXIS_PARAMETER_SECTOR * SECTOR_SIZE);

    if (status != kStatus_Success)
    {
        PRINTF("Erase sector failure !\r\n");
        return -1;
    }
    while (remaining > 0)
    {
        uint32_t writeSize = (remaining > FLASH_PAGE_SIZE) ? FLASH_PAGE_SIZE : remaining;

        // 调用页编程函数（已适配非Quad模式）
        status = flexspi_nor_flash_page_program(EXAMPLE_FLEXSPI, currentAddr, (uint32_t *)dataPtr);
        if (status != kStatus_Success)
        {
            PRINTF("Page program failed at 0x%X!\r\n", currentAddr);
            return -4;
        }
        // 等待W25Q内部编程完成（确保数据写入稳定）
        status = flexspi_nor_wait_bus_busy(EXAMPLE_FLEXSPI);
        if (status != kStatus_Success)
        {
            PRINTF("Wait for W25Q busy failed!\r\n");
            return -5;
        }

        // 更新地址和剩余长度
        remaining -= writeSize;
        currentAddr += writeSize;
        dataPtr += writeSize;
    }
    return 0;
}

int32_t flash_axis_parameter_read(weld_joint_axis_t *axis_parameter)
{
    status_t status;
    axis_parameter_t read_out[JOINT_MAX_COUNT] = {0};

    uint32_t dataSize = sizeof(axis_parameter_t) * JOINT_MAX_COUNT;
    uint8_t *dataPtr = (uint8_t *)read_out;
    uint32_t remaining = dataSize;
    uint32_t currentAddr = AXIS_PARAMETER_ADDR;

    // 检查目标缓冲区是否有效
    if (axis_parameter == NULL)
    {
        PRINTF("Invalid buffer pointer!\r\n");
        return -1;
    }

    while (remaining > 0)
    {
        // 计算本次读取大小，不超过页大小
        uint32_t readSize = (remaining > FLASH_PAGE_SIZE) ? FLASH_PAGE_SIZE : remaining;

        // 调用底层读取函数
        status = FlexSPI_NorFlash_Buffer_Read(EXAMPLE_FLEXSPI, currentAddr, dataPtr, readSize);
        if (status != kStatus_Success)
        {
            PRINTF("Read failed at address 0x%X!\r\n", currentAddr);
            return -2;
        }

        // 更新地址和剩余长度
        remaining -= readSize;
        currentAddr += readSize;
        dataPtr += readSize;
    }

    for (int i = 0; i < JOINT_MAX_COUNT; i++)
    {
        axis_parameter[i].attribute.acc = read_out[i].acc;
        axis_parameter[i].attribute.dec = read_out[i].dec;
    }

    return 0;
}