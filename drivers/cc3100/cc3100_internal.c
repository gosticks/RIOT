#include "vendor/hw_adc.h"
#include "vendor/hw_apps_rcm.h"
#include "vendor/hw_gprcm.h"
#include "vendor/hw_hib1p2.h"
#include "vendor/hw_ocp_shared.h"
#include "vendor/rom.h"

#include "cpu.h"

#include "include/cc3100_internal.h"
#include "include/cc3100_protocol.h"

#include <stdbool.h>
#include <stddef.h>

#define ENABLE_DEBUG (1)
#include "debug.h"

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

/* WAKENWP - (ARCM_BASE + APPS_RCM_O_APPS_TO_NWP_WAKE_REQUEST)
    - Bits 31:01 - Reserved
    - Bits 00    - Wake Request to NWP
*/
#define WAKENWP_WAKEREQ \
    (APPS_RCM_APPS_TO_NWP_WAKE_REQUEST_APPS_TO_NWP_WAKEUP_REQUEST)

// TODO: finetune this shared buffer and maybe move it to dev
static uint8_t sharedBuffer[512];

// TODO: maybe merge with dev or params of the driver
cc3100_drv_state_t state = { .requestQueue = { NULL },
                             .curReqCount  = 0,
                             .con          = { .connected = 0 } };

/* forward declarations */
void cc3100_add_to_drv_queue(volatile cc3100_drv_req_t *req);
uint8_t cc3100_remove_from_drv_queue(volatile cc3100_drv_req_t *req);
void cc3100_nwp_rx_handler(void *value);
void cc3100_cmd_handler(cc3100_nwp_resp_header_t *header);

/**
 * @brief mask and unmask NWP data interrupt
 *
 */
static inline void mask_nwp_rx_irqn(void)
{
    (*(unsigned long *)N2A_INT_MASK_SET) = 0x1;
}
static inline void unmask_nwp_rx_irqn(void)
{
    (*(unsigned long *)N2A_INT_MASK_CLR) = 0x1;
}

/**
 * @brief register irqn handler
 *
 * @param handler to be registered
 */
static inline void cc3100_register_nwp_rx_irqn(cc3100_rx_irqn_handler handler)
{
    ROM_IntRegister(INT_NWPIC, handler);
    ROM_IntPrioritySet(INT_NWPIC, 0x20);
    ROM_IntPendClear(INT_NWPIC);
    ROM_IntEnable(INT_NWPIC);
}

/**
 * @brief cc3100_unregister_nwp_rx_irqn unregisters NWP rx irqn handler
 *
 */
static inline void cc3100_unregister_nwp_rx_irqn(void)
{
    ROM_IntDisable(INT_NWPIC);
    ROM_IntUnregister(INT_NWPIC);
    ROM_IntPendClear(INT_NWPIC);
}

void cc3100_nwp_power_on(void)
{
    /* bring the 1.32 eco out of reset */
    cc3100_reg(NWP_SPARE_REG_5) &= ~NWP_SPARE_REG_5_SLSTOP;

    /* Clear host IRQ indication */
    cc3100_reg(N2A_INT_ACK) = 1;
    /* NWP Wake-up */
    ARCM->APPS_TO_NWP_WAKE_REQUEST = WAKENWP_WAKEREQ;

    // UnMask Host interrupt
    unmask_nwp_rx_irqn();

    DEBUG("POWER ON COMPLETED \n");
}

void cc3100_nwp_power_off(void)
{
    volatile unsigned long apps_int_sts_raw;
    volatile unsigned long sl_stop_ind       = cc3100_reg(NWP_SPARE_REG_5);
    volatile unsigned long nwp_lpds_wake_cfg = cc3100_reg(NWP_LPDS_WAKEUPCFG);
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
        volatile unsigned long nwp_wakup_ind = cc3100_reg(NWP_LPDS_WAKEUPCFG);
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
    DEBUG("POWER OFF COMPLETED");
}

/**
 * @brief quit all services and power off nwp
 *
 */
// NOTE: probably one of the registers is missaligned right now
void cc3100_nwp_graceful_power_off(void)
{
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
 * @brief power on nwp, register RX irqn handler and await nwp response
 *
 * @return int
 */
int cc3100_init_nwp(void)
{
    // register callback when wifi module is powered back on
    // cc3100_register_nwp_rx_irqn((cc3100_rx_irqn_handler)cc3100_nwp_rx_handler);

    // disable uDMA channels
    DEBUG("[NWP] powering on\n");

    volatile cc3100_drv_req_t r = { .Opcode         = 0x0008,
                                    .Waiting        = true,
                                    .DescBufferSize = 0 };
    cc3100_add_to_drv_queue(&r);

    cc3100_nwp_power_on();
    DEBUG("[WIFI] waiting for NWP power on response\n");
    // TODO: add a timeout
    while (r.Waiting) {
    }
    DEBUG("[WIFI] NWP booted\n");
    return 0;
}

/**
 * @brief cc3100_cmd_handler handles requested driver command responses
 * @param header
 */
void cc3100_cmd_handler(cc3100_nwp_resp_header_t *header)
{
    DEBUG("[NWP] RECV: \033[1;32m%x \033[0m, len=%d\n",
          header->GenHeader.opcode, header->GenHeader.len);

    volatile cc3100_drv_req_t *req = NULL;

    // check if any command is waiting on a response if so we can use
    // its buffer to avoid having to copy the data
    for (uint8_t i = 0; state.curReqCount != 0 && i < REQUEST_QUEUE_SIZE; i++) {
        if (state.requestQueue[i] == NULL ||
            state.requestQueue[i]->Opcode != header->GenHeader.opcode) {
            continue;
        }
        req          = state.requestQueue[i];
        req->Waiting = false;
        cc3100_remove_from_drv_queue(state.requestQueue[i]);
    }

    // when we have a request read the buffer to the request
    if (req != NULL) {
        int16_t remainder = header->GenHeader.len - req->DescBufferSize;
        if (remainder < 0) {
            remainder = 0;
        }
        if (req->DescBufferSize > 0 && remainder >= 0) {
            cc3100_read_from_nwp(req->DescBuffer, req->DescBufferSize);
        }

        // payload can sometimes be smaller then expected
        if (remainder < req->PayloadBufferSize) {
            // DEBUG("NWP: Read payload %d bytes \n", (remainder + 3) & ~0x03);
            cc3100_read_from_nwp(req->PayloadBuffer, (remainder + 3) & ~0x03);
            remainder = 0;
        } else {
            remainder -= req->PayloadBufferSize;
            // DEBUG("NWP: Read payload %d bytes \n", req->PayloadBufferSize);
            if (req->PayloadBufferSize > 0 && remainder >= 0) {
                cc3100_read_from_nwp(req->PayloadBuffer,
                                     req->PayloadBufferSize);
            }
        }

        // read all remaining data
        if (remainder > 0) {
            cc3100_read_from_nwp(sharedBuffer, remainder);
        }
    } else {
        // otherwise read everything into shared buffer;
        cc3100_read_from_nwp(sharedBuffer, header->GenHeader.len);
    }

    // handle commands
    // switch (header->GenHeader.Opcode) {
    // case SL_OPCODE_WLAN_WLANASYNCCONNECTEDRESPONSE:
    //     handleWlanConnectedResponse();
    //     break;
    // case SL_OPCODE_NETAPP_IPACQUIRED:
    //     handleIpAcquired();
    //     break;
    // }

    unmask_nwp_rx_irqn();
}

/**
 * @brief add driver request to the queue, the queue will be checked after each
 * succeffull driver RX callback
 *
 * @param cc3100_drv_req_t
 */
void cc3100_add_to_drv_queue(volatile cc3100_drv_req_t *req)
{
    // wait till the queue is free
    while (state.curReqCount >= REQUEST_QUEUE_SIZE) {
    }
    state.requestQueue[state.curReqCount] = req;
    state.curReqCount++;
}

/**
 * @brief remove a driver request from the driver queue in normal operations
 * this happens after a timeout on the command or a NWP response
 *
 * @param cc3100_drv_req_t
 * @return uint8_t
 */
uint8_t cc3100_remove_from_drv_queue(volatile cc3100_drv_req_t *req)
{
    for (uint8_t i = 0; i < REQUEST_QUEUE_SIZE; i++) {
        if (state.requestQueue[i] == req) {
            state.requestQueue[i] = NULL;
            state.curReqCount--;
            return 1;
        }
    }
    return 0;
}
