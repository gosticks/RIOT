#include "driver.h"
#include "setup.h"
#include "state.h"
#include "utils.h"
#include "vendor/rom.h"
#include "xtimer.h"
#include <stdio.h>
#include <string.h>

#define ENABLE_DEBUG (1)
#include "debug.h"

#define SL_DEVICE_GENERAL_CONFIGURATION (1)
#define SL_DEVICE_GENERAL_CONFIGURATION_DATE_TIME (11)
#define SL_DEVICE_GENERAL_VERSION (12)
#define SL_DEVICE_STATUS (2)
#define CMD_TIMEOUT 6000

#define DRV_RECV_TIMEOUT (1)
#define DRV_SEND_TIMEOUT (2)

#define DEVICE_INFO_RX_LEN \
    sizeof(_DeviceSetGet_t) + sizeof(SlVersionFull) // for some reason the

uint8_t sendCommand(DriverMessage *msg, DriverResponse *res)
{
    DEBUG("\033[0;34m[WIFI]\033[0m SEND CMD: \033[1;33m%x\033[0m\n",
          msg->Opcode);
    /* write a short sync frame to indicate sending of data */
    send_short_sync();

    /* compute the total message size (the header length is should be omitted)
     * because it is always present
     */
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

    /* send the header */
    send_header(&header);

    /* if a response is expected add request to the waiting queue */
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
    // TODO: handle timeouts
    while (req.Waiting) {
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

/**
 * @brief read network config data from NWP
 *
 * @param configId cc31xx_net_cfg_t enum value
 * @param configOpt
 * @param configLen
 * @param resp
 * @return int16_t
 */
int16_t getNetConfig(cc31xx_net_cfg_t configId, uint8_t *configOpt,
                     uint8_t configLen, uint8_t *resp)
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
    cc31xx_cmd_set_wifi_cfg_t m = { 0 };
    m.req.ConfigId              = configId;
    m.req.ConfigLen             = configLen;
    m.req.ConfigOpt             = configOpt;
    // create msg struct
    DriverMessage msg = { .Opcode     = SL_OPCODE_WLAN_CFG_SET,
                          .RespOpcode = SL_OPCODE_WLAN_CFG_SET_RESPONSE,
                          .cmdDesc    = &m,
                          .cmdDescLen = 8,
                          .payloadLen = (configLen + 3) & (~3),
                          .payload    = resp };

    // prepare response
    DriverResponse res = {
        .resLen = sizeof(_BasicResponse_t),
        .data   = &m, // reuse cmdDesc
    };

    sendCommand(&msg, &res);

    if (m.res.status != 0) {
        printf("[WIFI] setWifiConfig(): non zero status %d", m.res.status);
    }

    return m.res.status;
}

int16_t setWifiMode(uint8_t mode)
{
    cc31xx_cmd_set_wifi_mode_t m = { 0 };

    /* set values */
    m.req.mode = mode;

    /* prepare request and response */
    DriverMessage msg  = { .Opcode     = SL_OPCODE_WLAN_SET_MODE,
                          .RespOpcode = SL_OPCODE_WLAN_SET_MODE_RESPONSE,
                          .cmdDesc    = &m,
                          .cmdDescLen = sizeof(_WlanSetMode_t),
                          .payloadLen = 0 };
    DriverResponse res = {
        .resLen = sizeof(_BasicResponse_t),
        .data   = &m,
    };

    sendCommand(&msg, &res);

    if (m.res.status != 0) {
        printf("[WIFI] setWifiConfig(): non zero status %d", m.res.status);
    }

    return m.res.status;
}

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
    // printWifiConfig(conf);
    cc31xx_cmd_connect_t m = { 0 };

    DriverMessage msg = { .Opcode     = SL_OPCODE_WLAN_WLANCONNECTCOMMAND,
                          .RespOpcode = SL_OPCODE_WLAN_WLANCONNECTRESPONSE,
                          .cmdDesc    = &m,
                          .cmdDescLen = sizeof(_WlanConnectCommon_t),
                          .payloadLen = 0 };

    m.req.Args.Common.SsidLen = (uint8_t)conf->common.SsidLen;

    if (NULL != conf->ssid) {
        /* copy SSID */
        memcpy(SSID_STRING(&m.req), conf->ssid, conf->common.SsidLen);
        msg.cmdDescLen += conf->common.SsidLen;
    }

    /* update security type */
    m.req.Args.Common.SecType = conf->common.SecType;

    if (conf->common.SecType != SEC_TYPE_OPEN) {
        /* verify key length */
        if (conf->common.PasswordLen > MAX_KEY_LEN) {
            return -1;
        }
        /* update key length */
        m.req.Args.Common.PasswordLen = conf->common.PasswordLen;
        /* copy key (could be no key in case of WPS pin) */
        if (NULL != conf->key) {
            memcpy(PASSWORD_STRING(&m.req), conf->key,
                   conf->common.PasswordLen);
        }

        msg.cmdDescLen += conf->common.PasswordLen;
    } else {
        m.req.Args.Common.PasswordLen = 0;
    }

    /* set BSSID to zero */
    memset(m.req.Args.Common.Bssid, 0, sizeof(m.req.Args.Common.Bssid));

    // prepare response
    DriverResponse res = {
        .resLen = 8,      // sizeof(_BasicResponse_t),
        .data   = &m.req, // reuse cmdDesc
    };
    sendCommand(&msg, &res);
    return m.res.status;
}

int16_t profileAdd(WifiProfileConfig *conf)
{
    cc31xx_cmd_add_profile_t m = { 0 };

    m.req.Args.Common.Priority = (uint8_t)conf->common.Priority;
    m.req.Args.Common.SsidLen  = (uint8_t)conf->common.SsidLen;
    if (NULL != conf->ssid) {
        /* copy SSID */
        memcpy(PROFILE_SSID_STRING(&m.req), conf->ssid, conf->common.SsidLen);
    }

    /* update security type */
    m.req.Args.Common.SecType = conf->common.SecType;

    /* verify key length */
    if (conf->common.PasswordLen > MAX_KEY_LEN) {
        return -1;
    }
    /* update key length */
    m.req.Args.Common.PasswordLen = conf->common.PasswordLen;
    /* copy key (could be no key in case of WPS pin) */
    if (NULL != conf->key) {
        memcpy(PROFILE_PASSWORD_STRING(&m.req), conf->key,
               conf->common.PasswordLen);
    }

    memcpy(m.req.Args.Common.Bssid, 0, sizeof(m.req.Args.Common.Bssid));

    DriverMessage msg = {
        .Opcode     = SL_OPCODE_WLAN_PROFILEADDCOMMAND,
        .RespOpcode = SL_OPCODE_WLAN_PROFILEADDRESPONSE,
        .cmdDesc    = &m.req,
        .cmdDescLen = sizeof(_WlanAddGetProfile_t),
        .payloadLen = 0,
    };

    msg.cmdDescLen += conf->common.PasswordLen;
    msg.cmdDescLen += conf->common.SsidLen;
    // prepare response
    DriverResponse res = {
        .resLen = sizeof(_BasicResponse_t),
        .data   = &m.req,
    };
    sendCommand(&msg, &res);
    return m.res.status;
}

int16_t getProfile(int16_t index)
{
    cc31xx_cmd_del_get_profile_t m = { 0 };
    m.req.index                    = (uint8_t)index;
    // create msg struct
    DriverMessage msg = {
        .Opcode     = SL_OPCODE_WLAN_PROFILEGETCOMMAND,
        .RespOpcode = SL_OPCODE_WLAN_PROFILEGETRESPONSE,
        .cmdDesc    = &m.req,
        .cmdDescLen = sizeof(_WlanProfileDelGetCommand_t),
        .payloadLen = 0,
    };

    // prepare response
    DriverResponse res = {
        .resLen = sizeof(_SlProfileParams_t),
        .data   = &m.res, // reuse cmdDesc
    };

    sendCommand(&msg, &res);

#if ENABLE_DEBUG
    // try printing profile name
    if (m.res.Args.Common.SsidLen != 0) {
        printf("[WIFI] read profile SsidLen=%i SSID=",
               m.res.Args.Common.SsidLen);
        printChars((char *)EAP_PROFILE_SSID_STRING(&m.res),
                   m.res.Args.Common.SsidLen);
        printf("\n");
    } else {
        DEBUG("[WIFI] read profile SsidLen=0 \n");
    }
    DEBUG("[WIFI] Profile Info: secType=%d \n", m.res.Args.Common.SecType);
#endif
    return 0;
}

int16_t deleteProfile(int16_t index)
{
    cc31xx_cmd_del_get_profile_t m = { 0 };
    m.req.index                    = (uint8_t)index;
    // create msg struct
    DriverMessage req = {
        .Opcode     = SL_OPCODE_WLAN_PROFILEDELCOMMAND,
        .RespOpcode = SL_OPCODE_WLAN_PROFILEDELRESPONSE,
        .cmdDesc    = &m.req,
        .cmdDescLen = sizeof(_WlanProfileDelGetCommand_t),
    };

    // prepare response
    DriverResponse res = {
        .resLen = sizeof(_BasicResponse_t),
        .data   = &m.res, // reuse cmdDesc
    };

    if (sendCommand(&req, &res) != 0) {
        return -1;
    }

    return 0;
}

int16_t disconnectFromWifi(void)
{
    _BasicResponse_t resp = { 0 };

    // create msg struct
    DriverMessage msg = {
        .Opcode     = SL_OPCODE_WLAN_WLANDISCONNECTCOMMAND,
        .RespOpcode = SL_OPCODE_WLAN_WLANDISCONNECTRESPONSE,
        .cmdDesc    = NULL,
        .cmdDescLen = 0,
        .payloadLen = 0,
    };

    // prepare response
    DriverResponse res = {
        .resLen = sizeof(_BasicResponse_t),
        .data   = &resp, // reuse cmdDesc
    };

    if (sendCommand(&msg, &res) != 0) {
        return -1;
    }
    if (resp.status == 0) {
        DEBUG("[WIFI] Disconnected from network (%d) \n", resp.status);
        state.con.connected = false;
    }
    return resp.status;
}

int16_t setWifiPolicy(uint8_t type, uint8_t policy)
{
    cc31xx_cmd_wifi_policy_set_t m = { 0 };
    m.req.PolicyType               = type;
    m.req.PolicyOption             = policy;
    // create msg struct
    DriverMessage msg = {
        .Opcode     = SL_OPCODE_WLAN_POLICYSETCOMMAND,
        .RespOpcode = SL_OPCODE_WLAN_POLICYSETRESPONSE,
        .cmdDesc    = &m,
        .cmdDescLen = sizeof(_WlanPoliciySetGet_t),
        .payloadLen = 0,
    };

    // prepare response
    DriverResponse res = {
        .resLen = sizeof(_BasicResponse_t),
        .data   = &m,
    };

    sendCommand(&msg, &res);

    return m.res.status;
}

#define SL_CONNECTION_PENDING                                            \
    (-72) /* Transceiver - Device is connected, disconnect first to open \
             transceiver */

int16_t openSocket(int16_t domain, int16_t type, int16_t protocol)
{
    cc31xx_cmd_open_sock_t m = { 0 };
    m.req.Domain             = (uint8_t)domain;
    m.req.Type               = (uint8_t)type;
    m.req.Protocol           = (uint8_t)protocol;
    // create msg struct
    DriverMessage msg = {
        .Opcode     = SL_OPCODE_SOCKET_SOCKET,
        .RespOpcode = SL_OPCODE_SOCKET_SOCKETRESPONSE,
        .cmdDesc    = &m.req,
        .cmdDescLen = sizeof(_SocketCommand_t),
    };

    // prepare response
    DriverResponse res = {
        .resLen = sizeof(_SocketResponse_t),
        .data   = &m.res,
    };

    if (sendCommand(&msg, &res) != 0) {
        DEBUG("[WIFI] failed to open socket \n");
        return -1;
    }
    DEBUG("[WIFI] openSocket SOCK=%d StatusOrLen=%d \n", m.res.sd,
          m.res.statusOrLen);
    if (m.res.statusOrLen < 0) {
        DEBUG("[WIFI] failed to open socket status(%d) \n", m.res.statusOrLen);
#if ENABLE_DEBUG
        switch (m.res.statusOrLen) {
        case SL_CONNECTION_PENDING:
            DEBUG("[HINT] NWP still connected to network disconnect first to use RAW sockets \n");
            break;
        }
#endif
        return m.res.statusOrLen;
    }
    return m.res.sd;
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

int16_t recvRawTraceiverData(int16_t sock, void *buf, int16_t bufLen,
                             int16_t options)
{
    /* create data object */
    cc31xx_cmd_raw_sock_t m = { 0 };

    /* set socket informations */
    m.req.sd             = sock;
    m.req.StatusOrLen    = bufLen;
    m.req.FamilyAndFlags = options & 0x0F;

    DriverMessage msg = {
        .Opcode     = SL_OPCODE_SOCKET_RECV,
        .RespOpcode = SL_OPCODE_SOCKET_RECVASYNCRESPONSE,
        .cmdDesc    = &m,
        .cmdDescLen = sizeof(_sendRecvCommand_t),
    };

    DriverResponse res = {
        .resLen     = sizeof(_SocketResponse_t),
        .data       = &m.res, // reuse cmdDesc
        .payloadLen = bufLen,
        .payload    = buf,
    };

    if (sendCommand(&msg, &res) != 0) {
        DEBUG("ERR: failed to send NWP cmd (0x%x) \n", msg.Opcode);
    }

    return m.res.statusOrLen;
}

int16_t sendTo(int16_t sock, void *buf, uint16_t len, int16_t flags,
               SlSockAddr_t *to, uint16_t toLen)
{
    cc31xx_cmd_send_to_t m = { 0 };

    DriverMessage req = {
        .Opcode     = SL_OPCODE_SOCKET_SENDTO_V6,
        .cmdDesc    = &m.req,
        .cmdDescLen = sizeof(_SocketAddrIPv6Command_t),
        .payloadLen = len,
        .payload    = buf,
    };

    m.req.IpV4.lenOrPadding = len;
    m.req.IpV4.sd           = (uint8_t)sock;
    memcpy(&m.req.IpV4.address, to->sa_data, toLen);
    m.req.IpV4.FamilyAndFlags = (to->sa_family << 4) & 0xF0;
    // TODO: add port support
    // m.req.IpV4.port           = ;
    m.req.IpV4.FamilyAndFlags |= flags & 0x0F;

    if (sendCommand(&req, NULL) != 0) {
        DEBUG("ERR: failed to send data \n");
    }

    return 0;
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
int16_t sendRawTraceiverData(int16_t sock, void *buf, int16_t len,
                             int16_t options)
{
    // TODO: add sync for multiple sockets for now only one socket is
    // supported check mutex
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
        .cmdDesc          = &data,
        .cmdDescLen       = sizeof(_sendRecvCommand_t),
        .payloadHeader    = &optionsCopy,
        .payloadHeaderLen = sizeof(uint32_t),
    };

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

        // printf("Sending package len=%d chunk %d of %d\n", msg.payloadLen,
        // i,
        //        chunksCount);

        // send socket data
        if (sendCommand(&msg, NULL) != 0) {
            DEBUG("ERR: failed to send SOCK data");
            return -1;
        }

        // TODO: check response codes
    }

    return 0;
}

int16_t setWlanFilter(uint8_t filterOptions, uint8_t *inBuf, uint16_t bufLen)
{
    cc31xx_cmd_set_rx_filter_t m = { 0 };
    m.req.RxFilterOperation      = filterOptions;
    m.req.InputBufferLength      = bufLen;
    // create msg struct
    DriverMessage msg = {
        .Opcode     = SL_OPCODE_WLAN_WLANRXFILTERSETCOMMAND,
        .RespOpcode = SL_OPCODE_WLAN_WLANRXFILTERSETRESPONSE,
        .cmdDesc    = &m.req,
        .cmdDescLen = sizeof(_WlanRxFilterSetCommand_t),
        .payload    = inBuf,
        .payloadLen = bufLen,
    };

    // prepare response
    DriverResponse res = {
        .resLen = sizeof(_WlanRxFilterSetCommandReponse_t),
        .data   = &m.res, // reuse cmdDesc
    };

    if (sendCommand(&msg, &res) != 0) {
        printf("[WIFI] failed to set Rx Filter \n");
        return -1;
    }
    return m.res.Status;
}

int16_t setSocketOptions(uint16_t sock, uint16_t level, uint16_t optionName,
                         void *optionVal, uint8_t optionLen)
{
    cc31xx_cmd_sock_opt_t m = { 0 };
    m.req.sd                = (uint8_t)sock;
    m.req.level             = (uint8_t)level;
    m.req.optionLen         = (uint8_t)optionLen;
    m.req.optionName        = (uint8_t)optionName;

    DriverMessage msg = {
        .Opcode     = SL_OPCODE_SOCKET_SETSOCKOPT,
        .RespOpcode = SL_OPCODE_SOCKET_SETSOCKOPTRESPONSE,
        .cmdDesc    = &m.req,
        .cmdDescLen = sizeof(_WlanRxFilterSetCommand_t),
        .payload    = optionVal,
        .payloadLen = optionLen,
    };
    DriverResponse res = {
        .resLen = sizeof(_SocketResponse_t),
        .data   = &m.res,
    };

    if (sendCommand(&msg, &res) != 0) {
        printf("[WIFI] failed to set Rx Filter \n");
        return -1;
    }
    return m.res.statusOrLen;
}