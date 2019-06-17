#ifndef CC3200_WIFI_UTILS
#define CC3200_WIFI_UTILS

#include "proto.h"

#define alignDataLen(len) ((len) + 3) & (~3)

CC3200_RomInfo *getDeviceRomInfo(void);
// uint16_t alignDataLen(uint16_t len);
void sliceFirstInBuffer(uint8_t *buf, int len);
#endif