#ifndef CC3200_WIFI_DRIVER
#define CC3200_WIFI_DRIVER

#include "proto.h"
#include <stdint.h>
int read(uint8_t *buf, int len);
int send(uint8_t *in, int len);
void sendShortSync(void);
void sendHeader(uint16_t opcode, uint16_t len);
int readCmdHeader(cc3200_SlResponseHeader *resp);
void sendPowerOnPreamble(void);

#endif