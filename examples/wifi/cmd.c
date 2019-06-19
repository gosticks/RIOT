

#include "driver.h"
#include "protocol.h"
#include "setup.h"
#include "utils.h"
#include "xtimer.h"
#include <stdio.h>
#include <string.h>

#define SL_DEVICE_GENERAL_CONFIGURATION (1)
#define SL_DEVICE_GENERAL_CONFIGURATION_DATE_TIME (11)
#define SL_DEVICE_GENERAL_VERSION (12)
#define SL_DEVICE_STATUS (2)

typedef struct DriverMessage {
  _SlGenericHeader_t *header; // specifies opcode & total command size
  uint8_t *cmdDesc;           // command description
  uint8_t
      cmdDescLen; // length of the command description (not protocol aligned)
  uint8_t *payload;
  uint8_t payloadLen;

} DriverMessage;

typedef struct DriverResponse {
  uint8_t *cmdRespDesc;
  uint8_t *payload;
} DriverResponse;

const _SlCmdCtrl_t _SlDeviceGetCmdCtrl = {SL_OPCODE_DEVICE_DEVICEGET,
                                          sizeof(_DeviceSetGet_t),
                                          sizeof(_DeviceSetGet_t)};

static bool isFirstCmd = true;

#define DEVICE_INFO_RX_LEN sizeof(_DeviceSetGet_t) + sizeof(SlVersionFull)

void sendCommand(DriverMessage *msg, DriverResponse *res) {}

void getDeviceInfo(SlVersionFull *ver) {
  uint8_t buf[DEVICE_INFO_RX_LEN];

  uint8_t DeviceGetId = SL_DEVICE_GENERAL_CONFIGURATION;
  sendShortSync();
  uint16_t cmdDescLen = alignDataLen(sizeof(_DeviceSetGet_t));
  sendHeader(SL_OPCODE_DEVICE_DEVICEGET, cmdDescLen);

  // send command description
  _DeviceSetGet_t cmdDesc;
  cmdDesc.DeviceSetId = DeviceGetId;
  cmdDesc.Option = SL_DEVICE_GENERAL_VERSION;

  // mask rx interrupt
  maskWifiInterrupt();

  send((uint8_t *)&cmdDesc, cmdDescLen);

  // give SPI some time to "stabilize" as done in TIs code
  if (isFirstCmd) {
    volatile uint32_t CountVal = 0;
    isFirstCmd = true;
    CountVal = 80 * 50;
    while (CountVal--)
      ;
  }

  cc3200_SlResponseHeader cmd;

  while (1) {
    readCmdHeader(&cmd);
    printf("GOT RESPONSE: opcode=%d", cmd.GenHeader.Opcode);
    if (cmd.GenHeader.Opcode != SL_OPCODE_DEVICE_DEVICEGETRESPONSE) {
      // call default handler
      defaultCommandHandler(&cmd);
    } else {
      break;
    }
  }
  uint16_t rxPayloadLen = sizeof(SlVersionFull) + _SlDeviceGetCmdCtrl.RxDescLen;
  // now that we have the response we waited for
  read(buf, alignDataLen(rxPayloadLen));
  unmaskWifiInterrupt();

  memcpy(ver, &buf[sizeof(_DeviceSetGet_t)], sizeof(SlVersionFull));
}