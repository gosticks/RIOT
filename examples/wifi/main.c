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

#include "cmd.h"
#include "driver.h"
#include "periph/cpuid.h"
#include "periph/gpio.h"
#include "proto.h"
#include "setup.h"
#include "state.h"
#include "utils.h"
#include "vendor/rom.h"

#define RECEIVER (1)
#define WLAN_CHANNEL (1)

/* sockopt */
typedef _u32 SlTime_t;
typedef _u32 SlSuseconds_t;

typedef struct SlTimeval_t {
	SlTime_t      tv_sec; /* Seconds      */
	SlSuseconds_t tv_usec; /* Microseconds */
} SlTimeval_t;

typedef struct wifi_80211_baseheader {
	uint16_t fc; // frame control
	uint16_t duration; // duration
	uint8_t  dest[6]; // destination MAC
	uint8_t  src[6]; // src MAC
	uint8_t  bssid[6]; // bssid MAC
	uint16_t seq_ctl; // Sequence control
} wifi_80211_baseheader;

char RawData_Ping[] = {
	//   0x88, 0x49, 0x30, 0x00, 0xe0, 0x28, 0x6d, 0xc8, 0x87, 0xe6, 0x7c, 0x49,
	//   0xeb, 0x89, 0x08, 0x01, 0xe0, 0x28
	// , 0x6d, 0xc8, 0x87, 0xe4, 0x30, 0x1d, 0x00, 0x00, 0x47, 0x16, 0x00, 0x20,
	// 0x08, 0x00, 0x00, 0x00
	// , 0x45, 0x6d, 0xd5, 0x95, 0xba, 0x84, 0x38, 0x65, 0xab, 0x78, 0xed, 0x9c,
	// 0x6a, 0x1c, 0xe2, 0x7c
	// , 0x1b, 0xbd, 0x97, 0x78, 0x70, 0xdf, 0xf1, 0x49, 0x22, 0xa6, 0x49, 0xdb,
	// 0x90, 0x31, 0x63, 0x58
	// , 0xc6, 0x75, 0x2f, 0xa6, 0x1b, 0xe1, 0x2e, 0x05, 0xe0, 0x7b, 0x13, 0x79,
	// 0xa0, 0xfb, 0x47, 0x98
	// , 0x4b, 0xfd, 0x9a, 0x5c, 0x05, 0xde, 0xac, 0xf1, 0x62, 0x3a, 0x2a, 0xb7,
	// 0xec, 0xca, 0xa3, 0x43
	// , 0x83, 0xd8, 0xf3, 0x2a, 0xde, 0xe4, 0xfe, 0x38, 0x02, 0x96, 0xdb, 0x95,
	// 0x5a, 0x71, 0x30, 0xdd
	// , 0xde, 0x9f, 0xd3, 0x07, 0x1e, 0xec, 0x5c, 0x93, 0x35, 0x1d, 0xa1, 0x65
	/*---- wlan header start -----*/
	0x88, /* version, type and sub type */
	0x08, /* Frame control flag */
	0x2C, 0x00, /* Duration ID */
	0x00, 0x23, 0x75, 0x55, 0x55, 0x55, /* destination */
	0x08, 0x00, 0x28, 0x19, 0x02, 0x85, /* source */
	0x00, 0x22, 0x75, 0x55, 0x55, 0x55, /* bssid */
	0x00, 0x00, /* SC */
	0x00, /* LLC */
	/*---- ip header start -----*/
	0x45, 0x00, 0x00, 0x54, 0x96, 0xA1, 0x00, 0x00, 0x40, 0x01, 0x57,
	0xFA, /* checksum */
	0xc0, 0xa8, 0x01, 0x64, /* src ip */
	0xc0, 0xa8, 0x01, 0x02, /* dest ip  */
	/* payload - ping/icmp */
	0x08, 0x00, 0xA5, 0x51, 0x5E, 0x18, 0x00, 0x00, 0x41, 0x08, 0xBB, 0x8D,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

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
	unsigned char macAddr[6];

	printf("### Device MAC Addr: ");
	printMacAddr(macAddr);
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
			ROM_UtilsDelay(80 * 50);
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

char acBuffer[1500];

int16_t prepareWifi(void)
{
	unsigned char ucVal = 0;
	if (setWifiPolicy(SL_POLICY_SCAN, SL_SCAN_POLICY(0)) != 0) {
		puts("[WIFI] failed to set policy");
	} else {
		puts("[WIFI] policy set");
	}
	setWifiPolicy(SL_POLICY_CONNECTION,
		      SL_CONNECTION_POLICY(0, 0, 0, 0, 0));

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

	uint8_t filterConfig[8] = { 0xFF, 0xFF, 0xFF, 0xFF,
				    0xFF, 0xFF, 0xFF, 0xFF };
	// reset rx filters
	setWlanFilter(1, filterConfig,
		      sizeof(_WlanRxFilterOperationCommandBuff_t));

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
	int16_t sock = openSocket(6, 2, WLAN_CHANNEL);
	if (sock < 0) {
		return sock;
	}

	DEBUG("SOCKET OPEN: %i \n", sock);

	// setSocketOptions(sock0, 1, 20, &timeval, sizeof(timeval));

	while (1) {
		// sendRawTraceiverData(sock0, (uint8_t *)RawData_Ping,
		// sizeof(RawData_Ping), SL_RAW_RF_TX_PARAMS(13, 1, 0, 0));
		receiveLen = recvRawTraceiverData(sock, acBuffer, 1470, 0);
		wifi_80211_baseheader *test =
			(wifi_80211_baseheader *)(acBuffer + 8);
#ifdef ENABLE_DEBUG
		printf("-> RECV PACKET %d bytes from MAC: ", receiveLen);
		printMacAddr(test->src);

#endif
		ROM_UtilsDelay(80 * 100000 / 3);
		gpio_toggle(LED_ORANGE);
	}

	return 0;
}
#endif

#if !RECEIVER
int16_t sendData(void)
{
	int16_t sendLen = 0;
	// get a socket
	int16_t sock = openSocket(6, 2, WLAN_CHANNEL);
	if (sock < 0) {
		return sock;
	}

	DEBUG("SOCKET OPEN: %i \n", sock);

	// setSocketOptions(sock0, 1, 20, &timeval, sizeof(timeval));

	while (1) {
		sendLen = sendRawTraceiverData(
			sock, (uint8_t *)RawData_Ping, sizeof(RawData_Ping),
			SL_RAW_RF_TX_PARAMS(WLAN_CHANNEL, 1, 0, 0));
#ifdef ENABLE_DEBUG
		printf("<- SEND PACKET %d bytes \n", sendLen);
		// printMacAddr(test->src);
#endif
		ROM_UtilsDelay(4000000);
		gpio_toggle(LED_ORANGE);
	}

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
	printf("### DEVICE MAC is: ");
	printMacAddr((unsigned char *)&state.macAddr);
	printDeviceMacAddr();

	// configure wifi module
	prepareWifi();

	// connect(&apConf);
	// keept the programm going

	// wait for a connection
	// while (state.con.connected == 0) {
	//   puts("waiting for connection");
	//   ROM_UtilsDelay(30000 * 80 / 3);
	// }

#if RECEIVER
	receiveData();
#else
	sendData();
#endif

	return 0;
}