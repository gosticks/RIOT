

#include "driver.h"
#include "protocol.h"
#include "setup.h"
#include "state.h"
#include "utils.h"
#include "xtimer.h"
#include <stdio.h>
#include <string.h>

#define SL_DEVICE_GENERAL_CONFIGURATION (1)
#define SL_DEVICE_GENERAL_CONFIGURATION_DATE_TIME (11)
#define SL_DEVICE_GENERAL_VERSION (12)
#define SL_DEVICE_STATUS (2)
#define CMD_TIMEOUT 40

typedef struct DriverMessage {
  uint16_t Opcode;     // specifies opcode & total command size
  uint16_t RespOpcode; // response opcode
  uint8_t *cmdDesc;    // command description
  uint8_t
      cmdDescLen; // length of the command description (not protocol aligned)
  uint8_t *payload;
  uint8_t payloadLen;

} DriverMessage;

typedef struct DriverResponse {
  uint16_t resLen; // full length of response respDesc + payload
  uint8_t *data;   // full response data
} DriverResponse;

const _SlCmdCtrl_t _SlDeviceGetCmdCtrl = {SL_OPCODE_DEVICE_DEVICEGET,
                                          sizeof(_DeviceSetGet_t),
                                          sizeof(_DeviceSetGet_t)};

static bool isFirstCmd = true;

#define DEVICE_INFO_RX_LEN                                                     \
  sizeof(_DeviceSetGet_t) + sizeof(SlVersionFull) + 4 // for some reason the

void sendCommand(DriverMessage *msg, DriverResponse *res) {
  sendShortSync();

  // compute the total message size (the header is ignored by the server)
  uint16_t cmdDescLen = alignDataLen(msg->payloadLen + msg->cmdDescLen);
  _SlGenericHeader_t header = {.Opcode = SL_OPCODE_DEVICE_DEVICEGET,
                               .Len = cmdDescLen};
  // add request to the request queue
  DriverRequest drReq = {.ID = 2,
                         .Opcode = msg->RespOpcode,
                         .Waiting = true,
                         .BufferSize = res->resLen,
                         .Buffer = res->data};

  sendHeader(&header);
  addToQueue(&drReq);

  // send command descriptions
  send((uint8_t *)msg->cmdDesc, alignDataLen(msg->cmdDescLen));

  // give SPI some time to "stabilize" as done in TIs code
  if (isFirstCmd) {
    volatile uint32_t CountVal = 0;
    isFirstCmd = true;
    CountVal = 80 * 50;
    while (CountVal--)
      ;
  }

  // wait for message response (rxHandler will copy the value to the res buffer)
  // TODO: may lead to problems in multithreaded envs
  for (int i = CMD_TIMEOUT; (drReq.Waiting == 1) && (i > 0); i--) {
    xtimer_usleep(250);
  }
}

void getDeviceInfo(SlVersionFull *ver) {
  _DeviceSetGet_t cmdDesc;
  cmdDesc.DeviceSetId = SL_DEVICE_GENERAL_CONFIGURATION;
  cmdDesc.Option = SL_DEVICE_GENERAL_VERSION;

  // create msg struct
  DriverMessage msg = {.Opcode = SL_OPCODE_DEVICE_DEVICEGET,
                       .RespOpcode = SL_OPCODE_DEVICE_DEVICEGETRESPONSE,
                       .cmdDesc = (uint8_t *)&cmdDesc,
                       .cmdDescLen = sizeof(_DeviceSetGet_t),
                       .payloadLen = 0};

  // prepare response
  // uint16_t resLen = sizeof(SlVersionFull) + msg.cmdDescLen;
  uint8_t buf[DEVICE_INFO_RX_LEN];
  DriverResponse res = {
      .resLen = DEVICE_INFO_RX_LEN,
      .data = buf,
  };

  sendCommand(&msg, &res);

  // copy version info back to user
  memcpy(ver, &buf[sizeof(_DeviceSetGet_t)], sizeof(SlVersionFull));
}