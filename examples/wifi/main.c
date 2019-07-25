/*
 * Copyright (C) 2014 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Hello World application
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 * @author      Ludwig Knüpfer <ludwig.knuepfer@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>
#define ENABLE_DEBUG (1)
// #include "board.h"
#include "debug.h"

#include "80211.h"
#include "cmd.h"
#include "driver.h"
#include "periph/cpuid.h"
#include "periph/gpio.h"
#include "proto.h"
#include "setup.h"
#include "state.h"
#include "utils.h"
#include "vendor/rom.h"

/* sockopt */
typedef _u32 SlTime_t;
typedef _u32 SlSuseconds_t;

typedef struct SlTimeval_t {
    SlTime_t tv_sec;       /* Seconds      */
    SlSuseconds_t tv_usec; /* Microseconds */
} SlTimeval_t;

#define RECEIVER (0)
#define WLAN_CHANNEL (13)

/* Address families.  */
#define SL_AF_INET (2)  /* IPv4 socket (UDP, TCP, etc) */
#define SL_AF_INET6 (3) /* IPv6 socket (UDP, TCP, etc) */
#define SL_AF_INET6_EUI_48 (9)
#define SL_AF_RF                                                       \
    (6) /* data include RF parameter, All layer by user (Wifi could be \
           disconnected) */
#define SL_AF_PACKET (17)
/* Protocol families, same as address families.  */
#define SL_PF_INET AF_INET
#define SL_PF_INET6 AF_INET6
#define SL_INADDR_ANY (0) /*  bind any address  */

/* Define some BSD protocol constants.  */
#define SL_SOCK_STREAM (1)   /* TCP Socket */
#define SL_SOCK_DGRAM (2)    /* UDP Socket */
#define SL_SOCK_RAW (3)      /* Raw socket */
#define SL_IPPROTO_TCP (6)   /* TCP Raw Socket */
#define SL_IPPROTO_UDP (17)  /* UDP Raw Socket */
#define SL_IPPROTO_RAW (255) /* Raw Socket */
#define SL_SEC_SOCKET (100)  /* Secured Socket Layer (SSL,TLS) */
#define SL_SO_PHY_ALLOW_ACKS                                          \
    (106)                  /* Enable sending ACKs in transceiver mode \
                            */
#define SL_SOL_PHY_OPT (3) /* Define the PHY option category.    */

#ifdef ENABLE_DEBUG
// unsigned char g_ucDMAEnabled = 0;
void logHardwareVersion(void)
{
    SlVersionFull ver = { 0 };
    getDeviceInfo(&ver);
    printf("[WIFI] CC3100 Build Version "
           "%li.%li.%li.%li.31.%li.%li.%li.%li.%i.%i.%i.%i\n\r",
           ver.NwpVersion[0], ver.NwpVersion[1], ver.NwpVersion[2],
           ver.NwpVersion[3], ver.ChipFwAndPhyVersion.FwVersion[0],
           ver.ChipFwAndPhyVersion.FwVersion[1],
           ver.ChipFwAndPhyVersion.FwVersion[2],
           ver.ChipFwAndPhyVersion.FwVersion[3],
           ver.ChipFwAndPhyVersion.PhyVersion[0],
           ver.ChipFwAndPhyVersion.PhyVersion[1],
           ver.ChipFwAndPhyVersion.PhyVersion[2],
           ver.ChipFwAndPhyVersion.PhyVersion[3]);
}

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

void printDeviceMacAddr(void)
{
    printf("### Device MAC Addr: ");
    printMacAddr(state.macAddr);
}
#endif
/**
 * @brief prepare wifi module to be operational
 *
 */
void init_wifi(void)
{
    gpio_set(LED_RED);
    if (setupWifiModule() != 0) {
        // loop and blink to indicate problem (also helps with debugging)
        while (1) {
            ROM_UtilsDelay(80000 * 50);
            gpio_toggle(LED_RED);
        }
    }

    // light up green and disable red led
    gpio_clear(LED_RED);
    gpio_set(LED_GREEN);

#ifdef ENABLE_DEBUG
    logHardwareVersion();
#endif
}

uint8_t acBuffer[1500];

int16_t prepareWifi(void)
{
    unsigned char ucVal = 0;
    if (setWifiPolicy(SL_POLICY_SCAN, SL_SCAN_POLICY(0)) != 0) {
        puts("[WIFI] failed to set policy");
    } else {
        puts("[WIFI] policy set");
    }
    setWifiPolicy(SL_POLICY_CONNECTION, SL_CONNECTION_POLICY(0, 0, 0, 0, 0));

    if (disconnectFromWifi() != 0) {
        puts("[WIFI] failed to disconnect");
    } else {
        puts("[WIFI] disconnected");
    }

    // disable DHCP
    setNetConfig(4, 1, 1, &ucVal);

    // disable scan
    if (setWifiPolicy(SL_POLICY_SCAN, SL_SCAN_POLICY(0)) != 0) {
        puts("[WIFI] failed to set wifi policy");
    }

    // setup wifi power
    // power is a reverse metric (dB)
    uint8_t wifiPower = 0;
    setWifiConfig(1, 10, 1, &wifiPower);

    uint8_t filterConfig[8] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    // reset rx filters
    setWlanFilter(1, filterConfig, sizeof(_WlanRxFilterOperationCommandBuff_t));

    return 0;
}

#if RECEIVER == 1
int16_t receiveData(void)
{
    int16_t receiveLen = 0;

    // struct SlTimeval_t timeval;
    // timeval.tv_sec = 0;      // Seconds
    // timeval.tv_usec = 20000; // Microseconds.

    // get a socket
    int16_t sock = openSocket(SL_AF_RF, SL_SOCK_DGRAM, WLAN_CHANNEL);
    if (sock < 0) {
        return sock;
    }

    DEBUG("SOCKET OPEN: %i \n", sock);

    while (1) {
        // sendRawTraceiverData(sock0, (uint8_t *)RawData_Ping,
        // sizeof(RawData_Ping), SL_RAW_RF_TX_PARAMS(13, 1, 0, 0));
        receiveLen = recvRawTraceiverData(sock, acBuffer, 1470, 0);
        wifi_80211_baseheader *test = (wifi_80211_baseheader *)(acBuffer + 8);
#ifdef ENABLE_DEBUG
        printf("-> RECV PACKET %d bytes from MAC: ", receiveLen);
        printMacAddr(test->src);

#endif
        ROM_UtilsDelay(4000000);
        gpio_toggle(LED_ORANGE);
    }

    return 0;
}
#endif

static inline bool macAddrMatch(unsigned char *a, unsigned char *b)
{
    /* compare mac addr via subtraction (e.g. zero means equal). First
     * compare last 32bits (most likely to differ) then the first 16 bits */
    return !((uint32_t)a[2] - (uint32_t)b[2]) &&
           !((uint16_t)a[0] - (uint16_t)b[0]);
}

#if !RECEIVER
uint8_t setTagParameter(uint8_t *buf, uint8_t number, void *val, size_t len)
{
    buf[0] = number;
    buf[1] = len;
    memcpy(&buf[2], val, len);
    return len + 2;
}

typedef struct {
    uint8_t rate;
    uint8_t channel;
    int8_t rssi;
    uint8_t padding;
    uint32_t timestamp;
} TransceiverRxOverHead_t;

#define RAW_RX_TI_PAYLOAD_OFFSET 8

void sendFrame(int16_t sock, void *frame, size_t len, uint8_t ch, bool ack)
{
    uint8_t i;
    uint8_t j;
    uint8_t ack_buffer[14 + RAW_RX_TI_PAYLOAD_OFFSET];
    wlan_80211_ack_t *ack_frame =
            (wlan_80211_ack_t *)(ack_buffer + RAW_RX_TI_PAYLOAD_OFFSET);

    /* try resending the packet 8 times */
    for (i = 0; i < 8; i++) {
        /* send data (for some reason the packet needs to be 4 bytes longer
         * then payload) */
        sendRawTraceiverData(sock, frame, len + 4,
                             SL_RAW_RF_TX_PARAMS(ch, 1, 0, 0));
        /* if we dont need an ACK just return  */
        if (!ack) {
            return;
        }
        // check for ACK
        for (j = 0; j < 20; j++) {
            recvRawTraceiverData(sock, ack_buffer,
                                 56 + RAW_RX_TI_PAYLOAD_OFFSET, 0);
            DEBUG("[MAC] Got packet type=%d subtype=%d (%d, %d)  \n",
                  ack_frame->header.fc.bits.type,
                  ack_frame->header.fc.bits.subtype, WL_CTRL, WL_ACK);
            // check for ACK frame
            if (ack_frame->header.fc.bits.type == WL_CTRL &&
                ack_frame->header.fc.bits.subtype == WL_ACK &&
                macAddrMatch(ack_frame->recv, state.macAddr)) {
                DEBUG("GOT ACK \n");

                return;
                // if mac address matches
            }
        }
    }
}

void recvFrame(int16_t sock, void *frame, size_t len, uint8_t ch)
{
    wlan_80211_ack_t ack_frame;
    ack_frame.header.duration = 0x40;
    ack_frame.header.fc.raw   = 0x00;

    /* set frame type */
    ack_frame.header.fc.bits.type    = WL_CTRL;
    ack_frame.header.fc.bits.subtype = WL_ACK;

    /* recv frame into buffer */
    recvRawTraceiverData(sock, frame, len, 0);
    printMacAddr(
            ((wlan_80211_basic_frame_t *)(frame + RAW_RX_TI_PAYLOAD_OFFSET))
                    ->recv);
    if (macAddrMatch(
                ((wlan_80211_basic_frame_t *)(frame + RAW_RX_TI_PAYLOAD_OFFSET))
                        ->recv,
                state.macAddr)) {
        // DEBUG("[MAC] RECV packet for own MAC Addr");
        /* send ack if required by frame type */
        // TODO: check for type, for now always ACK
        memcpy(ack_frame.recv,
               ((wlan_80211_basic_frame_t *)(frame + RAW_RX_TI_PAYLOAD_OFFSET))
                       ->src,
               6);
        /* send ack */
        sendRawTraceiverData(sock, (uint8_t *)&ack_frame,
                             sizeof(wlan_80211_basic_frame_t),
                             SL_RAW_RF_TX_PARAMS(ch, 1, 0, 0));
        DEBUG("[MAC] ACK SEND \n");
    }
}

char *text = "HELLO THERE";

int16_t sendData(void)
{
    // int16_t sendLen = 0;
    // get a socket
    int16_t sock = openSocket(6, 2, WLAN_CHANNEL);
    if (sock < 0) {
        return sock;
    }

    DEBUG("SOCKET OPEN: %i \n", sock);

    // setSocketOptions(sock0, 3, 20, &timeval, sizeof(timeval));
    uint32_t Acks = 1;
    int16_t res   = setSocketOptions(sock, SL_SOL_PHY_OPT, SL_SO_PHY_ALLOW_ACKS,
                                   &Acks, sizeof(Acks));
    DEBUG("SOCKET OPTION RESULT %i \n", res);

    // setSocketOptions(sock0, 1, 20, &timeval, sizeof(timeval));
    uint8_t *buf = acBuffer;
    // TransceiverRxOverHead_t *frameRadioHeader = NULL;
    uint8_t offset = 0;
    // write association request data into buffer
    wlan_80211_association_req_t *req = (wlan_80211_association_req_t *)buf;
    // reset header value
    req->header.fc.raw          = 0x0000;
    req->header.fc.bits.version = 0x00;
    req->header.fc.bits.type    = WL_DATA;
    req->header.fc.bits.subtype = as_req;
    req->header.duration        = 0x190;
    uint8_t recvAddr[6]         = { 0x54, 0xe6, 0xfc, 0x94, 0x5d, 0xb9 };
    memcpy(&req->recv, recvAddr, 6);
    // memcpy(&req->header.transmitter, state.macAddr, 6);
    memcpy(&req->src, state.macAddr, 6);
    memcpy(&req->bssid, recvAddr, 6);

    // set capability info
    req->cap_info = 0x3143;

    // set listen interval
    req->listen_interval = 0x3;

    // compute tag parameter offset
    offset += sizeof(wlan_80211_association_req_t);

    printf("Buffer offset is %d (header size: %d)  \n", offset,
           sizeof(wlan_80211_association_req_t));

    // printf("FC: %x, size: %d", (uint16_t *)(&req->header.fc),
    //    sizeof(wlan_80211_frame_control_t));

    // add association ssid request
    char *ssid = "skynet";
    offset += setTagParameter(&buf[offset], 0, ssid, strlen(ssid));

    // set supported rates
    uint8_t supportedRates[8] = {
        0x82, 0x84, 0x8b, 0x96, 0x0c, 0x12, 0x18, 0x24
    };
    offset += setTagParameter(&buf[offset], 1, supportedRates,
                              sizeof(supportedRates));

    // send association request
    sendFrame(sock, buf, offset, WLAN_CHANNEL, true);
    while (true) {
        // recv response
        recvFrame(sock, acBuffer, 1470, WLAN_CHANNEL);
    }

    ROM_UtilsDelay(400000000);
    gpio_toggle(LED_ORANGE);
    //}

    return 0;
}
#endif

int main(void)
{
    uint8_t cpuid[CPUID_LEN];

    cpuid_get(cpuid);
    printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
    init_wifi();
    puts("wifi init completed !");
    printDeviceMacAddr();

    // configure wifi module
    prepareWifi();
    // char *ssid               = "honeypot";
    // char *pass               = "lurid-jongleur-glom-quit-toilette";
    // WifiProfileConfig apConf = { .common = { .SecType     = 0,
    //                                          .SsidLen     = strlen(ssid),
    //                                          .Priority    = 1,
    //                                          .PasswordLen = strlen(pass)
    //                                          },
    //                              .ssid   = ssid,
    //                              .key    = pass };
    // connect(&apConf);
    // // keept the programm going

    // // wait for a connection
    // while (state.con.connected == 0) {
    //     puts("waiting for connection");
    //     ROM_UtilsDelay(30000 * 80 / 3);
    // }

#if RECEIVER
    receiveData();
#else
    sendData();
#endif

    return 0;
}