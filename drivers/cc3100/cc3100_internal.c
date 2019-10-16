#include "vendor/hw_adc.h"
#include "vendor/hw_apps_rcm.h"
#include "vendor/hw_gprcm.h"
#include "vendor/hw_hib1p2.h"
#include "vendor/hw_ocp_shared.h"
#include "vendor/rom.h"
#include "vendor/socket.h"

#include "cpu.h"

#include "cc3100.h"
#include "include/cc3100_internal.h"
#include "include/cc3100_netdev.h"
#include "include/cc3100_nwp_com.h"
#include "include/cc3100_protocol.h"

#include <stdbool.h>
#include <stddef.h>

#define ENABLE_DEBUG (1)
#include "debug.h"

#define alignDataLen(len) ((len) + 3) & (~3)
#define uSEC_DELAY(x) (ROM_UtilsDelay(x * 80 / 3))
#define A2N_INT_STS_RAW (COMMON_REG_BASE + COMMON_REG_O_APPS_INT_STS_RAW)
#define A2N_INT_STS_CLR (COMMON_REG_BASE + COMMON_REG_O_APPS_INT_STS_CLR)
#define A2N_INT_TRIG (COMMON_REG_BASE + COMMON_REG_O_APPS_INT_TRIG)

#define ANA_DCDC_PARAMS0 (HIB1P2_BASE + HIB1P2_O_ANA_DCDC_PARAMETERS0)
#define ANA_DCDC_PARAMS0_PWMOVERRIDE (0x08000000)

#define NWP_SPARE_REG_5_SLSTOP (0x00000002)
#define NWP_LPDS_WAKEUPCFG (GPRCM_BASE + GPRCM_O_NWP_LPDS_WAKEUP_CFG)
#define NWP_LPDS_WAKEUPCFG_APPS2NWP (0x00000020)
#define NWP_LPDS_WAKEUPCFG_TIMEOUT_MSEC (600)
#define NWP_SPARE_REG_5 (OCP_SHARED_BASE + OCP_SHARED_O_SPARE_REG_5)
#define NWP_SPARE_REG_5_SLSTOP (0x00000002)

#define N2A_INT_ACK (COMMON_REG_BASE + COMMON_REG_O_NW_INT_ACK)

/**
 * @brief utility for printing a EUI48 address
 *
 * @param addr
 */
void printMacAddr(unsigned char *addr)
{
    printf("%02x:", addr[0]);
    printf("%02x:", addr[1]);
    printf("%02x:", addr[2]);
    printf("%02x:", addr[3]);
    printf("%02x:", addr[4]);
    printf("%02x", addr[5]);
    printf("\n");
}

/* WAKENWP - (ARCM_BASE + APPS_RCM_O_APPS_TO_NWP_WAKE_REQUEST)
    - Bits 31:01 - Reserved
    - Bits 00    - Wake Request to NWP
*/
#define WAKENWP_WAKEREQ \
    (APPS_RCM_APPS_TO_NWP_WAKE_REQUEST_APPS_TO_NWP_WAKEUP_REQUEST)

/**
 * @brief Transmission sequence number used for NWP <-> CPU sync
 *
 */
static uint32_t TxSeqNum = 0;

const cc3100_nwp_sync_pattern_t CPU_TO_NWP_SYNC_PATTERN =
        CPU_TO_NET_CHIP_SYNC_PATTERN;
const cc3100_nwp_sync_pattern_t CPU_TO_NWP_CNYS_PATTERN =
        CPU_TO_NET_CHIP_CNYS_PATTERN;

/**
 * @brief forward declaration
 *
 */
void sliceFirstInBuffer(uint8_t *buf, int len);
void cc3100_add_to_drv_queue(volatile cc31xx_nwp_req_t *req);
uint8_t cc3100_remove_from_drv_queue(volatile cc31xx_nwp_req_t *req);

/**
 * @brief cc3100_nwp_rx_handler is the default RX handlers for the NWP
 *
 */
void cc3100_nwp_rx_handler(void)
{
    DEBUG("%s()\n", __FUNCTION__);

    /* reset header buffer values */
    _cmd_header.GenHeader.Opcode  = 0;
    _cmd_header.GenHeader.Len     = 0;
    _cmd_header.TxPoolCnt         = 0;
    _cmd_header.DevStatus         = 0;
    _cmd_header.SocketTXFailure   = 0;
    _cmd_header.SocketNonBlocking = 0;

    cc31xx_read_cmd_header(_dev, &_cmd_header);
    cc3100_cmd_handler(_dev, &_cmd_header);

    /* mark that a interrupt was handled */
    _cc31xx_isr_state = 0;
}

/**
 * @brief register irqn handler
 *
 * @param handler to be registered
 */
static inline void cc3100_register_nwp_rx_irqn(cc3100_rx_irqn_handler handler)
{
    ROM_IntRegister(INT_NWPIC, handler);
    NVIC_SetPriority(NWPIC_IRQn, 2);
    NVIC_ClearPendingIRQ(NWPIC_IRQn);
    NVIC_EnableIRQ(NWPIC_IRQn);
}

/**
 * @brief cc3100_unregister_nwp_rx_irqn unregisters NWP rx irqn handler
 *
 */
static inline void cc3100_unregister_nwp_rx_irqn(void)
{
    DEBUG("%s()\n", __FUNCTION__);
    ROM_IntDisable(INT_NWPIC);
    ROM_IntUnregister(INT_NWPIC);
    ROM_IntPendClear(INT_NWPIC);
}

void cc3100_nwp_power_on(void)
{
    DEBUG("%s()\n", __FUNCTION__);
    /* bring the 1.32 eco out of reset */
    cc3100_reg(NWP_SPARE_REG_5) &= ~NWP_SPARE_REG_5_SLSTOP;

    /* Clear host IRQ indication */
    cc3100_reg(N2A_INT_ACK) = 1;
    /* NWP Wake-up */
    ARCM->APPS_TO_NWP_WAKE_REQUEST = WAKENWP_WAKEREQ;

    // UnMask Host interrupt
    unmask_nwp_rx_irqn();

    DEBUG("cc3100_nwp_power_on: completed \n");
}

void cc3100_nwp_power_off(void)
{
    DEBUG("%s()\n", __FUNCTION__);
    uint32_t apps_int_sts_raw;
    uint32_t sl_stop_ind       = cc3100_reg(NWP_SPARE_REG_5);
    uint32_t nwp_lpds_wake_cfg = cc3100_reg(NWP_LPDS_WAKEUPCFG);
    if ((nwp_lpds_wake_cfg != NWP_LPDS_WAKEUPCFG_APPS2NWP) && /* Check for NWP
                                                                 POR condition -
                                                                 APPS2NWP is
                                                                 reset condition
                                                               */
        !(sl_stop_ind & NWP_SPARE_REG_5_SLSTOP)) /* Check if sl_stop was
                                                    executed */
    {
        cc3100_reg(0xE000E104) = 0x200; /* Enable the out of band interrupt,
                                      this is not a wake-up source*/
        cc3100_reg(A2N_INT_TRIG) = 0x1; /* Trigger out of band interrupt  */
        ARCM->APPS_TO_NWP_WAKE_REQUEST = WAKENWP_WAKEREQ; /* Wake-up the NWP */

        // _SlDrvStartMeasureTimeout(&SlTimeoutInfo,
        // NWP_N2A_INT_ACK_TIMEOUT_MSEC);

        /* Wait for the A2N_INT_TRIG to be cleared by the NWP to indicate it's
         * awake and ready for shutdown. poll until APPs->NWP interrupt is
         * cleared or timeout : for service pack 3.1.99.1 or higher, this
         * condition is fulfilled in less than 1 mSec.
         * Otherwise, in some cases it may require up to 3000 mSec of waiting.
         */
        apps_int_sts_raw = cc3100_reg(A2N_INT_STS_RAW);
        while (!(apps_int_sts_raw & 0x1)) {
            apps_int_sts_raw = cc3100_reg(A2N_INT_STS_RAW);
        }

        /* wait till nwp powers off */
        uint32_t nwp_wakup_ind = cc3100_reg(NWP_LPDS_WAKEUPCFG);
        while (nwp_wakup_ind != NWP_LPDS_WAKEUPCFG_APPS2NWP) {
            nwp_wakup_ind = cc3100_reg(NWP_LPDS_WAKEUPCFG);
        }
    }
    /* Clear Out of band interrupt, Acked by the NWP */
    cc3100_reg(A2N_INT_STS_CLR) = 0x1;

    // mask network processor interrupt interrupt
    mask_nwp_rx_irqn();

    /* Switch to PFM Mode */
    cc3100_reg(ANA_DCDC_PARAMS0) &= ~ANA_DCDC_PARAMS0_PWMOVERRIDE;

    // sl_stop eco for PG1.32 devices
    cc3100_reg(NWP_SPARE_REG_5) |= NWP_SPARE_REG_5_SLSTOP;

    uSEC_DELAY(200);
    DEBUG("[cc31xx] NWP power off completed\n");
}

/**
 * @brief quit all services and power off nwp
 *
 */
// NOTE: probably one of the registers is missaligned right now
void cc3100_nwp_graceful_power_off(void)
{
    DEBUG("%s()\n", __FUNCTION__);
    /* turn of all network services */
    cc3100_reg(COMMON_REG_BASE + ADC_O_ADC_CH_ENABLE) = 1;

    ROM_UtilsDelay(800000 / 5);

    /* check if NWP was powered of or is in some Low Power Deep Sleep state */
    if ((GPRCM->NWP_LPDS_WAKEUP_CFG != 0x20) &&
        !(cc3100_reg(OCP_SHARED_BASE + OCP_SHARED_O_SPARE_REG_5) & 0x2)) {
        uint16_t retry_count = 0;
        /* Loop until APPs->NWP interrupt is cleared or timeout */
        while (retry_count < 1000) {
            /* interrupt gets cleared when NWP has powered on */
            if (!(cc3100_reg(COMMON_REG_BASE + COMMON_REG_O_APPS_INT_STS_RAW) &
                  0x1)) {
                break;
            }
            ROM_UtilsDelay(800000 / 5);
            retry_count++;
        }
    }

    /* Clear APPs to NWP interrupt */
    cc3100_reg(COMMON_REG_BASE + ADC_O_adc_ch7_fifo_lvl) = 1;
    ROM_UtilsDelay(800000 / 5);

    /* power off NWP */
    cc3100_nwp_power_off();
}

/**
 * @brief read data from NWP SPI
 *
 * @param buf read destination buffer
 * @param len length of bytes to be read (must be a multiple of word length)
 * @return int returns bytes read
 */
int cc3100_read_from_nwp(cc3100_t *dev, void *buf, int len)
{
    DEBUG("%s()\n", __FUNCTION__);
    // spi_transfer_bytes(dev->params.spi, SPI_CS_UNDEF, false, NULL, buf, len);
    spi_transfer_bytes(1, SPI_CS_UNDEF, false, NULL, buf, ((len) + 3) & (~3));
    return len;
}

/**
 * @brief send content of buffer buf to the NWP using SPI
 *
 * @param buf buffer to be send
 * @param len length in bytes to be send (must be a multiple of word length)
 * @return int bytes written to SPI
 */
int cc31xx_send_to_nwp(cc3100_t *dev, const void *buf, int len)
{
    DEBUG("%s()\n", __FUNCTION__);
    // spi_transfer_bytes(dev->params.spi, SPI_CS_UNDEF, false, buf, NULL, len);
    spi_transfer_bytes(1, SPI_CS_UNDEF, false, buf, NULL, ((len) + 3) & (~3));
    return len;
}

/**
 * @brief power on nwp, register RX irqn handler and await nwp response
 *
 * @return int
 */
int cc3100_init_nwp(cc3100_t *dev)
{
    DEBUG("%s()\n", __FUNCTION__);

    // register callback when wifi module is powered back on
    cc3100_register_nwp_rx_irqn((cc3100_rx_irqn_handler)cc3100_nwp_rx_handler);

    DEBUG("[cc31xx] powering on\n");

    volatile cc31xx_nwp_req_t r = { .opcode   = 0x0008,
                                    .wait     = true,
                                    .desc_len = 0 };
    cc3100_add_to_drv_queue(&r);

    cc3100_nwp_power_on();

    DEBUG("[cc31xx] waiting for on NWP INIT_COMPLETE...\n");
    while (r.wait) {
        thread_yield();
    }
    DEBUG("[cc31xx] NWP ready for commands!\n");
    return 0;
}

/**
 * @brief cc3100_cmd_handler handles requested driver command responses
 * @param header
 */
void cc3100_cmd_handler(cc3100_t *dev, cc3100_nwp_resp_header_t *header)
{
    DEBUG("%s(): opcode=%x len=%d\n", __FUNCTION__, header->GenHeader.Opcode,
          header->GenHeader.Len);

    volatile cc31xx_nwp_req_t *req = NULL;

    /* check if any command is waiting on a response if so we can use
       its buffer to avoid having to copy the data */
    for (uint8_t i = 0; _nwp_com.cur_len != 0 && i < REQUEST_QUEUE_SIZE; i++) {
        if (_nwp_com.queue[i] == NULL ||
            _nwp_com.queue[i]->opcode != header->GenHeader.Opcode) {
            continue;
        }
        req       = _nwp_com.queue[i];
        req->wait = false;
        cc3100_remove_from_drv_queue(_nwp_com.queue[i]);
    }

    // spi_acquire(1, SPI_CS_UNDEF, SPI_SUB_MODE_0, SPI_CLK_20MHZ);

    /* when we have a request read the buffer to the request */
    if (req != NULL) {
        DEBUG("[cc31xx] drv requests found \n");
        int16_t remainder = header->GenHeader.Len - req->desc_len;
        if (remainder < 0) {
            remainder = 0;
        }
        if (req->desc_len > 0 && remainder >= 0) {
            cc3100_read_from_nwp(dev, req->desc_buf, req->desc_len);
        }

        /* payload can sometimes be smaller then expected */
        if (remainder < req->payload_len) {
            // DEBUG("NWP: Read payload %d bytes \n", (remainder + 3) & ~0x03);
            cc3100_read_from_nwp(dev, req->payload_buf,
                                 (remainder + 3) & ~0x03);
            remainder = 0;
        } else {
            remainder -= req->payload_len;
            // DEBUG("NWP: Read payload %d bytes \n", req->PayloadBufferSize);
            if (req->payload_len > 0 && remainder >= 0) {
                cc3100_read_from_nwp(dev, req->payload_buf, req->payload_len);
            }
        }

        /* read all remaining data */
        if (remainder > 0) {
            cc3100_read_from_nwp(dev, sharedBuffer, remainder);
        }
    }
    // else {
    //     /* otherwise read everything into shared buffer */
    //     cc3100_read_from_nwp(dev, sharedBuffer, header->GenHeader.Len);
    // }

    // spi_release(1);
    unmask_nwp_rx_irqn();
}

/**
 * @brief add driver request to the queue, the queue will be checked after each
 * succeffull driver RX callback
 *
 * @param cc31xx_nwp_req_t
 */
void cc3100_add_to_drv_queue(volatile cc31xx_nwp_req_t *req)
{
    DEBUG("%s(opcode=%x)\n", __FUNCTION__, req->opcode);
    /* wait till the queue is free */
    while (_nwp_com.cur_len >= REQUEST_QUEUE_SIZE) {
        DEBUG("waiting for space in queue...\n");
    }
    _nwp_com.queue[_nwp_com.cur_len] = req;
    _nwp_com.cur_len++;
}

/**
 * @brief remove a driver request from the driver queue in normal operations
 * this happens after a timeout on the command or a NWP response
 *
 * @param cc31xx_nwp_req_t
 * @return uint8_t
 */
uint8_t cc3100_remove_from_drv_queue(volatile cc31xx_nwp_req_t *req)
{
    DEBUG("%s(opcode=%x)\n", __FUNCTION__, req->opcode);
    for (uint8_t i = 0; i < REQUEST_QUEUE_SIZE; i++) {
        if (_nwp_com.queue[i] == req) {
            _nwp_com.queue[i] = NULL;
            _nwp_com.cur_len--;
            return 1;
        }
    }
    return 0;
}

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
 * @brief read command header with sync and TxSeqNum validation
 *
 * @param buf destination buffer
 * @param align
 * @return int
 */
int cc31xx_read_cmd_header(cc3100_t *dev, cc3100_nwp_resp_header_t *buf)
{
    DEBUG("%s(%u)\n", __FUNCTION__, dev->params.spi);
    /* acquire spi */
    // spi_acquire(dev->params.spi, SPI_CS_UNDEF, SPI_SUB_MODE_0,
    // dev->params.spi);
    // spi_acquire(1, SPI_CS_UNDEF, SPI_SUB_MODE_0, SPI_CLK_20MHZ);

    /* write sync pattern to indicate read start */
    cc31xx_send_to_nwp(dev, &CPU_TO_NWP_CNYS_PATTERN.Short, sizeof(uint32_t));
    uint32_t SyncCnt = 0;

    /* read the 4 bytes/1 word from NWP until response sync pattern was found.
     * Since the sync pattern can be surrounded by random trash we compare the
     * buffer at all possible offsets */
    // TODO: remove expensice slice operation on favour of pointer magic
    cc3100_read_from_nwp(dev, buf, 4);
    while (!match_sync_pattern((uint32_t *)buf)) {
        /* if we have sliced 4 bytes read 1 word into the buffer */
        if (0 == (SyncCnt % (uint32_t)4)) {
            cc3100_read_from_nwp(dev, &buf[4], 4);
        }
        ROM_UtilsDelay(100);
        /* move content of buffer by one byte to the left */
        sliceFirstInBuffer((uint8_t *)buf, 8);

        /* increase sync counter to keep track of sliced of bytes */
        SyncCnt++;

        /* fail transmission after no response */
        if (SyncCnt == 40) {
            return 0;
        }
    }

    SyncCnt %= 4;

    // TODO: simplify this step
    if (SyncCnt > 0) {
        *(uint32_t *)buf = *(uint32_t *)&buf[4];
        cc3100_read_from_nwp(dev, &buf[4 - SyncCnt], (uint16_t)SyncCnt);
    } else {
        cc3100_read_from_nwp(dev, buf, 4);
    }

    /* read till sync pattern is found */
    while (match_sync_pattern((uint32_t *)buf)) {
        cc3100_read_from_nwp(dev, buf, 4);
    }

    /* incremenet NWP send counter required for packet validation */
    TxSeqNum++;

    /* Read NWP command header into the buffer */
    cc3100_read_from_nwp(dev, &buf[4], 4);

    /* read reamainder of data on the SPI bus, since NWP will only write new
     * data after all was read/cleared */
    // TODO: this is potentially problematic since it could overwrite command
    // headers, maybe use generic scratch buffer
    cc3100_read_from_nwp(dev, buf,
                         (uint8_t)((SyncCnt > 0) ? (4 - SyncCnt) : 0));

    // spi_release(1);
    return 0;
}

void cc31xx_send_header(cc3100_t *dev, cc3100_nwp_header_t *header)
{
    cc31xx_send_to_nwp(dev, header, sizeof(cc3100_nwp_header_t));
}

/**
 * @brief send a short SYNC command to the NWP. This is send before every
 * command to notify NWP of incoming data
 *
 */
void _send_short_sync(cc3100_t *dev)
{
    cc31xx_send_to_nwp(dev, &CPU_TO_NWP_SYNC_PATTERN.Short, sizeof(uint32_t));
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

/**
 * @brief send a msg to the nwp and block till the response is read
 *
 * @param msg
 * @param res
 * @return uint8_t
 */
uint8_t cc31xx_send_nwp_cmd(cc3100_t *dev, cc31xx_nwp_msg_t *msg,
                            cc31xx_nwp_rsp_t *res)
{
    /* aquire SPI device */
    // spi_acquire(dev->params.spi, SPI_CS_UNDEF, SPI_SUB_MODE_0,
    // dev->params.spi);
    // spi_acquire(1, SPI_CS_UNDEF, SPI_SUB_MODE_0, SPI_CLK_20MHZ);

    DEBUG("\033[0;34m[WIFI]\033[0m SEND CMD: \033[1;33m%x\033[0m\n",
          msg->opcode);
    /* write a short sync frame to indicate sending of data */
    _send_short_sync(dev);

    /* compute the total message size (the header length is should be omitted)
     * because it is always present
     */
    uint16_t desc_len = alignDataLen(msg->payload_hdr_len + msg->payload_len +
                                     msg->desc_len);
    cc3100_nwp_header_t header = { .opcode = msg->opcode, .len = desc_len };
    // add request to the request queue
    volatile cc31xx_nwp_req_t req = { .opcode = msg->resp_opcode != 0 ?
                                                        msg->resp_opcode :
                                                        msg->opcode - 0x8000,
                                      .wait     = true,
                                      .desc_len = res->res_len,
                                      .desc_buf = res->data };

    /* set the request payload buffers to the response buffer addresses */
    if (res->payload != NULL) {
        req.payload_buf = res->payload;
        req.payload_len = res->payload_len;
    }

    /* send the header */
    cc31xx_send_header(dev, &header);

    /* if a response is expected add request to the waiting queue */
    if (res != NULL) {
        cc3100_add_to_drv_queue(&req);
    } else {
        req.wait = false;
    }

    if (msg->desc_len > 0) {
        /* send command descriptions */
        cc31xx_send_to_nwp(dev, msg->desc_buf, alignDataLen(msg->desc_len));
    }

    /* if a payload header is provided send it */
    if (msg->payload_hdr_len > 0) {
        DEBUG("SEND HEADER\n");
        /* send command descriptions */
        cc31xx_send_to_nwp(dev, msg->payload_hdr_buf,
                           alignDataLen(msg->payload_hdr_len));
    }

    /* send payload if provided */
    if (msg->payload_buf != NULL && msg->payload_len > 0) {
        DEBUG("SEND PAYLOAD\n");
        cc31xx_send_to_nwp(dev, msg->payload_buf,
                           alignDataLen(msg->payload_len));
    }
    /* release SPI device */
    // spi_release(dev->params.spi);
    // spi_release(1);

    // wait for message response (rxHandler will copy the value to the res
    // buffer)
    // TODO: handle timeouts
    while (res != NULL && req.wait) {
        thread_yield();
    }
    return 0;
}

/**
 * @brief initiate NWP in its default configuration
 *
 */
uint16_t _nwp_setup(cc3100_t *dev)
{
    /* register handler */
    int16_t err = 0;
    /* delete existing profiles */
    err = _nwp_del_profile(dev, 0xFF);
    if (err != 0) {
        DEBUG("[cc31xx] failed to delete profiles\n");
    }

    if (_nwp_set_wifi_policy(dev, SL_POLICY_SCAN, SL_SCAN_POLICY(0)) != 0) {
        DEBUG("[cc31xx] failed to set scan policy");
    }
    if (_nwp_set_wifi_policy(dev, SL_POLICY_CONNECTION,
                             SL_CONNECTION_POLICY(0, 0, 0, 0, 0)) != 0) {
        DEBUG("[cc31xx] failed to set connect policy");
    }

    // if (_nwp_disconnect(dev) != 0) {
    //     DEBUG("[cc31xx] failed to disconnect \n");
    // }

    // disable DHCP
    uint8_t dhcp_disable = 1;
    if (_nwp_set_net_cfg(dev, 4, 0, 1, &dhcp_disable) != 0) {
        DEBUG("[cc31xx] failed to disable DHCP\n");
    }

    // setup wifi power
    // power is a reverse metric (dB)
    uint8_t tx_power = 0;
    if (_nwp_set_wifi_cfg(dev, 1, 10, 1, &tx_power) != 0) {
        DEBUG("[cc31xx] failed to set NWP TX power\n");
    }

    uint8_t filter_cfg[8] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    // reset rx filters
    if (_nwp_set_wifi_filter(dev, 1, filter_cfg,
                             sizeof(_WlanRxFilterOperationCommandBuff_t) !=
                                     0)) {
        DEBUG("[cc31xx]failed to reset wifi filters\n");
    }
    // get device mac address
    if (_nwp_get_net_cfg(dev, SL_MAC_ADDRESS_GET, NULL, 8, dev->netdev.addr) <
        0) {
        DEBUG("[cc31xx] failed to get MAC-Address\n");
    } else {
        printf("[cc31xx] hardware address: ");
        printMacAddr(dev->netdev.addr);
    }

    /* open RAW socket */
    int16_t sock = _nwp_sock_create(dev, SL_AF_RF, SL_SOCK_RAW, WIFI_CHANNEL);
    if (sock < 0) {
        DEBUG("[cc31xx] failed to open raw socket\n");
    }
    dev->sock_id = sock;
    return 0;
}