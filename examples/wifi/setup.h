#ifndef CC3200_WIFI_SETUP
#define CC3200_WIFI_SETUP

#include "proto.h"

void defaultCommandHandler(cc3200_SlResponseHeader *header);
int setupWifiModule(void);
void powerOffWifi(void);
void powerOnWifi(void);

#endif