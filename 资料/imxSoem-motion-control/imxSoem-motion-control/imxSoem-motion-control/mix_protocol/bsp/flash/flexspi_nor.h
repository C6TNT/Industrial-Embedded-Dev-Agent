#ifndef __FLEXSPI_NOR_H_
#define __FLEXSPI_NOR_H_
#include "fsl_clock.h"
#include "fsl_flexspi.h"
#include "fsl_debug_console.h"
#include "weld_sever/weld_system.h"
/* @brief QSPI lookup table depth. */
#define FSL_FEATURE_QSPI_LUT_DEPTH (64)
/* @brief QSPI Tx FIFO depth. */
#define FSL_FEATURE_QSPI_TXFIFO_DEPTH (16)
/* @brief QSPI Rx FIFO depth. */
#define FSL_FEATURE_QSPI_RXFIFO_DEPTH (16)
/* @brief QSPI AHB buffer count. */
#define FSL_FEATURE_QSPI_AHB_BUFFER_COUNT (4)
/* @brief QSPI has command usage error flag. */
#define FSL_FEATURE_QSPI_HAS_IP_COMMAND_USAGE_ERROR (1)
/* @brief QSPI support parallel mode. */
#define FSL_FEATURE_QSPI_SUPPORT_PARALLEL_MODE (1)
/* @brief QSPI support dual die. */
#define FSL_FEATURE_QSPI_SUPPORT_DUAL_DIE (1)
/* @brief there is  no SCLKCFG bit in MCR register. */
#define FSL_FEATURE_QSPI_CLOCK_CONTROL_EXTERNAL (1)
/* @brief there is no AITEF bit in FR register. */
#define FSL_FEATURE_QSPI_HAS_NO_AITEF (1)
/* @brief  there is no AIBSEF bit in FR register. */
#define FSL_FEATURE_QSPI_HAS_NO_AIBSEF (1)
/* @brief there is no TXDMA and TXWA bit in SR register. */
#define FSL_FEATURE_QSPI_HAS_NO_TXDMA (1)
/* @brief there is no SFACR register. */
#define FSL_FEATURE_QSPI_HAS_NO_SFACR (1)
/* @brief there is no TDH bit in FLSHCR register. */
#define FSL_FEATURE_QSPI_HAS_NO_TDH (0)
/* @brief QSPI AHB buffer size in byte. */
#define FSL_FEATURE_QSPI_AHB_BUFFER_SIZE (1024U)
/* @brief QSPI AMBA base address. */
#define FSL_FEATURE_QSPI_AMBA_BASE (0xC0000000U)
/* @brief QSPI AHB buffer ARDB base address. */
#define FSL_FEATURE_QSPI_ARDB_BASE (0x34000000U)
/* @brief QSPI has no SOCCR register. */
#define FSL_FEATURE_QSPI_HAS_NO_SOCCR_REG (1)

#define NOR_CMD_LUT_SEQ_IDX_READ 0
#define NOR_CMD_LUT_SEQ_IDX_READSTATUSREG 1
#define NOR_CMD_LUT_SEQ_IDX_READID 2
#define NOR_CMD_LUT_SEQ_IDX_WRITEENABLE_OPI 3
#define NOR_CMD_LUT_SEQ_IDX_ERASESECTOR 4
#define NOR_CMD_LUT_SEQ_IDX_PAGEPROGRAM_QUAD 5
#define NOR_CMD_LUT_SEQ_IDX_ENABLEQUAD 6
#define NOR_CMD_LUT_SEQ_IDX_WRITE 7
#define NOR_CMD_LUT_SEQ_IDX_WRITEENABLE 8
#define NOR_CMD_LUT_SEQ_IDX_READ_FAST_QUAD 9
#define NOR_CMD_LUT_SEQ_IDX_ERASECHIP 10

#define CUSTOM_LUT_LENGTH 60
#define FLASH_QUAD_ENABLE 1
#define FLASH_BUSY_STATUS_POL 1
#define FLASH_BUSY_STATUS_OFFSET 0

#define XIP_EXTERNAL_FLASH
#define EXAMPLE_FLEXSPI FLEXSPI
#define FLASH_SIZE 0x2000 /* 16KByte */
#define EXAMPLE_FLEXSPI_AMBA_BASE FlexSPI_AMBA_BASE
#define FLASH_PAGE_SIZE 256

#define SECTOR_SIZE 0x1000 /* 4K */

#define WELD_PARAMETER_SECTOR 10
#define WELD_PARAMETER_ADDR (WELD_PARAMETER_SECTOR * SECTOR_SIZE) /* 结构体在Flash中的绝对地址 */

#define AXIS_PARAMETER_SECTOR 5
#define AXIS_PARAMETER_ADDR (AXIS_PARAMETER_SECTOR * SECTOR_SIZE) /* 结构体在Flash中的绝对地址 */
#define EXAMPLE_FLEXSPI_CLOCK kCLOCK_Root_Flexspi1
#define FLASH_PORT kFLEXSPI_PortA1
#define EXAMPLE_FLEXSPI_RX_SAMPLE_CLOCK kFLEXSPI_ReadSampleClkLoopbackInternally

void flash_init();
typedef struct
{
    int32_t transper;
    uint32_t acc;
    uint32_t dec;
} axis_parameter_t;
int32_t flash_weld_parameter_write(weld_parameter_table_t *weld_parameter);
int32_t flash_weld_parameter_read(weld_parameter_table_t *weld_parameter);
int32_t flash_axis_parameter_write(weld_joint_axis_t *axis_parameter);
int32_t flash_axis_parameter_read(weld_joint_axis_t *axis_parameter);
#endif