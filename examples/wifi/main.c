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
#include "debug.h"
#include "xtimer.h"

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

#define RECEIVER (1)
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
#define RAW_RX_TI_PAYLOAD_OFFSET 8

static uint8_t acBuffer[1500];

#ifdef ENABLE_DEBUG
// unsigned char g_ucDMAEnabled = 0;
void logHardwareVersion(void)
{
    SlVersionFull ver = { 0 };
    getDeviceInfo(&ver);
    puts("Netword Module INFO:");
    printf("\t CHIP: 0x%lx \n", ver.ChipId);
    printf("\t MAC:  %d.%d.%d.%d\n", ver.FwVersion[0], ver.FwVersion[1],
           ver.FwVersion[2], ver.FwVersion[3]);
    printf("\t PHY:  %d.%d.%d.%d\n", ver.PhyVersion[0], ver.PhyVersion[1],
           ver.PhyVersion[2], ver.PhyVersion[3]);
    printf("\t NWP:  %d.%d.%d.%d\n", ver.NwpVersion[0], ver.NwpVersion[1],
           ver.NwpVersion[2], ver.NwpVersion[3]);
    printf("\t ROM:  %d\n", ver.RomVersion);
    // printf("[WIFI] CC3100 Build Version "
    //        "%li.%li.%li.%li.31.%li.%li.%li.%li.%i.%i.%i.%i\n\r",
    //        ver.NwpVersion[0], ver.NwpVersion[1], ver.NwpVersion[2],
    //        ver.NwpVersion[3], ver.ChipFwAndPhyVersion.FwVersion[0],
    //        ver.ChipFwAndPhyVersion.FwVersion[1],
    //        ver.ChipFwAndPhyVersion.FwVersion[2],
    //        ver.ChipFwAndPhyVersion.FwVersion[3],
    //        ver.ChipFwAndPhyVersion.PhyVersion[0],
    //        ver.ChipFwAndPhyVersion.PhyVersion[1],
    //        ver.ChipFwAndPhyVersion.PhyVersion[2],
    //        ver.ChipFwAndPhyVersion.PhyVersion[3]);
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

int16_t prepareWifi(void)
{
    unsigned char ucVal = 1;
    int16_t status      = 0;

    /* disconnect from network */
    status = deleteProfile(0xFF);
    if (status != 0) {
        DEBUG("ERR: failed to delete Wifi connection profiles (%d)\n", status);
    }
    if (setWifiPolicy(SL_POLICY_SCAN, SL_SCAN_POLICY(0)) != 0) {
        puts("[WIFI] failed to set scan policy");
    } else {
        puts("[WIFI] policy set");
    }
    status = setWifiPolicy(SL_POLICY_CONNECTION,
                           SL_CONNECTION_POLICY(0, 0, 0, 0, 0));
    if (status != 0) {
        DEBUG("failed to set wifi scan policy (%d)\n", status);
    }
    status = disconnectFromWifi();
    if (status != 0) {
        DEBUG("[WIFI] failed to disconnect (%d)\n", status);
    }

    // disable DHCP
    status = setNetConfig(4, 0, 1, &ucVal);
    if (status != 0) {
        DEBUG("ERR: failed to disable DHCP (%d)\n", status);
    }
    // disable scan
    if (setWifiPolicy(SL_POLICY_SCAN, SL_SCAN_POLICY(0)) != 0) {
        puts("[WIFI] failed to set wifi policy");
    }

    // setup wifi power
    // power is a reverse metric (dB)
    uint8_t wifiPower = 0;
    status            = setWifiConfig(1, 10, 1, &wifiPower);
    if (status != 0) {
        DEBUG("ERR: failed to set WIFI power (%d)\n", status);
    }

    uint8_t filterConfig[8] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    // reset rx filters
    status = setWlanFilter(1, filterConfig,
                           sizeof(_WlanRxFilterOperationCommandBuff_t));
    if (status != 0) {
        DEBUG("ERR: failed to reset wifi filters (%d)\n", status);
    }

    return 0;
}

static inline bool macAddrMatch(unsigned char *a, unsigned char *b)
{
    /* compare mac addr via subtraction (e.g. zero means equal). First
     * compare last 32bits (most likely to differ) then the first 16 bits */
    return !((uint32_t)a[2] - (uint32_t)b[2]) &&
           !((uint16_t)a[0] - (uint16_t)b[0]);
}

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

uint8_t sendFrame(int16_t sock, void *frame, size_t len, uint8_t ch, bool ack)
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
            return 0;
        }
        // check for ACK
        for (j = 0; j < 20; j++) {
            recvRawTraceiverData(sock, ack_buffer,
                                 56 + RAW_RX_TI_PAYLOAD_OFFSET, 0);
            // DEBUG("[MAC] Got packet type=%d subtype=%d (%d, %d)  \n",
            //   ack_frame->header.fc.bits.type,
            //   ack_frame->header.fc.bits.subtype, WL_CTRL, WL_ACK);
            // check for ACK frame
            if (ack_frame->header.fc.bits.type == WL_CTRL &&
                ack_frame->header.fc.bits.subtype == WL_ACK &&
                macAddrMatch(ack_frame->recv, state.macAddr)) {
                DEBUG("GOT ACK \n");

                return 0;
                // if mac address matches
            }
        }
    }
    return 1;
}

/* recvFrame reads the next incoming MAC frame from NWP, 0 is retuned if MAC
 * Addr matches device, -1 if not. */
int16_t recvFrame(int16_t sock, void *frame, size_t len, uint8_t ch, bool ack)
{
    wlan_80211_ack_t ack_frame;
    ack_frame.header.duration = 0x40;
    ack_frame.header.fc.raw   = 0x00;

    /* set frame type */
    ack_frame.header.fc.bits.type    = WL_CTRL;
    ack_frame.header.fc.bits.subtype = WL_ACK;

    /* recv frame into buffer */
    recvRawTraceiverData(sock, frame, len, 0);

    if (!macAddrMatch(
                ((wlan_80211_basic_frame_t *)(frame + RAW_RX_TI_PAYLOAD_OFFSET))
                        ->recv,
                state.macAddr)) {
        return -1;
    }

    /* send ack if required by frame type */
    if (ack) {
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
    return 0;
}

int16_t recvOwnFrame(int16_t sd, void *frame, size_t len, uint8_t ch, bool ack,
                     int16_t retries)
{
    for (uint8_t i = 0; i < retries; i++) {
        if (recvFrame(sd, frame, len, ch, ack) == 0) {
            return 0;
        }
    }
    return -1;
}

#define SL_IPPROTO_IP (2)  /* Define the IP option category.     */
#define SL_IP_HDRINCL (67) /* Raw socket IPv4 header included. */

/* Forward declarations */
int16_t start_pairing(int16_t sd);

int main(void)
{
    int16_t status = 0;
    printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
    init_wifi();
    puts("wifi init completed !");
    printDeviceMacAddr();
    deleteProfile(0);
    deleteProfile(1);
    // // configure wifi module
    // prepareWifi();
    // char *ssid = "skynet";
    // // char *pass = "qwerty1234";
    // WifiProfileConfig apConf = { .common = { .SecType     = SEC_TYPE_OPEN,
    //                                          .SsidLen     = strlen(ssid),
    //                                          .Priority    = 1,
    //                                          .PasswordLen = 0,
    //                                           },
    //                                           .key = NULL,
    //                              .ssid   = ssid,
    // };
    // int16_t profileIndex = profileAdd(&apConf);
    // DEBUG("Profile index %i \n", profileIndex);

    // // try getting profile
    // status = getProfile(profileIndex);
    // if (status != 0) {
    //     DEBUG("ERR: failed to get wifi profile \n");
    // }
    // status = connect(&apConf);
    // if (status != 0) {
    //     DEBUG("ERR: failed to send connect request \n");
    // }
    // printf("[WIFI] waiting for connection to %s", ssid);
    // // wait for a connection
    // while (!state.con.connected) {
    //     DEBUG(".");
    //     USEC_DELAY(1000);
    // }
    // int enableHeader = 1;
    // int16_t wifi_socket_handle =
    //         openSocket(SL_AF_INET, SL_SOCK_RAW, SL_IPPROTO_TCP);
    // DEBUG("OPEN SOCKET %d \n", wifi_socket_handle);
    // setSocketOptions(wifi_socket_handle, SL_IPPROTO_IP, SL_IP_HDRINCL,
    //                  &enableHeader, sizeof(enableHeader));
    // char *test = "TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST
    // TEST"; SlSockAddr_t addr = { 0 }; addr.sa_family    = SL_AF_INET6;
    // addr.sa_data[0]   = 0x00;
    // addr.sa_data[1]   = 0x00;
    // // addr.
    // while (true) {
    //     status = sendTo(wifi_socket_handle, test, strlen(test), 0, &addr,
    //                     sizeof(addr));
    //     DEBUG("SEND S*** (%d)\n", status);
    //     // xtimer_sleep(2);
    //     USEC_DELAY(9000000);
    // }
    int16_t sd = openSocket(SL_AF_RF, SL_SOCK_RAW, WLAN_CHANNEL);
    if (sd < 0) {
        return sd;
    }
    // start broadcasting connection signal
    status = start_pairing(sd);
    if (status != 0) {
        DEBUG("ERR: failed to pair \n");
    }
    DEBUG("Pairing completed \n");

    return 0;
}

typedef struct test_beacon_frame {
    wlan_80211_header_common_t header;
    unsigned char recv[6];  // receiver MAC
    unsigned char src[6];   // source MAC
    unsigned char bssid[6]; // bssid MAC
    uint16_t len;           // padding
} test_beacon_frame;

typedef struct test_beacon_response {
    wlan_80211_header_common_t header;
    unsigned char recv[6]; // receiver MAC
    unsigned char src[6];  // source MAC
    uint8_t magic;         // a magic number full of magic
} test_beacon_response;

#define CUSTOM_PROTO_VERSION \
    3 /**< unused MAC version 3 to identify our custom protocol */

#define BEACON_BUF_SIZE RAW_RX_TI_PAYLOAD_OFFSET + sizeof(test_beacon_frame)

int16_t start_pairing(int16_t sd)
{
    bool connected = false;
    /* beacon starts at RAW_RX_TI_PAYLOAD_OFFSET offset, since TI prefixes
     * metric info before every data frame */
    while (!connected) {
        /* make sure buffer is clear */
        memset(acBuffer, 0, BEACON_BUF_SIZE);
#if RECEIVER

        /* listen for incoming beacons */
        for (int16_t i = 0; i < 100; i++) {
            DEBUG("[WLP]: listening for beacons (%d/100) \n", i + 1);
            /* read network frame into buffer (driver takes care of reading only
             * BEACON_BUF_SIZE or less of incoming frame) */
            recvRawTraceiverData(sd, acBuffer, 1400, 0);
            test_beacon_frame *beacon =
                    (test_beacon_frame *)(acBuffer + RAW_RX_TI_PAYLOAD_OFFSET);
            // DEBUG("TO ");
            // printMacAddr(beacon->recv);
            // DEBUG("FROM ");
            // printMacAddr(beacon->src);
            // DEBUG("TEST: VER: %d TYPE: %d SUBTYPE %d, %p, %p \n",
            //       beacon->header.fc.bits.version,
            //       beacon->header.fc.bits.type,
            //       beacon->header.fc.bits.subtype, beacon, acBuffer);
            /* check for custom protocol header */
            if (beacon->header.fc.bits.version == CUSTOM_PROTO_VERSION &&
                beacon->header.fc.bits.type == WL_DATA &&
                beacon->header.fc.bits.subtype == WL_BEACON) {
                // DEBUG("OK: found mesh partner \n");
                test_beacon_response *res   = (void *)acBuffer;
                res->header.fc.raw          = 0;
                res->header.fc.bits.version = CUSTOM_PROTO_VERSION;
                res->header.fc.bits.type    = WL_DATA;
                res->header.fc.bits.subtype = WL_BEACON;

                /* copy dest addr */
                memcpy(&res->recv, beacon->src, 6);
                /* copy device mac address */
                memcpy(&res->src, state.macAddr, 6);

                res->magic = 27;

                /* send response */
                if (sendFrame(sd, res, sizeof(test_beacon_response),
                              WLAN_CHANNEL, true) != 0) {
                    DEBUG("ERR: failed to send Association response.");
                }
                return 0;
            }
        }

#else

        test_beacon_frame *beacon = (test_beacon_frame *)acBuffer;
        uint8_t respBuf[sizeof(test_beacon_response) + RAW_RX_TI_PAYLOAD_OFFSET];
        char *data     = "FUCK FUCK FUCK";
        size_t dataLen = 0;

        memset(beacon, 0, BEACON_BUF_SIZE);
        // DEBUG("[WLP]: sending beacons (%d/20) \n", i + 1);
        beacon->header.fc.raw          = 0x00;
        beacon->header.fc.bits.version = CUSTOM_PROTO_VERSION;
        beacon->header.fc.bits.type    = WL_DATA;
        beacon->header.fc.bits.subtype = WL_BEACON;
        beacon->header.duration        = 0x40;
        beacon->len                    = strlen(data);
        dataLen += sizeof(test_beacon_frame) + beacon->len;
        memcpy((void *)(beacon + beacon->len), data, beacon->len);
        // /* copy device mac address */
        memcpy(&beacon->src, state.macAddr, 6);
        DEBUG("SENDING %d bytes \n", dataLen);
        // /* set dest to broadcast */
        memset(&beacon->recv, 0xFF, 6);
        /* if no beacon found send some beacons */
        for (uint16_t i = 0; i < 1000; i++) {
            sendFrame(sd, beacon, dataLen, WLAN_CHANNEL, false);
            recvOwnFrame(sd, respBuf,
                         sizeof(test_beacon_response) +
                                 RAW_RX_TI_PAYLOAD_OFFSET,
                         WLAN_CHANNEL, true, 1);
            /* check if response is valid */
            test_beacon_response *res =
                    (test_beacon_response *)(respBuf +
                                             RAW_RX_TI_PAYLOAD_OFFSET);

            if (res->header.fc.bits.version == CUSTOM_PROTO_VERSION &&
                res->header.fc.bits.type == WL_MNG &&
                res->header.fc.bits.subtype == as_req) {
                DEBUG("GOT VALID ASSO RESPONSE: MAGIC(%d)", res->magic);
                return 0;
            }
        }

#endif
    }

    return 1;
}