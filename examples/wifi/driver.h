#ifndef CC3200_WIFI_DRIVER
#define CC3200_WIFI_DRIVER

#include "proto.h"
#include "protocol.h"
#include <stdbool.h>
#include <stdint.h>

/* get response code for a request code */
#define GET_RESPONSE_OPCODE(x) (x - 0x8000))

/**
 * @brief SYNC and CNYS patterns used to establish communication with NWP
 * @{
 */

#define N2H_SYNC_PATTERN_SEQ_NUM_BITS \
    0x00000003 /**< 0-2 least significant bits used for seq num over SPI */
#define NWP_SYNC_PATTERN_SEQ_NUM_SET                                    \
    ((uint32_t)0x00000004) /* Flag that sequence number present in sync \
                              pattern */
#define N2H_SYNC_PATTERN_MASK \
    ((uint32_t)0xFFFFFFF8) /* Bits 3 to 31 - constant SYNC PATTERN */
#define N2H_SYNC_SPI_BUGS_MASK                                             \
    ((uint32_t)0x7FFF7F7F) /* Bits 7,15,31 - ignore the SPI (8,16,32 bites \
                              bus) error bits  */
#define BUF_SYNC_SPIM(pBuf) ((*(uint32_t *)(pBuf)) & N2H_SYNC_SPI_BUGS_MASK)
#define N2H_SYNC_SPIM (N2H_SYNC_PATTERN & N2H_SYNC_SPI_BUGS_MASK)

#define N2H_SYNC_SPIM_WITH_SEQ(TxSeqNum)                                      \
    ((N2H_SYNC_SPIM & N2H_SYNC_PATTERN_MASK) | NWP_SYNC_PATTERN_SEQ_NUM_SET | \
     ((TxSeqNum) & (N2H_SYNC_PATTERN_SEQ_NUM_BITS)))

/**
 * @brief Match sync frame without sequence number
 *
 */
#define MATCH_WOUT_SEQ_NUM(pBuf) (BUF_SYNC_SPIM(pBuf) == N2H_SYNC_SPIM)

/**
 * @brief Match sync frame with sequence number
 *
 */
#define MATCH_WITH_SEQ_NUM(pBuf, TxSeqNum) \
    (BUF_SYNC_SPIM(pBuf) == (N2H_SYNC_SPIM_WITH_SEQ(TxSeqNum)))

#define N2H_SYNC_PATTERN_MATCH(pBuf, TxSeqNum)             \
    (((*((_u32 *)pBuf) & NWP_SYNC_PATTERN_SEQ_NUM_SET) &&  \
      (MATCH_WITH_SEQ_NUM(pBuf, TxSeqNum))) ||             \
     (!(*((_u32 *)pBuf) & NWP_SYNC_PATTERN_SEQ_NUM_SET) && \
      (MATCH_WOUT_SEQ_NUM(pBuf))))

/* Sync pattern between CPU and NWP */
#define CPU_TO_NET_CHIP_SYNC_PATTERN   \
    {                                  \
        0xBBDDEEFF, 0x4321, 0x34, 0x12 \
    }

/* CNYS Pattern */
#define CPU_TO_NET_CHIP_CNYS_PATTERN   \
    {                                  \
        0xBBDDEEFF, 0x8765, 0x78, 0x56 \
    }

/**}@ */

/**
 * @brief 802.11 connection profile limits
 * @{
 */
#define MAX_SSID_LEN (32) /**< Max SSID length */
#define MAX_KEY_LEN (63)  /**< Max security key length */
#define MAX_USER_LEN (32) /**< Max username length for Enterprice networks */
#define MAX_ANON_USER_LEN (32)    /**< No idea */
#define MAX_SMART_CONFIG_KEY (16) /**< No idea */
/**}@ */

/**
 * @brief DriverMessage is used to send a message to the NWP. Below is a simple
 * diagram of a message to the NWP. Each block is transmitted a separate
 *  +---------------------------+
 *  |                           |
 *  |        Msg Header         |   4 byte (OPCODE + length)
 *  |                           |
 *  +---------------------------+
 *  |                           |
 *  |      Cmd Description      |   n * 4 byte (length set by cmdDescLen)
 *  |        (optional)         |
 *  |                           |
 *  +---------------------------+
 *  |                           |
 *  |      Payload Header       |   n * 4 byte (length set by payloadLen)
 *  |        (optional)         |
 *  |                           |
 *  +---------------------------+
 *  |                           |
 *  |         Payload           |   n * 4 byte (length set by payloadLen)
 *  |        (optional)         |
 *  |                           |
 *  +---------------------------+
 */
typedef struct DriverMessage {
    uint16_t Opcode;    /**< specifies opcode & total command size  */
    uint8_t cmdDescLen; /**< length of descriptions*/
    bool receiveFlagsViaRxPayload;
    uint16_t RespOpcode; /**< response opcode */
    void *cmdDesc;       /**< command description */
    void *payload;
    uint16_t payloadLen;
    void *payloadHeader;
    uint16_t payloadHeaderLen;
} DriverMessage;

/**
 * @brief CC32xx NWP response object
 *
 */
typedef struct DriverResponse {
    uint16_t resLen;     /**< full length of response respDesc + payload */
    uint16_t payloadLen; /**< payload length */
    void *data;          /**< data buffer */
    void *payload;       /**< payload buffer */
} DriverResponse;

/**
 * @brief CC32xx Network Config type
 *
 */
typedef enum {
    NWP_MAC_ADDRESS_SET               = 1, /**< get device IP addr */
    NWP_MAC_ADDRESS_GET               = 2, /**< set device IP addr */
    NWP_IPV4_STA_P2P_CL_GET_INFO      = 3,
    NWP_IPV4_STA_P2P_CL_DHCP_ENABLE   = 4, /**< set IP using DHCD (default) */
    NWP_IPV4_STA_P2P_CL_STATIC_ENABLE = 5, /**< set static IP persist in File
                                              System */
    NWP_IPV4_AP_P2P_GO_GET_INFO      = 6,
    NWP_IPV4_AP_P2P_GO_STATIC_ENABLE = 7,
    NWP_SET_HOST_RX_AGGR             = 8,
    NWP_MAX_SETTINGS                 = 0xFF /**<  */
} cc31xx_net_cfg_t;

/**
 * NWP message types
 */

typedef struct {
    _WlanAddGetEapProfile_t Args;
    _i8 Strings[MAX_SSID_LEN + MAX_KEY_LEN + MAX_USER_LEN + MAX_ANON_USER_LEN];
} _SlProfileParams_t;

typedef struct {
    _WlanConnectEapCommand_t Args;
    _i8 Strings[MAX_SSID_LEN + MAX_KEY_LEN + MAX_USER_LEN + MAX_ANON_USER_LEN];
} _WlanConnectCmd_t;

typedef union cc31xx_cmd_open_sock_t {
    _SocketCommand_t req;
    _SocketResponse_t res;
} cc31xx_cmd_open_sock_t;

/**
 * @brief cc31xx_cmd_raw_sock_tx is used for receiving raw tranceiver data
 *
 */
typedef union {
    _sendRecvCommand_t req;
    _SocketResponse_t res;
} cc31xx_cmd_raw_sock_t;

/**
 * @brief cc31xx_cmd_sock_opt_t is used to set socket options
 *
 */
typedef union {
    _setSockOptCommand_t req;
    _SocketResponse_t res;
} cc31xx_cmd_sock_opt_t;

/**
 * @brief cc31xx_cmd_send_to_t is used to send data over a IP level socket
 *
 */
typedef union {
    _SocketAddrCommand_u req;
    /*  no response for 'sendto' commands*/
} cc31xx_cmd_send_to_t;

/**
 * @brief cc31xx_cmd_set_wifi_cfg_t used to set wifi settings
 *
 */
typedef union {
    _WlanCfgSetGet_t req;
    _BasicResponse_t res;
} cc31xx_cmd_set_wifi_cfg_t;

/**
 * @brief cc31xx_cmd_set_wifi_mode_t used to set wifi mode (STA/AP)
 *
 */
typedef union {
    _WlanSetMode_t req;
    _BasicResponse_t res;
} cc31xx_cmd_set_wifi_mode_t;

/**
 * @brief cc31xx_cmd_connect_t used to connect to a wifi AP
 *
 */
typedef union {
    _WlanPoliciySetGet_t req;
    _BasicResponse_t res;
} cc31xx_cmd_wifi_policy_set_t;

/**
 * @brief
 *
 */
typedef union {
    _WlanRxFilterSetCommand_t req;
    _WlanRxFilterSetCommandReponse_t res;
} cc31xx_cmd_set_rx_filter_t;
;

/**
 * @brief cc31xx_cmd_add_profile_t used to add a wifi connection profile
 *
 */
typedef union {
    _SlProfileParams_t req;
    _BasicResponse_t res;
} cc31xx_cmd_add_profile_t;

/**
 * @brief cc31xx_cmd_add_profile_t used to add a wifi connection profile
 *
 */
typedef union {
    _WlanProfileDelGetCommand_t req;
    _SlProfileParams_t res;
} cc31xx_cmd_del_get_profile_t;

/**
 * @brief cc31xx_cmd_connect_t used to connect to a wifi AP
 *
 */
typedef union {
    _WlanConnectCmd_t req;
    _BasicResponse_t res;
} cc31xx_cmd_connect_t;

/* read from NWP */
int read(void *buf, int len);

/* write to NWP */
int send(const void *in, int len);

/* send NWP message header */
void send_header(_SlGenericHeader_t *header);

/* read NWP message header */
int read_cmd_header(cc3200_SlResponseHeader *resp);

/* send power on init code to NWP */
void graceful_nwp_shutdown(void);

/* write short sync frame to NWP */
void send_short_sync(void);

#endif