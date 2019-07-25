#include "driver.h"
#include "protocol.h"
#include "setup.h"
#include "state.h"
#include "utils.h"
#include "vendor/rom.h"
#include "xtimer.h"
#include <stdio.h>
#include <string.h>

#define SL_DEVICE_GENERAL_CONFIGURATION (1)
#define SL_DEVICE_GENERAL_CONFIGURATION_DATE_TIME (11)
#define SL_DEVICE_GENERAL_VERSION (12)
#define SL_DEVICE_STATUS (2)
#define CMD_TIMEOUT 80

#define MAX_SSID_LEN (32)
#define MAX_KEY_LEN (63)
#define MAX_USER_LEN (32)
#define MAX_ANON_USER_LEN (32)
#define MAX_SMART_CONFIG_KEY (16)

typedef struct {
    _WlanAddGetEapProfile_t Args;
    _i8 Strings[MAX_SSID_LEN + MAX_KEY_LEN + MAX_USER_LEN + MAX_ANON_USER_LEN];
} _SlProfileParams_t;

typedef struct DriverMessage {
    uint16_t Opcode;     // specifies opcode & total command size
    uint16_t RespOpcode; // response opcode
    uint8_t *cmdDesc;    // command description
    uint8_t cmdDescLen;  // length of the command description (not protocol
                         // aligned)
    uint8_t *payload;
    uint16_t payloadLen;
    uint8_t *payloadHeader;
    uint16_t payloadHeaderLen;
    bool receiveFlagsViaRxPayload;
} DriverMessage;

typedef struct DriverResponse {
    uint16_t resLen; // full length of response respDesc + payload
    uint16_t payloadLen;
    uint8_t *data; // full response data
    uint8_t *payload;
} DriverResponse;

const _SlCmdCtrl_t _SlDeviceGetCmdCtrl = { SL_OPCODE_DEVICE_DEVICEGET,
                                           sizeof(_DeviceSetGet_t),
                                           sizeof(_DeviceSetGet_t) };

#define DEVICE_INFO_RX_LEN \
    sizeof(_DeviceSetGet_t) + sizeof(SlVersionFull) + 4 // for some reason the

uint8_t sendCommand(DriverMessage *msg, DriverResponse *res)
{
    // printf("\033[0;34m[WIFI]\033[0m SEND CMD: \033[1;33m%x\033[0m\n",
    //    msg->Opcode);
    sendShortSync();

    // compute the total message size (the header is ignored by the server)
    uint16_t cmdDescLen = alignDataLen(msg->payloadHeaderLen + msg->payloadLen +
                                       msg->cmdDescLen);
    _SlGenericHeader_t header = { .Opcode = msg->Opcode, .Len = cmdDescLen };
    // add request to the request queue
    volatile DriverRequest req = { .Opcode         = msg->RespOpcode,
                                   .Waiting        = true,
                                   .DescBufferSize = res->resLen,
                                   .DescBuffer     = res->data };

    if (res->payload != NULL) {
        req.PayloadBuffer     = res->payload;
        req.PayloadBufferSize = res->payloadLen;
    }

    sendHeader(&header);
    if (res != NULL) {
        addToQueue(&req);
    } else {
        req.Waiting = false;
    }

    if (msg->cmdDescLen > 0) {
        // send command descriptions
        send(msg->cmdDesc, alignDataLen(msg->cmdDescLen));
    }

    // if a payload header is provided send it
    if (msg->payloadHeaderLen > 0) {
        // send command descriptions
        send(msg->payloadHeader, alignDataLen(msg->payloadHeaderLen));
    }

    // send payload if provided
    if (msg->payloadLen > 0) {
        send(msg->payload, alignDataLen(msg->payloadLen));
    }

    // wait for message response (rxHandler will copy the value to the res
    // buffer)
    int i;
    // TODO: may lead to problems in multithreaded envs
    for (i = CMD_TIMEOUT; req.Waiting && (i > 0); i--) {
        ROM_UtilsDelay(80 * 50000 / 3);
    }

    if (i == 0 && req.Waiting) {
        printf("[WIFI] response timeout for cmd opcode=%x \n", msg->Opcode);
        return -1;
    }
    return 0;
}

int16_t setNetConfig(uint8_t configId, uint8_t configOpt, uint8_t configLen,
                     uint8_t *payload)
{
    _NetCfgSetGet_t cmdDesc = { 0 };

    // copy config to the cmd
    cmdDesc.ConfigId  = configId;
    cmdDesc.ConfigOpt = configOpt;
    cmdDesc.ConfigLen = configLen;

    // create msg struct
    DriverMessage msg = { .Opcode     = SL_OPCODE_DEVICE_NETCFG_SET_COMMAND,
                          .RespOpcode = SL_OPCODE_DEVICE_NETCFG_SET_RESPONSE,
                          .cmdDesc    = (uint8_t *)&cmdDesc,
                          .cmdDescLen = sizeof(_NetCfgSetGet_t),
                          .payloadLen = configLen,
                          .payload    = payload };

    DriverResponse res = {
        .resLen = 8,
        .data   = (uint8_t *)&cmdDesc,
    };
    if (sendCommand(&msg, &res) != 0) {
        return -1;
    }

    // return status
    return cmdDesc.Status;
}
int16_t getNetConfig(uint8_t configId, uint8_t *configOpt, uint8_t configLen,
                     uint8_t *resp)
{
    // uint8_t respBuf;
    _NetCfgSetGet_t cmdDesc = { 0 };
    cmdDesc.ConfigId        = configId;
    if (configOpt != NULL) {
        cmdDesc.ConfigOpt = *configOpt;
    }
    cmdDesc.ConfigLen = configLen;

    // create msg struct
    DriverMessage msg = { .Opcode     = SL_OPCODE_DEVICE_NETCFG_GET_COMMAND,
                          .RespOpcode = SL_OPCODE_DEVICE_NETCFG_GET_RESPONSE,
                          .cmdDesc    = (uint8_t *)&cmdDesc,
                          .cmdDescLen = sizeof(_NetCfgSetGet_t),
                          .payloadLen = 0 };

    DriverResponse res = { .resLen     = 8,
                           .data       = (uint8_t *)&cmdDesc,
                           .payload    = resp,
                           .payloadLen = configLen };
    if (sendCommand(&msg, &res) != 0) {
        return -1;
    }

    // also copy over configuration options
    *configOpt = cmdDesc.ConfigOpt;

    if (cmdDesc.Status != 0) {
        printf("[WIFI] getNetConfig(): non zero status %d \n", cmdDesc.Status);
    }

    // return status
    return cmdDesc.Status;
}

int16_t getDeviceInfo(SlVersionFull *ver)
{
    _DeviceSetGet_t cmdDesc;
    cmdDesc.DeviceSetId = SL_DEVICE_GENERAL_CONFIGURATION;
    cmdDesc.Option      = SL_DEVICE_GENERAL_VERSION;

    // create msg struct
    DriverMessage msg = { .Opcode     = SL_OPCODE_DEVICE_DEVICEGET,
                          .RespOpcode = SL_OPCODE_DEVICE_DEVICEGETRESPONSE,
                          .cmdDesc    = (uint8_t *)&cmdDesc,
                          .cmdDescLen = 8,
                          .payloadLen = 0 };

    // prepare response
    // uint16_t resLen = sizeof(SlVersionFull) + msg.cmdDescLen;
    uint8_t buf[DEVICE_INFO_RX_LEN];
    DriverResponse res = {
        .resLen = DEVICE_INFO_RX_LEN,
        .data   = buf,
    };

    if (sendCommand(&msg, &res) != 0) {
        return -1;
    }

    // copy version info back to user
    memcpy(ver, &buf[8], sizeof(SlVersionFull));

    return 0;
}

int16_t setWifiConfig(uint8_t configId, uint8_t configOpt, uint8_t configLen,
                      uint8_t *resp)
{
    _WlanCfgSetGet_t data;
    data.ConfigId  = configId;
    data.ConfigLen = configLen;
    data.ConfigOpt = configOpt;
    // create msg struct
    DriverMessage msg = { .Opcode     = SL_OPCODE_WLAN_CFG_SET,
                          .RespOpcode = SL_OPCODE_WLAN_CFG_SET_RESPONSE,
                          .cmdDesc    = (uint8_t *)&data,
                          .cmdDescLen = 8,
                          .payloadLen = (configLen + 3) & (~3),
                          .payload    = resp };

    // prepare response
    DriverResponse res = {
        .resLen = 8,
        .data   = (uint8_t *)&data, // reuse cmdDesc
    };

    sendCommand(&msg, &res);

    if (((_BasicResponse_t *)res.data)->status != 0) {
        printf("[WIFI] setWifiConfig(): non zero status %d",
               ((_BasicResponse_t *)res.data)->status);
    }

    return ((_BasicResponse_t *)res.data)->status;
}

int16_t setWifiMode(uint8_t mode)
{
    _WlanSetMode_t cmdDesc = { 0 };
    cmdDesc.mode           = mode;

    // create msg struct
    DriverMessage msg = { .Opcode     = SL_OPCODE_WLAN_SET_MODE,
                          .RespOpcode = SL_OPCODE_WLAN_SET_MODE_RESPONSE,
                          .cmdDesc    = (uint8_t *)&cmdDesc,
                          .cmdDescLen = 8,
                          .payloadLen = 0 };

    // prepare response
    // uint16_t resLen = sizeof(SlVersionFull) + msg.cmdDescLen;
    DriverResponse res = {
        .resLen = 8,
        .data   = (uint8_t *)&cmdDesc, // reuse cmdDesc
    };

    sendCommand(&msg, &res);

    printf("[WIFI] setWifiMode(): status=%d \n",
           ((_BasicResponse_t *)&cmdDesc)->status);

    return ((_BasicResponse_t *)&cmdDesc)->status;
}

int16_t profileAddEnterprise(WifiProfileConfig *conf)
{
    (void)conf;
    return 0;
}

typedef union {
    _SlProfileParams_t Cmd;
    _BasicResponse_t Rsp;
} _SlProfileAddMsg_u;

typedef struct {
    _WlanConnectEapCommand_t Args;
    _i8 Strings[MAX_SSID_LEN + MAX_KEY_LEN + MAX_USER_LEN + MAX_ANON_USER_LEN];
} _WlanConnectCmd_t;

typedef union {
    _WlanConnectCmd_t Cmd;
    _BasicResponse_t Rsp;
} _SlWlanConnectMsg_u;

void printWifiConfig(WifiProfileConfig *conf)
{
    printf("[WIFI] connecting to network SSID=\"");
    printChars(conf->ssid, conf->common.SsidLen);
    printf("\" KEY=\"");
    printChars(conf->key, conf->common.PasswordLen);
    printf("\"\n");
}

int16_t connect(WifiProfileConfig *conf)
{
    printWifiConfig(conf);
    _SlWlanConnectMsg_u Msg = { 0 };

    // Msg.Cmd.Args.Common.Priority = (uint8_t)6;
    Msg.Cmd.Args.Common.SsidLen = (uint8_t)conf->common.SsidLen;
    // FIXME: cleanup test
    // check where the ssid is written to

    if (NULL != conf->ssid) {
        /* copy SSID */
        memcpy(SSID_STRING(&Msg), conf->ssid, conf->common.SsidLen);
    }

    /* update security type */
    Msg.Cmd.Args.Common.SecType = conf->common.SecType;

    /* verify key length */
    if (conf->common.PasswordLen > MAX_KEY_LEN) {
        return -1;
    }
    /* update key length */
    Msg.Cmd.Args.Common.PasswordLen = conf->common.PasswordLen;
    /* copy key (could be no key in case of WPS pin) */
    if (NULL != conf->key) {
        memcpy(PASSWORD_STRING(&Msg), conf->key, conf->common.PasswordLen);
    }

    memcpy(Msg.Cmd.Args.Common.Bssid, 0, sizeof(Msg.Cmd.Args.Common.Bssid));

    DriverMessage msg = { .Opcode     = SL_OPCODE_WLAN_WLANCONNECTCOMMAND,
                          .RespOpcode = SL_OPCODE_WLAN_WLANCONNECTRESPONSE,
                          .cmdDesc    = (uint8_t *)&Msg,
                          .cmdDescLen = sizeof(_WlanConnectCommon_t),
                          .payloadLen = 0 };

    msg.cmdDescLen += conf->common.PasswordLen;
    msg.cmdDescLen += conf->common.SsidLen;
    // prepare response
    // uint16_t resLen = sizeof(SlVersionFull) + msg.cmdDescLen;
    DriverResponse res = {
        .resLen = 8,               // sizeof(_BasicResponse_t),
        .data   = (uint8_t *)&Msg, // reuse cmdDesc
    };
    printf("connect() sending msg: size=%i \n", msg.cmdDescLen);
    sendCommand(&msg, &res);
    printf("connect() returned: %i \n", Msg.Rsp.status);
    return Msg.Rsp.status;
}

int16_t profileAdd(WifiProfileConfig *conf)
{
    _SlProfileAddMsg_u Msg = { 0 };

    Msg.Cmd.Args.Common.Priority = (uint8_t)conf->common.Priority;
    Msg.Cmd.Args.Common.SsidLen  = (uint8_t)conf->common.SsidLen;
    if (NULL != conf->ssid) {
        /* copy SSID */
        memcpy(PROFILE_SSID_STRING(&Msg), conf->ssid, conf->common.SsidLen);
    }

    /* update security type */
    Msg.Cmd.Args.Common.SecType = conf->common.SecType;

    /* verify key length */
    if (conf->common.PasswordLen > MAX_KEY_LEN) {
        return -1;
    }
    /* update key length */
    Msg.Cmd.Args.Common.PasswordLen = conf->common.PasswordLen;
    /* copy key (could be no key in case of WPS pin) */
    if (NULL != conf->key) {
        memcpy(PROFILE_PASSWORD_STRING(&Msg), conf->key,
               conf->common.PasswordLen);
    }

    memcpy(Msg.Cmd.Args.Common.Bssid, 0, sizeof(Msg.Cmd.Args.Common.Bssid));

    DriverMessage msg = { .Opcode     = SL_OPCODE_WLAN_PROFILEADDCOMMAND,
                          .RespOpcode = SL_OPCODE_WLAN_PROFILEADDRESPONSE,
                          .cmdDesc    = (uint8_t *)&Msg,
                          .cmdDescLen = sizeof(_WlanAddGetProfile_t),
                          .payloadLen = 0 };

    msg.cmdDescLen += conf->common.PasswordLen;
    msg.cmdDescLen += conf->common.SsidLen;
    // prepare response
    // uint16_t resLen = sizeof(SlVersionFull) + msg.cmdDescLen;
    DriverResponse res = {
        .resLen = sizeof(_BasicResponse_t),
        .data   = (uint8_t *)&Msg, // reuse cmdDesc
    };
    printf("profileAdd() sending msg: size=%i \n", msg.cmdDescLen);
    sendCommand(&msg, &res);
    printf("profileAdd() returned: %i \n", Msg.Rsp.status);
    return Msg.Rsp.status;
}

typedef union {
    _WlanProfileDelGetCommand_t Cmd;
    _SlProfileParams_t Rsp;
} _SlProfileGetMsg_u;

int16_t getProfile(int16_t index)
{
    _SlProfileGetMsg_u data = { 0 };
    data.Cmd.index          = (uint8_t)index;
    // create msg struct
    DriverMessage msg = { .Opcode     = SL_OPCODE_WLAN_PROFILEGETCOMMAND,
                          .RespOpcode = SL_OPCODE_WLAN_PROFILEGETRESPONSE,
                          .cmdDesc    = (uint8_t *)&data,
                          .cmdDescLen = 8,
                          .payloadLen = 0 };

    // prepare response
    DriverResponse res = {
        .resLen = sizeof(_SlProfileParams_t),
        .data   = (uint8_t *)&data, // reuse cmdDesc
    };

    sendCommand(&msg, &res);
    // try printing profile name
    if (data.Rsp.Args.Common.SsidLen != 0) {
        printf("[WIFI] read profile SsidLen=%i SSID=\"",
               data.Rsp.Args.Common.SsidLen);
        printChars((char *)EAP_PROFILE_SSID_STRING(&data),
                   data.Rsp.Args.Common.SsidLen);
        printf("\"\n");
    } else {
        printf("[WIFI] read profile SsidLen=0");
    }
    printf("[WIFI] Profile Info: secType=%d", data.Rsp.Args.Common.SecType);

    return 0;
}

int16_t deleteProfile(int16_t index)
{
    _WlanProfileDelGetCommand_t data = { 0 };
    data.index                       = index;
    // create msg struct
    DriverMessage msg = { .Opcode     = SL_OPCODE_WLAN_PROFILEDELCOMMAND,
                          .RespOpcode = SL_OPCODE_WLAN_PROFILEDELRESPONSE,
                          .cmdDesc    = (uint8_t *)&data,
                          .cmdDescLen = 8,
                          .payloadLen = 0 };

    // prepare response
    // uint16_t resLen = sizeof(SlVersionFull) + msg.cmdDescLen;
    DriverResponse res = {
        .resLen = 8,
        .data   = (uint8_t *)&data, // reuse cmdDesc
    };

    sendCommand(&msg, &res);

    return ((_BasicResponse_t *)&data)->status;
}

int16_t disconnectFromWifi(void)
{
    _BasicResponse_t resp = { 0 };

    // create msg struct
    DriverMessage msg = { .Opcode     = SL_OPCODE_WLAN_WLANDISCONNECTCOMMAND,
                          .RespOpcode = SL_OPCODE_WLAN_WLANDISCONNECTRESPONSE,
                          .cmdDesc    = NULL,
                          .cmdDescLen = 0,
                          .payloadLen = 0 };

    // prepare response
    // uint16_t resLen = sizeof(SlVersionFull) + msg.cmdDescLen;
    DriverResponse res = {
        .resLen = 8,
        .data   = (uint8_t *)&resp, // reuse cmdDesc
    };

    sendCommand(&msg, &res);
    if (resp.status == 0) {
        state.con.connected = false;
    }
    return resp.status == 0;
}

int16_t setWifiPolicy(uint8_t type, uint8_t policy)
{
    _WlanPoliciySetGet_t data = { 0 };
    data.PolicyType           = type;
    data.PolicyOption         = policy;
    // create msg struct
    DriverMessage msg = { .Opcode     = SL_OPCODE_WLAN_POLICYSETCOMMAND,
                          .RespOpcode = SL_OPCODE_WLAN_POLICYSETRESPONSE,
                          .cmdDesc    = (uint8_t *)&data,
                          .cmdDescLen = 8,
                          .payloadLen = 0 };

    // prepare response
    // uint16_t resLen = sizeof(SlVersionFull) + msg.cmdDescLen;
    DriverResponse res = {
        .resLen = 8,
        .data   = (uint8_t *)&data, // reuse cmdDesc
    };

    sendCommand(&msg, &res);

    return ((_BasicResponse_t *)&data)->status;
}

int16_t openSocket(int16_t domain, int16_t type, int16_t protocol)
{
    _SocketCommand_t data = { 0 };
    data.Domain           = (uint8_t)domain;
    data.Type             = (uint8_t)type;
    data.Protocol         = (uint8_t)protocol;
    // create msg struct
    DriverMessage msg = { .Opcode     = SL_OPCODE_SOCKET_SOCKET,
                          .RespOpcode = SL_OPCODE_SOCKET_SOCKETRESPONSE,
                          .cmdDesc    = (uint8_t *)&data,
                          .cmdDescLen = sizeof(_SocketCommand_t) };

    // prepare response
    // uint16_t resLen = sizeof(SlVersionFull) + msg.cmdDescLen;
    DriverResponse res = {
        .resLen = 8,
        .data   = (uint8_t *)&data, // reuse cmdDesc
    };

    if (sendCommand(&msg, &res) != 0) {
        printf("[WIFI] failed to open socket \n");
        return -1;
    }
    printf("[WIFI] openSocket SOCK=%d StatusOrLen=%d \n",
           ((_SocketResponse_t *)&data)->sd,
           ((_SocketResponse_t *)&data)->statusOrLen);
    return (_i16)(((_SocketResponse_t *)&data)->sd);
}

/**
 * @brief compute maximal payload len for a provided socket type
 *
 * @param type
 * @return int16_t
 */
uint16_t maxPayloadLen(uint8_t socType)
{
    switch (socType) {
    case 0:
        return 1400;
    default:
        return 1476;
    }
}

typedef union {
    _sendRecvCommand_t Cmd;
    _SocketResponse_t Rsp;
} _SlRecvMsg_u;

int16_t recvRawTraceiverData(int16_t sock, void *buf, int16_t bufLen,
                             int16_t options)
{
    _SlRecvMsg_u Msg;
    Msg.Cmd.sd             = sock;
    Msg.Cmd.StatusOrLen    = bufLen;
    Msg.Cmd.FamilyAndFlags = options & 0x0F;
    DriverMessage msg      = { .Opcode     = SL_OPCODE_SOCKET_RECV,
                          .RespOpcode = SL_OPCODE_SOCKET_RECVASYNCRESPONSE,
                          .cmdDesc    = (uint8_t *)&Msg.Cmd,
                          .cmdDescLen = sizeof(_sendRecvCommand_t) };

    DriverResponse res = { .resLen     = sizeof(_SocketResponse_t),
                           .data       = (uint8_t *)&Msg.Rsp, // reuse cmdDesc
                           .payloadLen = bufLen,
                           .payload    = buf };

    // send socket data
    sendCommand(&msg, &res);

    return Msg.Rsp.statusOrLen;
}

/**
 * @brief send provided buffer as a raw socket type over the MCU
 * buffer will be trancated if length exceeds max len
 * SL_SOCKET_PAYLOAD_TYPE_RAW_PACKET(1514)
 *
 * @param sock
 * @param buf
 * @param len
 * @return int16_t
 */
int16_t sendRawTraceiverData(int16_t sock, uint8_t *buf, int16_t len,
                             int16_t options)
{
    if ((sock & 0xF0) == 0x80) {
        puts("sending via true raw socket");
    }

    // TODO: add sync for multiple sockets for now only one socket is supported
    // check mutex
    _sendRecvCommand_t data = { 0 };
    // FIXME: for now only support raw sockets
    uint16_t packetLen   = maxPayloadLen(0);
    uint32_t optionsCopy = options;

    // fast ceil without math library
    uint8_t chunksCount = (len + packetLen - 1) / packetLen;
    // printf("Split into %d chunks \n", chunksCount);
    // create request response objects
    DriverMessage msg = {
        .Opcode = SL_OPCODE_SOCKET_SEND,
        // .RespOpcode = SL_OPCODE_DEVICE_DEVICEASYNCDUMMY,
        .cmdDesc          = (uint8_t *)&data,
        .cmdDescLen       = sizeof(_sendRecvCommand_t),
        .payloadHeader    = (uint8_t *)(&optionsCopy),
        .payloadHeaderLen = 4,
    };

    // prepare response
    // uint16_t resLen = sizeof(SlVersionFull) + msg.cmdDescLen;
    // DriverResponse res = {
    //     .resLen = 0,
    //     .data = NULL, // reuse cmdDesc
    // };
    uint16_t bufOffset = 0;

    for (int i = 0; i < chunksCount; i++) {
        data.sd = (uint8_t)sock;
        data.FamilyAndFlags |= options & 0x0F;

        msg.payload = (uint8_t *)(buf + bufOffset);

        // compute package lan either max package len or remainder
        msg.payloadLen =
                len - bufOffset > packetLen ? packetLen : len - bufOffset;
        data.StatusOrLen = msg.payloadLen;

        // increment buffer offset
        bufOffset += packetLen;

        // printf("Sending package len=%d chunk %d of %d\n", msg.payloadLen, i,
        //        chunksCount);

        // send socket data
        sendCommand(&msg, NULL);

        // TODO: check response codes
    }

    return 0;
}

/* Set command */
typedef union _SlRxFilterSetMsg_u {
    _WlanRxFilterSetCommand_t Cmd;
    _WlanRxFilterSetCommandReponse_t Rsp;
} _SlRxFilterSetMsg_u;

int16_t setWlanFilter(uint8_t filterOptions, uint8_t *inBuf, uint16_t bufLen)
{
    _SlRxFilterSetMsg_u data   = { 0 };
    data.Cmd.RxFilterOperation = filterOptions;
    data.Cmd.InputBufferLength = bufLen;
    // create msg struct
    DriverMessage msg = { .Opcode     = SL_OPCODE_WLAN_WLANRXFILTERSETCOMMAND,
                          .RespOpcode = SL_OPCODE_WLAN_WLANRXFILTERSETRESPONSE,
                          .cmdDesc    = (uint8_t *)&data,
                          .cmdDescLen = sizeof(_WlanRxFilterSetCommand_t),
                          .payload    = inBuf,
                          .payloadLen = bufLen };

    // prepare response
    // uint16_t resLen = sizeof(SlVersionFull) + msg.cmdDescLen;
    DriverResponse res = {
        .resLen = sizeof(_WlanRxFilterSetCommandReponse_t),
        .data   = (uint8_t *)&data, // reuse cmdDesc
    };

    if (sendCommand(&msg, &res) != 0) {
        printf("[WIFI] failed to set Rx Filter \n");
        return -1;
    }
    printf("!! setWlanFilter: %d \n", data.Rsp.Status);
    return data.Rsp.Status;
}

typedef union {
    _setSockOptCommand_t Cmd;
    _SocketResponse_t Rsp;
} _SlSetSockOptMsg_u;

int16_t setSocketOptions(uint16_t sock, uint16_t level, uint16_t optionName,
                         void *optionVal, uint8_t optionLen)
{
    _SlSetSockOptMsg_u data = { 0 };
    data.Cmd.sd             = (uint8_t)sock;
    data.Cmd.level          = (uint8_t)level;
    data.Cmd.optionLen      = (uint8_t)optionLen;
    data.Cmd.optionName     = (uint8_t)optionName;
    // create msg struct
    DriverMessage msg = { .Opcode     = SL_OPCODE_SOCKET_SETSOCKOPT,
                          .RespOpcode = SL_OPCODE_SOCKET_SETSOCKOPTRESPONSE,
                          .cmdDesc    = (uint8_t *)&data,
                          .cmdDescLen = sizeof(_WlanRxFilterSetCommand_t),
                          .payload    = optionVal,
                          .payloadLen = optionLen };

    // prepare response
    // uint16_t resLen = sizeof(SlVersionFull) + msg.cmdDescLen;
    DriverResponse res = {
        .resLen = sizeof(_SocketResponse_t),
        .data   = (uint8_t *)&data, // reuse cmdDesc
    };

    if (sendCommand(&msg, &res) != 0) {
        printf("[WIFI] failed to set Rx Filter \n");
        return -1;
    }
    printf("!! setSocketOptions: %d \n", data.Rsp.statusOrLen);
    return data.Rsp.statusOrLen;
}