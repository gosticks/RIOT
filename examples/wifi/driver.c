#include "vendor/hw_adc.h"
#include "vendor/hw_common_reg.h"
#include "vendor/hw_ints.h"
#include "vendor/hw_mcspi.h"
#include "vendor/hw_memmap.h"
#include "vendor/hw_ocp_shared.h"
#include "vendor/hw_types.h"
#include "vendor/hw_udma.h"
#include "vendor/rom.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

#include "periph/spi.h"

#include "driver.h"
#include "proto.h"
#include "protocol.h"
#include "setup.h"
#include "utils.h"

#include <stdbool.h>
#include <stdio.h>

/**
 * @brief Transmission sequence number send initially set by NWP
 *
 */
static uint32_t TxSeqNum = 0;

const _SlSyncPattern_t CPU_TO_NWP_SYNC_PATTERN = CPU_TO_NET_CHIP_SYNC_PATTERN;
const _SlSyncPattern_t CPU_TO_NWP_CNYS_PATTERN = CPU_TO_NET_CHIP_CNYS_PATTERN;

/**
 * @brief Validate if sync frame is valid:
 *  - validate SYNC_PATTERN
 *  - If N2H_SYNC_PATTERN_SEQ_NUM_EXISTS set validate
 *
 * @param buf
 */
static inline bool match_sync_pattern(uint32_t *sync_frame)
{
    if (*sync_frame & NWP_SYNC_PATTERN_SEQ_NUM_SET) {
        return MATCH_WITH_SEQ_NUM(sync_frame, TxSeqNum);
    } else {
        return MATCH_WOUT_SEQ_NUM(sync_frame);
    }
    return false;
}

/**
 * @brief read data from NWP SPI
 *
 * @param buf read destination buffer
 * @param len length of bytes to be read (must be a multiple of word length)
 * @return int returns bytes read
 */
int read(void *buf, int len)
{
    spi_transfer_bytes(1, SPI_CS_UNDEF, false, NULL, buf, len);
    return len;
}

/**
 * @brief send content of buffer buf to the NWP using SPI
 *
 * @param buf buffer to be send
 * @param len length in bytes to be send (must be a multiple of word length)
 * @return int bytes written to SPI
 */
int send(const void *buf, int len)
{
    spi_transfer_bytes(1, SPI_CS_UNDEF, false, buf, NULL, len);
    return len;
}

/**
 * @brief read command header with sync and TxSeqNum validation
 *
 * @param buf destination buffer
 * @param align
 * @return int
 */
int read_cmd_header(cc3200_SlResponseHeader *buf)
{
    /* write sync */
    send(&CPU_TO_NWP_CNYS_PATTERN.Short, sizeof(uint32_t));

    uint32_t SyncCnt = 0;

    // read 4 bytes
    read(buf, 4);
    while (!match_sync_pattern((uint32_t *)buf)) {
        if (0 == (SyncCnt % (uint32_t)4)) {
            read(&buf[4], 4);
        }

        sliceFirstInBuffer((uint8_t *)buf, 8);
        SyncCnt++;
    }

    SyncCnt %= 4;

    if (SyncCnt > 0) {
        *(uint32_t *)buf = *(uint32_t *)&buf[4];
        read(&buf[4 - SyncCnt], (uint16_t)SyncCnt);
    } else {
        read(buf, 4);
    }

    /* read till sync pattern is found */
    while (match_sync_pattern((uint32_t *)buf)) {
        read(buf, 4);
    }
    TxSeqNum++;

    /*  7. Here we've read Generic Header (4 bytes). Read the Resp Specific
     * header (4 more bytes). */
    read(&buf[4], 4);

    /* read reamainder on the SPI bus */
    // uint32_t alignBuf;
    read(buf, (uint8_t)((SyncCnt > 0) ? (4 - SyncCnt) : 0));
    return 0;
}

// int read_cmd_header(cc3200_SlResponseHeader *resp)
// {
//     uint8_t align;
//     _read_cmd_header((uint8_t *)resp, &align);

//     // read up to the next aligned byte

//     return 0;
// }

void send_header(_SlGenericHeader_t *header)
{
    send(header, sizeof(_SlGenericHeader_t));
}

/**
 * @brief send a short SYNC command to the NWP. This is send before every
 * command to notify NWP of incoming data
 *
 */
void send_short_sync(void)
{
    send(&CPU_TO_NWP_SYNC_PATTERN.Short, sizeof(uint32_t));
}

/**
 * @brief quit all services and power off nwp
 *
 */
// NOTE: probably one of the registers is missaligned right now
void graceful_nwp_shutdown(void)
{
    /* turn of all network services */
    HWREG(COMMON_REG_BASE + ADC_O_ADC_CH_ENABLE) = 1;

    ROM_UtilsDelay(800000 / 5);

    /* check if NWP was powered of or is in some Low Power Deep Sleep state */
    if ((GPRCM->NWP_LPDS_WAKEUP_CFG != 0x20) &&
        !(HWREG(OCP_SHARED_BASE + OCP_SHARED_O_SPARE_REG_5) & 0x2)) {
        uint16_t retry_count = 0;
        /* Loop until APPs->NWP interrupt is cleared or timeout */
        while (retry_count < 1000) {
            /* interrupt gets cleared when NWP has powered on */
            if (!(HWREG(COMMON_REG_BASE + COMMON_REG_O_APPS_INT_STS_RAW) &
                  0x1)) {
                break;
            }
            ROM_UtilsDelay(800000 / 5);
            retry_count++;
        }
    }

    /* Clear APPs to NWP interrupt */
    HWREG(COMMON_REG_BASE + ADC_O_adc_ch7_fifo_lvl) = 1;
    ROM_UtilsDelay(800000 / 5);

    /* power of NWP */
    powerOffWifi();
}