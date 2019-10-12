#include "vendor/rom.h"
#include "vendor/sl_nwp.h"

#include "xtimer.h"

#include <stdio.h>
#include <string.h>

#include "cc3100.h"
#include "cc3100_internal.h"
#include "cc3100_protocol.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

#define SL_DEVICE_GENERAL_CONFIGURATION (1)
#define SL_DEVICE_GENERAL_CONFIGURATION_DATE_TIME (11)
#define SL_DEVICE_GENERAL_VERSION (12)
#define SL_DEVICE_STATUS (2)

#define SL_CONNECTION_PENDING                                            \
    (-72) /* Transceiver - Device is connected, disconnect first to open \
             transceiver */

/* Default command timeout */
#define CMD_TIMEOUT 6000

#define DRV_RECV_TIMEOUT (1)
#define DRV_SEND_TIMEOUT (2)

#define DEVICE_INFO_RX_LEN \
    sizeof(_DeviceSetGet_t) + sizeof(SlVersionFull) // for some reason the

#if ENABLE_DEBUG
/**
 * @brief Util to print single chars
 *
 * @param str
 * @param len
 */
void printChars(char *str, uint16_t len)
{
    for (int i = 0; i < len; i++) {
        printf("%c", str[i]);
    }
}

void printWifiConfig(WifiProfileConfig *conf)
{
    printf("[WIFI] connecting to network SSID=\"");
    printChars(conf->ssid, conf->common.SsidLen);
    printf("\" KEY=\"");
    printChars(conf->key, conf->common.PasswordLen);
    printf("\"\n");
}

#endif

/**
 * @brief compute maximal payload len for a provided socket type
 *
 * @param type
 * @return int16_t
 */
static inline uint16_t _max_payload_len(uint8_t socType)
{
    switch (socType) {
    case 0:
        return 1400;
    default:
        return 1476;
    }
}

/**
 * @brief configure NWP network options
 *
 * @param configId
 * @param configOpt
 * @param configLen
 * @param payload
 * @return int16_t
 */
int16_t _nwp_set_net_cfg(cc3100_t *dev, uint8_t configId, uint8_t configOpt,
                         uint8_t configLen, uint8_t *payload)
{
    _NetCfgSetGet_t cmdDesc = { 0 };

    // copy config to the cmd
    cmdDesc.ConfigId  = configId;
    cmdDesc.ConfigOpt = configOpt;
    cmdDesc.ConfigLen = configLen;

    // create msg struct
    cc31xx_nwp_msg_t msg = { .opcode = SL_OPCODE_DEVICE_NETCFG_SET_COMMAND,
                             // .RespOpcode =
                             // SL_OPCODE_DEVICE_NETCFG_SET_RESPONSE,
                             .desc_buf    = (uint8_t *)&cmdDesc,
                             .desc_len    = sizeof(_NetCfgSetGet_t),
                             .payload_len = configLen,
                             .payload_buf = payload };

    cc31xx_nwp_rsp_t res = {
        .res_len = 8,
        .data    = (uint8_t *)&cmdDesc,
    };
    if (cc31xx_send_nwp_cmd(dev, &msg, &res) != 0) {
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
int16_t _nwp_get_net_cfg(cc3100_t *dev, cc31xx_net_cfg_t configId,
                         uint8_t *configOpt, uint8_t configLen, uint8_t *resp)
{
    // uint8_t respBuf;
    _NetCfgSetGet_t cmdDesc = { 0 };
    cmdDesc.ConfigId        = configId;
    if (configOpt != NULL) {
        cmdDesc.ConfigOpt = *configOpt;
    }
    cmdDesc.ConfigLen = configLen;

    // create msg struct
    cc31xx_nwp_msg_t msg = { .opcode = SL_OPCODE_DEVICE_NETCFG_GET_COMMAND,
                             // .RespOpcode =
                             // SL_OPCODE_DEVICE_NETCFG_GET_RESPONSE,
                             .desc_buf    = (uint8_t *)&cmdDesc,
                             .desc_len    = sizeof(_NetCfgSetGet_t),
                             .payload_len = 0 };

    cc31xx_nwp_rsp_t res = { .res_len     = 8,
                             .data        = (uint8_t *)&cmdDesc,
                             .payload     = resp,
                             .payload_len = configLen };
    if (cc31xx_send_nwp_cmd(dev, &msg, &res) != 0) {
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

/**
 * @brief Get the Device Info object
 *
 * @param ver
 * @return int16_t
 */
int16_t _nwp_get_dvc_info(cc3100_t *dev, SlVersionFull *ver)
{
    _DeviceSetGet_t cmdDesc;
    cmdDesc.DeviceSetId = SL_DEVICE_GENERAL_CONFIGURATION;
    cmdDesc.Option      = SL_DEVICE_GENERAL_VERSION;

    // create msg struct
    cc31xx_nwp_msg_t msg = { .opcode = SL_OPCODE_DEVICE_DEVICEGET,
                             // .RespOpcode =
                             // SL_OPCODE_DEVICE_DEVICEGETRESPONSE,
                             .desc_buf    = (uint8_t *)&cmdDesc,
                             .desc_len    = 8,
                             .payload_len = 0 };

    // prepare response
    uint8_t buf[DEVICE_INFO_RX_LEN];
    cc31xx_nwp_rsp_t res = {
        .res_len = DEVICE_INFO_RX_LEN,
        .data    = buf,
    };

    if (cc31xx_send_nwp_cmd(dev, &msg, &res) != 0) {
        return -1;
    }

    // copy version info back to user
    memcpy(ver, &buf[8], sizeof(SlVersionFull));

    return 0;
}

/**
 * @brief set wifi configurations
 *
 * @param configId
 * @param configOpt
 * @param configLen
 * @param resp
 * @return int16_t
 */
int16_t _nwp_set_wifi_cfg(cc3100_t *dev, uint8_t configId, uint8_t configOpt,
                          uint8_t configLen, uint8_t *resp)
{
    cc31xx_cmd_set_wifi_cfg_t m = { 0 };
    m.req.ConfigId              = configId;
    m.req.ConfigLen             = configLen;
    m.req.ConfigOpt             = configOpt;
    // create msg struct
    cc31xx_nwp_msg_t msg = { .opcode = SL_OPCODE_WLAN_CFG_SET,
                             // .RespOpcode = SL_OPCODE_WLAN_CFG_SET_RESPONSE,
                             .desc_buf    = &m,
                             .desc_len    = 8,
                             .payload_len = (configLen + 3) & (~3),
                             .payload_buf = resp };

    // prepare response
    cc31xx_nwp_rsp_t res = {
        .res_len = sizeof(_BasicResponse_t),
        .data    = &m, // reuse cmdDesc
    };

    cc31xx_send_nwp_cmd(dev, &msg, &res);

    if (m.res.status != 0) {
        printf("[WIFI] setWifiConfig(): non zero status %d", m.res.status);
    }

    return m.res.status;
}

int16_t _nwp_set_wifi_mode(cc3100_t *dev, uint8_t mode)
{
    cc31xx_cmd_set_wifi_mode_t m = { 0 };

    /* set values */
    m.req.mode = mode;

    /* prepare request and response */
    cc31xx_nwp_msg_t msg = { .opcode = SL_OPCODE_WLAN_SET_MODE,
                             // .RespOpcode = SL_OPCODE_WLAN_SET_MODE_RESPONSE,
                             .desc_buf    = &m,
                             .desc_len    = sizeof(_WlanSetMode_t),
                             .payload_len = 0 };
    cc31xx_nwp_rsp_t res = {
        .res_len = sizeof(_BasicResponse_t),
        .data    = &m,
    };

    cc31xx_send_nwp_cmd(dev, &msg, &res);

    if (m.res.status != 0) {
        printf("[WIFI] setWifiConfig(): non zero status %d", m.res.status);
    }

    return m.res.status;
}

/**
 * @brief connect to a Wi-Fi network set by the config
 *
 * @param dev
 * @param conf
 * @return int16_t
 */
int16_t _nwp_connect(cc3100_t *dev, WifiProfileConfig *conf)
{
#if ENABLE_DEBUG
    printWifiConfig(conf);
#endif
    cc31xx_cmd_connect_t m = { 0 };

    cc31xx_nwp_msg_t msg = { .opcode = SL_OPCODE_WLAN_WLANCONNECTCOMMAND,
                             // .RespOpcode =
                             // SL_OPCODE_WLAN_WLANCONNECTRESPONSE,
                             .desc_buf    = &m,
                             .desc_len    = sizeof(_WlanConnectCommon_t),
                             .payload_len = 0 };

    m.req.Args.Common.SsidLen = (uint8_t)conf->common.SsidLen;

    if (NULL != conf->ssid) {
        /* copy SSID */
        memcpy(SSID_STRING(&m.req), conf->ssid, conf->common.SsidLen);
        msg.desc_len += conf->common.SsidLen;
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

        msg.desc_len += conf->common.PasswordLen;
    } else {
        m.req.Args.Common.PasswordLen = 0;
    }

    /* set BSSID to zero */
    memset(m.req.Args.Common.Bssid, 0, sizeof(m.req.Args.Common.Bssid));

    // prepare response
    cc31xx_nwp_rsp_t res = {
        .res_len = 8,      // sizeof(_BasicResponse_t),
        .data    = &m.req, // reuse cmdDesc
    };
    cc31xx_send_nwp_cmd(dev, &msg, &res);
    return m.res.status;
}

/**
 * @brief
 *
 * @param conf
 * @return int16_t
 */
int16_t _nwp_profile_add(cc3100_t *dev, WifiProfileConfig *conf)
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

    cc31xx_nwp_msg_t msg = {
        .opcode = SL_OPCODE_WLAN_PROFILEADDCOMMAND,
        // .RespOpcode = SL_OPCODE_WLAN_PROFILEADDRESPONSE,
        .desc_buf    = &m.req,
        .desc_len    = sizeof(_WlanAddGetProfile_t),
        .payload_len = 0,
    };

    msg.desc_len += conf->common.PasswordLen;
    msg.desc_len += conf->common.SsidLen;
    // prepare response
    cc31xx_nwp_rsp_t res = {
        .res_len = sizeof(_BasicResponse_t),
        .data    = &m.req,
    };
    cc31xx_send_nwp_cmd(dev, &msg, &res);
    return m.res.status;
}

int16_t _nwp_get_profile(cc3100_t *dev, int16_t index)
{
    cc31xx_cmd_del_get_profile_t m = { 0 };
    m.req.index                    = (uint8_t)index;
    // create msg struct
    cc31xx_nwp_msg_t msg = {
        .opcode = SL_OPCODE_WLAN_PROFILEGETCOMMAND,
        // .RespOpcode = SL_OPCODE_WLAN_PROFILEGETRESPONSE,
        .desc_buf    = &m.req,
        .desc_len    = sizeof(_WlanProfileDelGetCommand_t),
        .payload_len = 0,
    };

    // prepare response
    cc31xx_nwp_rsp_t res = {
        .res_len = sizeof(_SlProfileParams_t),
        .data    = &m.res, // reuse cmdDesc
    };

    cc31xx_send_nwp_cmd(dev, &msg, &res);

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

/**
 * @brief delete a connection profile stored on the NWP
 *
 * @param index index of the profile or 0xFF for all
 * @return int16_t NWP error code (0 == OK)
 */
int16_t _nwp_del_profile(cc3100_t *dev, int16_t index)
{
    cc31xx_cmd_del_get_profile_t m = { 0 };
    m.req.index                    = (uint8_t)index;
    // create msg struct
    cc31xx_nwp_msg_t req = {
        .opcode = SL_OPCODE_WLAN_PROFILEDELCOMMAND,
        // .RespOpcode = SL_OPCODE_WLAN_PROFILEDELRESPONSE,
        .desc_buf = &m.req,
        .desc_len = sizeof(_WlanProfileDelGetCommand_t),
    };

    // prepare response
    cc31xx_nwp_rsp_t res = {
        .res_len = sizeof(_BasicResponse_t),
        .data    = &m.res, // reuse cmdDesc
    };

    if (cc31xx_send_nwp_cmd(dev, &req, &res) != 0) {
        return -1;
    }

    return 0;
}

int16_t _nwp_disconnect(cc3100_t *dev)
{
    _BasicResponse_t resp = { 0 };

    // create msg struct
    cc31xx_nwp_msg_t msg = {
        .opcode = SL_OPCODE_WLAN_WLANDISCONNECTCOMMAND,
        // .RespOpcode = SL_OPCODE_WLAN_WLANDISCONNECTRESPONSE,
        .desc_buf    = NULL,
        .desc_len    = 0,
        .payload_len = 0,
    };

    // prepare response
    cc31xx_nwp_rsp_t res = {
        .res_len = sizeof(_BasicResponse_t),
        .data    = &resp, // reuse cmdDesc
    };

    if (cc31xx_send_nwp_cmd(dev, &msg, &res) != 0) {
        return -1;
    }
    if (resp.status == 0) {
        DEBUG("[WIFI] Disconnected from network (%d) \n", resp.status);
        // _nwp.con.connected = false;
    }
    return resp.status;
}

int16_t _nwp_set_wifi_policy(cc3100_t *dev, uint8_t type, uint8_t policy)
{
    cc31xx_cmd_wifi_policy_set_t m = { 0 };
    m.req.PolicyType               = type;
    m.req.PolicyOption             = policy;
    // create msg struct
    cc31xx_nwp_msg_t msg = {
        .opcode = SL_OPCODE_WLAN_POLICYSETCOMMAND,
        // .RespOpcode = SL_OPCODE_WLAN_POLICYSETRESPONSE,
        .desc_buf    = &m,
        .desc_len    = sizeof(_WlanPoliciySetGet_t),
        .payload_len = 0,
    };

    // prepare response
    cc31xx_nwp_rsp_t res = {
        .res_len = sizeof(_BasicResponse_t),
        .data    = &m,
    };

    cc31xx_send_nwp_cmd(dev, &msg, &res);

    return m.res.status;
}

/**
 * @brief
 *
 * @param dev
 * @param domain
 * @param type
 * @param protocol
 * @return int16_t
 */
int16_t _nwp_sock_create(cc3100_t *dev, int16_t domain, int16_t type,
                         int16_t protocol)
{
    cc31xx_cmd_open_sock_t m = { 0 };
    m.req.Domain             = (uint8_t)domain;
    m.req.Type               = (uint8_t)type;
    m.req.Protocol           = (uint8_t)protocol;
    // create msg struct
    cc31xx_nwp_msg_t msg = {
        .opcode = SL_OPCODE_SOCKET_SOCKET,
        // .RespOpcode = SL_OPCODE_SOCKET_SOCKETRESPONSE,
        .desc_buf = &m.req,
        .desc_len = sizeof(_SocketCommand_t),
    };

    // prepare response
    cc31xx_nwp_rsp_t res = {
        .res_len = sizeof(_SocketResponse_t),
        .data    = &m.res,
    };

    if (cc31xx_send_nwp_cmd(dev, &msg, &res) != 0) {
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
 * @brief
 *
 * @param dev
 * @param sock
 * @param buf
 * @param bufLen
 * @param options
 * @return int16_t
 */
int16_t _nwp_read_raw_frame(cc3100_t *dev, int16_t sock, void *buf,
                            int16_t bufLen, int16_t options)
{
    /* create data object */
    cc31xx_cmd_raw_sock_t m = { 0 };

    /* set socket informations */
    m.req.sd             = sock;
    m.req.StatusOrLen    = bufLen;
    m.req.FamilyAndFlags = options & 0x0F;

    cc31xx_nwp_msg_t msg = {
        .opcode = SL_OPCODE_SOCKET_RECV,
        // .RespOpcode = SL_OPCODE_SOCKET_RECVASYNCRESPONSE,
        .desc_buf = &m,
        .desc_len = sizeof(_sendRecvCommand_t),
    };

    cc31xx_nwp_rsp_t res = {
        .res_len     = sizeof(_SocketResponse_t),
        .data        = &m.res, // reuse cmdDesc
        .payload_len = bufLen,
        .payload     = buf,
    };

    if (cc31xx_send_nwp_cmd(dev, &msg, &res) != 0) {
        DEBUG("ERR: failed to send NWP cmd (0x%x) \n", msg.opcode);
    }

    return m.res.statusOrLen;
}

int16_t _nwp_send_frame_to(cc3100_t *dev, int16_t sock, void *buf, uint16_t len,
                           int16_t flags, SlSockAddr_t *to, uint16_t toLen)
{
    cc31xx_cmd_send_to_t m = { 0 };

    cc31xx_nwp_msg_t req = {
        .opcode      = SL_OPCODE_SOCKET_SENDTO_V6,
        .desc_buf    = &m.req,
        .desc_len    = sizeof(_SocketAddrIPv6Command_t),
        .payload_len = len,
        .payload_buf = buf,
    };

    m.req.IpV4.lenOrPadding = len;
    m.req.IpV4.sd           = (uint8_t)sock;
    memcpy(&m.req.IpV4.address, to->sa_data, toLen);
    m.req.IpV4.FamilyAndFlags = (to->sa_family << 4) & 0xF0;
    // TODO: add port support
    // m.req.IpV4.port           = ;
    m.req.IpV4.FamilyAndFlags |= flags & 0x0F;

    if (cc31xx_send_nwp_cmd(dev, &req, NULL) != 0) {
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
int16_t _nwp_send_raw_frame(cc3100_t *dev, int16_t sock, void *buf, int16_t len,
                            int16_t options)
{
    // TODO: add sync for multiple sockets for now only one socket is
    // supported check mutex
    _sendRecvCommand_t data = { 0 };

    /* compute packet len based on socket type */
    uint16_t packetLen   = _max_payload_len(0);
    uint32_t optionsCopy = options;

    // fast ceil without math library
    uint8_t chunksCount = (len + packetLen - 1) / packetLen;
    // printf("Split into %d chunks \n", chunksCount);
    // create request response objects
    cc31xx_nwp_msg_t msg = {
        .opcode = SL_OPCODE_SOCKET_SEND,
        // // .RespOpcode = SL_OPCODE_DEVICE_DEVICEASYNCDUMMY,
        .desc_buf        = &data,
        .desc_len        = sizeof(_sendRecvCommand_t),
        .payload_hdr_buf = &optionsCopy,
        .payload_hdr_len = sizeof(uint32_t),
    };

    uint16_t bufOffset = 0;

    for (int i = 0; i < chunksCount; i++) {
        data.sd = (uint8_t)sock;
        data.FamilyAndFlags |= options & 0x0F;

        msg.payload_buf = (uint8_t *)(buf + bufOffset);

        // compute package lan either max package len or remainder
        msg.payload_len =
                len - bufOffset > packetLen ? packetLen : len - bufOffset;
        data.StatusOrLen = msg.payload_len;

        // increment buffer offset
        bufOffset += packetLen;

        // printf("Sending package len=%d chunk %d of %d\n", msg.payload_len,
        // i,
        //        chunksCount);

        // send socket data
        if (cc31xx_send_nwp_cmd(dev, &msg, NULL) != 0) {
            DEBUG("ERR: failed to send SOCK data");
            return -1;
        }

        // TODO: check response codes
    }

    return 0;
}

int16_t _nwp_set_wifi_filter(cc3100_t *dev, uint8_t filterOptions,
                             uint8_t *inBuf, uint16_t bufLen)
{
    cc31xx_cmd_set_rx_filter_t m = { 0 };
    m.req.RxFilterOperation      = filterOptions;
    m.req.InputBufferLength      = bufLen;
    // create msg struct
    cc31xx_nwp_msg_t msg = {
        .opcode = SL_OPCODE_WLAN_WLANRXFILTERSETCOMMAND,
        // .RespOpcode = SL_OPCODE_WLAN_WLANRXFILTERSETRESPONSE,
        .desc_buf    = &m.req,
        .desc_len    = sizeof(_WlanRxFilterSetCommand_t),
        .payload_buf = inBuf,
        .payload_len = bufLen,
    };

    // prepare response
    cc31xx_nwp_rsp_t res = {
        .res_len = sizeof(_WlanRxFilterSetCommandReponse_t),
        .data    = &m.res, // reuse cmdDesc
    };

    if (cc31xx_send_nwp_cmd(dev, &msg, &res) != 0) {
        printf("[WIFI] failed to set Rx Filter \n");
        return -1;
    }
    return m.res.Status;
}

/**
 * @brief Set the Socket Options object
 *
 * @param dev
 * @param sock
 * @param level
 * @param optionName
 * @param optionVal
 * @param optionLen
 * @return int16_t
 */
int16_t _nwp_set_sock_opt(cc3100_t *dev, uint16_t sock, uint16_t level,
                          uint16_t optionName, void *optionVal,
                          uint8_t optionLen)
{
    cc31xx_cmd_sock_opt_t m = { 0 };
    m.req.sd                = (uint8_t)sock;
    m.req.level             = (uint8_t)level;
    m.req.optionLen         = (uint8_t)optionLen;
    m.req.optionName        = (uint8_t)optionName;

    cc31xx_nwp_msg_t msg = {
        .opcode = SL_OPCODE_SOCKET_SETSOCKOPT,
        // .RespOpcode = SL_OPCODE_SOCKET_SETSOCKOPTRESPONSE,
        .desc_buf    = &m.req,
        .desc_len    = sizeof(_WlanRxFilterSetCommand_t),
        .payload_buf = optionVal,
        .payload_len = optionLen,
    };
    cc31xx_nwp_rsp_t res = {
        .res_len = sizeof(_SocketResponse_t),
        .data    = &m.res,
    };

    if (cc31xx_send_nwp_cmd(dev, &msg, &res) != 0) {
        printf("[WIFI] failed to set Rx Filter \n");
        return -1;
    }
    return m.res.statusOrLen;
}
