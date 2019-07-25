#include "cmd.h"
#include "driver.h"
#include "proto.h"
#include "state.h"
#include "utils.h"
#include "xtimer.h"

#include "vendor/hw_apps_rcm.h"
#include "vendor/hw_common_reg.h"
#include "vendor/hw_gprcm.h"
#include "vendor/hw_hib1p2.h"
#include "vendor/hw_ints.h"
#include "vendor/hw_memmap.h"
#include "vendor/hw_ocp_shared.h"
#include "vendor/hw_types.h"
#include "vendor/hw_udma.h"
#include "vendor/rom.h"

#include "periph/spi.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#define ENABLE_DEBUG (1)
#include "debug.h"

// static bool wifiModuleBooted = false;
static uint8_t handledIrqsCount = 0;

// wifi module register memory map
// volatile CC3200_MCSPI* wifiReg = (struct CC3200_MCSPI *)WIFI_SPI_BASE;

/**
 * @brief utility to get bit rate for a given rom minor version
 *
 */
uint32_t getSPIBitRate(void)
{
    /* Check NWP generation */
    if ((GPRCM->GPRCM_DIEID_READ_REG4 >> 24) & 0x02) {
        /* CC3220 (ver > 1.3.2) */
        return SPI_RATE_20M;
    } else {
        /* CC3200 (ver > 1.3.2) */
        return SPI_RATE_30M;
    }
}

int registerRxInterruptHandler(SimpleLinkEventHandler handler)
{
    ROM_IntRegister(INT_NWPIC, handler);
    ROM_IntPrioritySet(INT_NWPIC, 0x20);
    ROM_IntPendClear(INT_NWPIC);
    ROM_IntEnable(INT_NWPIC);
    return 0;
}

void _clear_wifi_interrupt_handler(void)
{
    ROM_IntDisable(INT_NWPIC);
    ROM_IntUnregister(INT_NWPIC);
    ROM_IntPendClear(INT_NWPIC);

    // TODO: also clear any IO specific parts
}

#define A2N_INT_STS_CLR (COMMON_REG_BASE + COMMON_REG_O_APPS_INT_STS_CLR)
#define uSEC_DELAY(x) (ROM_UtilsDelay(x * 80 / 3))

/* NWP_SPARE_REG_5 - (OCP_SHARED_BASE + OCP_SHARED_O_SPARE_REG_5)
    - Bits 31:02 - Reserved
    - Bits 01    - SLSTOP1 - NWP in Reset, Power Domain Down
    - Bits 00    - Reserved
*/
#define NWP_SPARE_REG_5 (OCP_SHARED_BASE + OCP_SHARED_O_SPARE_REG_5)
#define NWP_SPARE_REG_5_SLSTOP (0x00000002)

/* ANA_DCDC_PARAMS0 - (HIB1P2_BASE + HIB1P2_O_ANA_DCDC_PARAMETERS0)
    - Bits 31:28 - Reserved
    - Bits 27    - Override PWM mode (==> PFM)
    - Bits 26:00 - Reserved
*/
#define ANA_DCDC_PARAMS0 (HIB1P2_BASE + HIB1P2_O_ANA_DCDC_PARAMETERS0)
#define ANA_DCDC_PARAMS0_PWMOVERRIDE (0x08000000)

/* NWP_LPDS_WAKEUPCFG - (GPRCM_BASE + GPRCM_O_NWP_LPDS_WAKEUP_CFG)
    - Bits 31:08 - Reserved
    - Bits 07:00 - WakeUp Config AppsToNwp Wake (0x20) - reset condition
*/
#define NWP_LPDS_WAKEUPCFG (GPRCM_BASE + GPRCM_O_NWP_LPDS_WAKEUP_CFG)
#define NWP_LPDS_WAKEUPCFG_APPS2NWP (0x00000020)
#define NWP_LPDS_WAKEUPCFG_TIMEOUT_MSEC (600)

#define N2A_INT_ACK (COMMON_REG_BASE + COMMON_REG_O_NW_INT_ACK)
/* WAKENWP - (ARCM_BASE + APPS_RCM_O_APPS_TO_NWP_WAKE_REQUEST)
    - Bits 31:01 - Reserved
    - Bits 00    - Wake Request to NWP
*/
#define WAKENWP_WAKEREQ \
    (APPS_RCM_APPS_TO_NWP_WAKE_REQUEST_APPS_TO_NWP_WAKEUP_REQUEST)

/* A2N_INT_STS_CLR -                    (COMMON_REG_BASE +
 * COMMON_REG_O_APPS_INT_STS_CLR)  */
#define A2N_INT_STS_CLR (COMMON_REG_BASE + COMMON_REG_O_APPS_INT_STS_CLR)
/* A2N_INT_TRIG -                       (COMMON_REG_BASE +
 * COMMON_REG_O_APPS_INT_TRIG)     */
#define A2N_INT_TRIG (COMMON_REG_BASE + COMMON_REG_O_APPS_INT_TRIG)
/* A2N_INT_STS_RAW -                    (COMMON_REG_BASE +
 * COMMON_REG_O_APPS_INT_STS_RAW)  */
#define A2N_INT_STS_RAW (COMMON_REG_BASE + COMMON_REG_O_APPS_INT_STS_RAW)

void waitForNWPPowerOff(void)
{
    volatile unsigned long nwp_wakup_ind = HWREG(NWP_LPDS_WAKEUPCFG);
    while (nwp_wakup_ind != NWP_LPDS_WAKEUPCFG_APPS2NWP) {
        nwp_wakup_ind = HWREG(NWP_LPDS_WAKEUPCFG);
    }

    return;
}

void powerOffWifi(void)
{
    volatile unsigned long apps_int_sts_raw;
    volatile unsigned long sl_stop_ind       = HWREG(NWP_SPARE_REG_5);
    volatile unsigned long nwp_lpds_wake_cfg = HWREG(NWP_LPDS_WAKEUPCFG);
    if ((nwp_lpds_wake_cfg != NWP_LPDS_WAKEUPCFG_APPS2NWP) && /* Check for NWP
                                                                 POR condition -
                                                                 APPS2NWP is
                                                                 reset condition
                                                               */
        !(sl_stop_ind & NWP_SPARE_REG_5_SLSTOP)) /* Check if sl_stop was
                                                    executed */
    {
        HWREG(0xE000E104) = 0x200; /* Enable the out of band interrupt, this is
                                      not a wake-up source*/
        HWREG(A2N_INT_TRIG) = 0x1; /* Trigger out of band interrupt  */
        ARCM->APPS_TO_NWP_WAKE_REQUEST = WAKENWP_WAKEREQ; /* Wake-up the NWP */

        // _SlDrvStartMeasureTimeout(&SlTimeoutInfo,
        // NWP_N2A_INT_ACK_TIMEOUT_MSEC);

        /* Wait for the A2N_INT_TRIG to be cleared by the NWP to indicate it's
         * awake and ready for shutdown. poll until APPs->NWP interrupt is
         * cleared or timeout : for service pack 3.1.99.1 or higher, this
         * condition is fulfilled in less than 1 mSec.
         * Otherwise, in some cases it may require up to 3000 mSec of waiting.
         */

        apps_int_sts_raw = HWREG(A2N_INT_STS_RAW);
        while (!(apps_int_sts_raw & 0x1)) {
            apps_int_sts_raw = HWREG(A2N_INT_STS_RAW);
        }

        waitForNWPPowerOff();
    }
    /* Clear Out of band interrupt, Acked by the NWP */
    HWREG(A2N_INT_STS_CLR) = 0x1;

    // mask network processor interrupt interrupt
    maskWifiInterrupt();

    /* Switch to PFM Mode */
    HWREG(ANA_DCDC_PARAMS0) &= ~ANA_DCDC_PARAMS0_PWMOVERRIDE;

    // sl_stop eco for PG1.32 devices
    HWREG(NWP_SPARE_REG_5) |= NWP_SPARE_REG_5_SLSTOP;

    uSEC_DELAY(200);
    DEBUG("POWER OFF COMPLETED");
}

void powerOnWifi(void)
{
    /* bring the 1.32 eco out of reset */
    HWREG(NWP_SPARE_REG_5) &= ~NWP_SPARE_REG_5_SLSTOP;

    /* Clear host IRQ indication */
    HWREG(N2A_INT_ACK) = 1;

    /* NWP Wake-up */
    ARCM->APPS_TO_NWP_WAKE_REQUEST = WAKENWP_WAKEREQ;

    // UnMask Host interrupt
    unmaskWifiInterrupt();

    DEBUG("POWER ON COMPLETED \n");
}

static uint8_t sharedBuffer[512];
typedef struct {
    _u8 connection_type; /* 0-STA,3-P2P_CL */
    _u8 ssid_len;
    _u8 ssid_name[32];
    _u8 go_peer_device_name_len;
    _u8 go_peer_device_name[32];
    _u8 bssid[6];
    _u8 reason_code;
    _u8 padding[2];
} slWlanConnectAsyncResponse_t;

typedef struct {
    _u32 ip;
    _u32 gateway;
    _u32 dns;
} SlIpV4AcquiredAsync_t;

void handleWlanConnectedResponse(void)
{
    slWlanConnectAsyncResponse_t *test =
            (slWlanConnectAsyncResponse_t *)sharedBuffer;
#if ENABLE_DEBUG
    printf("[WIFI] connected to SSID ");
    printChars((char *)test->ssid_name, test->ssid_len);
    printf("\n");
#endif

    // store values on the current state
    state.con.type    = test->connection_type;
    state.con.ssidLen = test->ssid_len;

    // copy ssid
    memcpy(state.con.ssid, test->ssid_name, test->ssid_len);

    // copy bssid
    memcpy(state.con.bssid, test->bssid, 6);

    state.con.connected = true;
}

void handleIpAcquired(void)
{
    SlIpV4AcquiredAsync_t *test = (SlIpV4AcquiredAsync_t *)sharedBuffer;
    uint8_t *octets             = (uint8_t *)&test->ip;
    DEBUG("[WIFI] acquired IP %u.%u.%u.%u \n", octets[3], octets[2], octets[1],
          octets[0]);
}

void defaultCommandHandler(cc3200_SlResponseHeader *header)
{
    DEBUG("[WIFI] RECV: \033[1;32m%x \033[0m, len=%d\n",
          header->GenHeader.Opcode, header->GenHeader.Len);

    volatile DriverRequest *req = NULL;

    // check if any command is waiting on a response if so we can use
    // its buffer to avoid having to copy the data
    for (uint8_t i = 0; state.curReqCount != 0 && i < REQUEST_QUEUE_SIZE; i++) {
        if (state.requestQueue[i] == NULL ||
            state.requestQueue[i]->Opcode != header->GenHeader.Opcode) {
            continue;
        }
        req          = state.requestQueue[i];
        req->Waiting = false;
        removeFromQueue(state.requestQueue[i]);
    }

    // when we have a request read the buffer to the request
    if (req != NULL) {
        int16_t remainder = header->GenHeader.Len - req->DescBufferSize;
        if (remainder < 0) {
            remainder = 0;
        }
        if (req->DescBufferSize > 0 && remainder >= 0) {
            read(req->DescBuffer, req->DescBufferSize);
        }

        // payload can sometimes be smaller then expected
        if (remainder < req->PayloadBufferSize) {
            // DEBUG("NWP: Read payload %d bytes \n", (remainder + 3) & ~0x03);
            read(req->PayloadBuffer, (remainder + 3) & ~0x03);
            remainder = 0;
        } else {
            remainder -= req->PayloadBufferSize;
            // DEBUG("NWP: Read payload %d bytes \n", req->PayloadBufferSize);
            if (req->PayloadBufferSize > 0 && remainder >= 0) {
                read(req->PayloadBuffer, req->PayloadBufferSize);
            }
        }

        // read all remaining data
        if (remainder > 0) {
            read(sharedBuffer, remainder);
        }
    } else {
        // otherwise read everything into shared buffer;
        read(sharedBuffer, header->GenHeader.Len);
    }

    // handle commands
    switch (header->GenHeader.Opcode) {
    case SL_OPCODE_WLAN_WLANASYNCCONNECTEDRESPONSE:
        handleWlanConnectedResponse();
        break;
    case SL_OPCODE_NETAPP_IPACQUIRED:
        handleIpAcquired();
        break;
    }

    unmaskWifiInterrupt();
}

/**
 * @brief
 *
 */
void wifiRxHandler(void *value)
{
    DEBUG("RX HANDLER!!!\n");
    (void)value;
    // if (value != NULL) {
    //   puts(value);
    // }
    handledIrqsCount++;
    maskWifiInterrupt();
    // initWifiModule is waiting for the setup command so
    // only use default handler after that

    cc3200_SlResponseHeader cmd;
    readCmdHeader(&cmd);
    defaultCommandHandler(&cmd);

    cortexm_isr_end();
}

//
// Channel 12
//
#define UDMA_CH12_LSPI_RX 0x0000000C
#define UDMA_CH12_SW 0x0003000C

//
// Channel 13
//
#define UDMA_CH13_LSPI_TX 0x0000000D
#define UDMA_CH13_SW 0x0003000D

/**
 * @brief
 *
 */
int initWifiModule(void)
{
    // register callback when wifi module is powered back on
    registerRxInterruptHandler((SimpleLinkEventHandler)wifiRxHandler);

    // disable uDMA channels
    DEBUG("POWER ON!!!! \n");

    // ROM_uDMAChannelDisable(UDMA_CH12_LSPI_RX);
    // ROM_uDMAChannelDisable(UDMA_CH13_LSPI_TX);

    powerOnWifi();
    uint8_t buf[256];
    memset(buf, 0, sizeof(buf));
    DEBUG("BEFORE? %d \n", ((_SlGenericHeader_t *)buf)->Opcode);
    read(buf, 256);
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 16; j++) {
            printf("%02x ", buf[i * 16 + j]);
        }
        printf("\n");
    }
    DEBUG("HEADER? %x \n", ((_SlGenericHeader_t *)buf)->Opcode);
    volatile DriverRequest r = { .Opcode         = 0x0008,
                                 .Waiting        = true,
                                 .DescBufferSize = 0 };
    addToQueue(&r);
    DEBUG("WAITING FOR RETURN \n");
    DEBUG(".");
    while (r.Waiting) {
        // DEBUG("WHAT WHAT WHAT WHAT \n");
    }
    puts("[WIFI] setup completed");
    return 0;
}

/**
 * @brief init wifi SPI and return FD
 *
 * @return int
 */
int initWifiSPI(void)
{
    // TODO: later remove check when replaced calls
    // get rom version.
    uint32_t spiBitRate = 0;

    spiBitRate = getSPIBitRate();
    if (spiBitRate == 0) {
        return -1;
    }
    DEBUG("GOT SPI BITRATE %lu \n", spiBitRate);
    spi_acquire(1, SPI_CS_UNDEF, SPI_SUB_MODE_0, spiBitRate);
    DEBUG("ACQUIRED SPI \n");

    return 0;
}

int setupWifiModule(void)
{
    // DEBUG("sending power on preamble \n");
    // sendPowerOnPreamble();
    DEBUG("init wifi spi \n");
    if (initWifiSPI() != 0) {
        puts("failed to init wifi spi module");
        return -1;
    }
    DEBUG("init NWP \n");
    if (initWifiModule() != 0) {
        puts("failed to start wifi module");
        return -1;
    }
    DEBUG("setting wifi mode \n");
    // 2 = AP mode, 0 = Station
    if (setWifiMode(0) != 0) {
        puts("failed to set wifi mode");
        return -1;
    }

    // getDevice mac address

    // get mac address
    getNetConfig(SL_MAC_ADDRESS_GET, NULL, 8, (unsigned char *)state.macAddr);

    // sendPowerOnPreamble();
    // if (initWifiModule() != 0) {
    //   puts("failed to start wifi module");
    //   return -1;
    // }
    return 0;
}