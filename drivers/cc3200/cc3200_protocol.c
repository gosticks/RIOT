#include <stdbool.h>
#include <stdio.h>

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

#include "include/cc3200_protocol.h"
// #include "proto.h"
// #include "protocol.h"
// #include "setup.h"
// #include "utils.h"

/**
 * @brief Transmission sequence number used for NWP <-> CPU sync
 *
 */
static uint32_t TxSeqNum = 0;

const cc3200_nwp_sync_pattern_t CPU_TO_NWP_SYNC_PATTERN =
        CPU_TO_NET_CHIP_SYNC_PATTERN;
const cc3200_nwp_sync_pattern_t CPU_TO_NWP_CNYS_PATTERN =
        CPU_TO_NET_CHIP_CNYS_PATTERN;

/**
 * @brief forward declaration
 *
 */
void sliceFirstInBuffer(uint8_t *buf, int len);

/**
 * @brief Validate if sync frame is valid:
 *  - validate SYNC_PATTERN
 *  - If N2H_SYNC_PATTERN_SEQ_NUM_EXISTS flag set validate SEQ number
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
int cc3200_read_from_nwp(void *buf, int len)
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
int cc3200_send_to_nwp(const void *buf, int len)
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
int cc3200_read_cmd_header(cc3200_nwp_resp_header_t *buf)
{
    /* write sync pattern to indicate read start */
    cc3200_send_to_nwp(&CPU_TO_NWP_CNYS_PATTERN.Short, sizeof(uint32_t));

    uint32_t SyncCnt = 0;

    /* read the 4 bytes/1 word from NWP until response sync pattern was found.
     * Since the sync pattern can be surrounded by random trash we compare the
     * buffer at all possible offsets */
    // TODO: remove expensice slice operation on favour of pointer magic
    cc3200_read_from_nwp(buf, 4);
    while (!match_sync_pattern((uint32_t *)buf)) {
        /* if we have sliced 4 bytes read 1 word into the buffer */
        if (0 == (SyncCnt % (uint32_t)4)) {
            cc3200_read_from_nwp(&buf[4], 4);
        }

        /* move content of buffer by one byte to the left */
        sliceFirstInBuffer((uint8_t *)buf, 8);

        /* increase sync counter to keep track of sliced of bytes */
        SyncCnt++;
    }

    SyncCnt %= 4;

    // TODO: simplify this step
    if (SyncCnt > 0) {
        *(uint32_t *)buf = *(uint32_t *)&buf[4];
        cc3200_read_from_nwp(&buf[4 - SyncCnt], (uint16_t)SyncCnt);
    } else {
        cc3200_read_from_nwp(buf, 4);
    }

    /* read till sync pattern is found */
    while (match_sync_pattern((uint32_t *)buf)) {
        cc3200_read_from_nwp(buf, 4);
    }

    /* incremenet NWP send counter required for packet validation */
    TxSeqNum++;

    /* Read NWP command header into the buffer */
    cc3200_read_from_nwp(&buf[4], 4);

    /* read reamainder of data on the SPI bus, since NWP will only write new
     * data after all was read/cleared */
    // TODO: this is potentially problematic since it could overwrite command
    // headers, maybe use generic scratch buffer
    cc3200_read_from_nwp(buf, (uint8_t)((SyncCnt > 0) ? (4 - SyncCnt) : 0));
    return 0;
}

void send_header(cc3200_nwp_header_t *header)
{
    cc3200_send_to_nwp(header, sizeof(cc3200_nwp_header_t));
}

/**
 * @brief send a short SYNC command to the NWP. This is send before every
 * command to notify NWP of incoming data
 *
 */
void send_short_sync(void)
{
    cc3200_send_to_nwp(&CPU_TO_NWP_SYNC_PATTERN.Short, sizeof(uint32_t));
}

/**
 * @brief removes the first element in the buffer of length len
 *
 */
void sliceFirstInBuffer(uint8_t *buf, int len)
{
    for (uint8_t i = 0; i < (len - 1); i++) {
        buf[i] = buf[i + 1];
    }
    buf[len - 1] = 0;
}