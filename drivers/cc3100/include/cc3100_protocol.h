#ifndef CC31XX_PROTOCOL_INTERNAL_H
#define CC31XX_PROTOCOL_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "vendor/hw_common_reg.h"
#include "vendor/hw_memmap.h"
#include "vendor/sl_nwp.h"

#include <stdint.h>

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
 * @brief Match sync frame with sequence number
 *
 */
#define MATCH_WITH_SEQ_NUM(pBuf, TxSeqNum) \
    (BUF_SYNC_SPIM(pBuf) == (N2H_SYNC_SPIM_WITH_SEQ(TxSeqNum)))

/**
 * @brief Match sync frame without sequence number
 *
 */
#define MATCH_WOUT_SEQ_NUM(pBuf) (BUF_SYNC_SPIM(pBuf) == N2H_SYNC_SPIM)

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

/* N2A_INT_MASK_SET -                   (COMMON_REG_BASE +
 * COMMON_REG_O_NW_INT_MASK_SET)  */
#define N2A_INT_MASK_SET (COMMON_REG_BASE + COMMON_REG_O_NW_INT_MASK_SET)

/* N2A_INT_MASK_CLR -                   (COMMON_REG_BASE +
 * COMMON_REG_O_NW_INT_MASK_CLR)  */
#define N2A_INT_MASK_CLR (COMMON_REG_BASE + COMMON_REG_O_NW_INT_MASK_CLR)

// base address of the wifi spi peripherials
#define WIFI_SPI_BASE 0x44022000
#define WIFI_REG (cc3100_spi_t *)WIFI_SPI_BASE

#define SL_MAC_ADDRESS_GET 2

// ROM VERSIONS
#define ROM_VER_PG1_21 1
#define ROM_VER_PG1_32 2
#define ROM_VER_PG1_33 3

#define REG_INT_MASK_SET 0x400F7088
#define REG_INT_MASK_CLR 0x400F708C
#define APPS_SOFT_RESET_REG 0x4402D000
#define OCP_SHARED_MAC_RESET_REG 0x4402E168
#define ROM_VERSION_ADDR 0x00000400

#define mscpi_reg uint32_t
#define wifi_opcode uint16_t

#define SL_RAW_RF_TX_PARAMS_CHANNEL_SHIFT (0)
#define SL_RAW_RF_TX_PARAMS_RATE_SHIFT (6)
#define SL_RAW_RF_TX_PARAMS_POWER_SHIFT (11)
#define SL_RAW_RF_TX_PARAMS_PREAMBLE_SHIFT (15)

#define SL_RAW_RF_TX_PARAMS(chan, rate, power, preamble) \
    ((chan << SL_RAW_RF_TX_PARAMS_CHANNEL_SHIFT) |       \
     (rate << SL_RAW_RF_TX_PARAMS_RATE_SHIFT) |          \
     (power << SL_RAW_RF_TX_PARAMS_POWER_SHIFT) |        \
     (preamble << SL_RAW_RF_TX_PARAMS_PREAMBLE_SHIFT))

#define SL_POLICY_CONNECTION (0x10)
#define SL_POLICY_SCAN (0x20)
#define SL_POLICY_PM (0x30)
#define SL_POLICY_P2P (0x40)

#define VAL_2_MASK(position, value) ((1 & (value)) << (position))
#define MASK_2_VAL(position, mask) (((1 << position) & (mask)) >> (position))

#define SL_CONNECTION_POLICY(Auto, Fast, Open, anyP2P, autoSmartConfig) \
    (VAL_2_MASK(0, Auto) | VAL_2_MASK(1, Fast) | VAL_2_MASK(2, Open) |  \
     VAL_2_MASK(3, anyP2P) | VAL_2_MASK(4, autoSmartConfig))
#define SL_SCAN_POLICY_EN(policy) (MASK_2_VAL(0, policy))
#define SL_SCAN_POLICY(Enable) (VAL_2_MASK(0, Enable))

#define SL_NORMAL_POLICY (0)
#define SL_LOW_LATENCY_POLICY (1)
#define SL_LOW_POWER_POLICY (2)
#define SL_ALWAYS_ON_POLICY (3)
#define SL_LONG_SLEEP_INTERVAL_POLICY (4)

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
 * @brief 802.11 connection profile limits
 * @{
 */
#define MAX_SSID_LEN (32) /**< Max SSID length */
#define MAX_KEY_LEN (63)  /**< Max security key length */
#define MAX_USER_LEN (32) /**< Max username length for Enterprice networks */
#define MAX_ANON_USER_LEN (32)    /**< No idea */
#define MAX_SMART_CONFIG_KEY (16) /**< No idea */
/**}@ */

typedef enum {
    SOCKET_0,
    SOCKET_1,
    SOCKET_2,
    SOCKET_3,
    SOCKET_4,
    SOCKET_5,
    SOCKET_6,
    SOCKET_7,
    MAX_SOCKET_ENUM_IDX,
    ACCEPT_ID = MAX_SOCKET_ENUM_IDX,
    CONNECT_ID,
    SELECT_ID,
    GETHOSYBYNAME_ID,
    GETHOSYBYSERVICE_ID,
    PING_ID,
    START_STOP_ID,
    RECV_ID
} _SlActionID_e;

typedef enum {
    RECV_RESP_CLASS,
    CMD_RESP_CLASS,
    ASYNC_EVT_CLASS,
    DUMMY_MSG_CLASS
} _SlRxMsgClass_e;

typedef uint8_t _SlSd_t;

typedef struct {
    uint32_t ChipId;
    uint8_t FwVersion[4];
    uint8_t PhyVersion[4];
    uint8_t NwpVersion[4];
    uint16_t RomVersion;
    uint16_t Padding;
} SlVersionFull;

typedef struct {
    uint16_t Opcode;
    uint8_t TxDescLen;
    uint8_t RxDescLen;
} _SlCmdCtrl_t;

typedef struct {
    uint16_t TxPayloadLen;
    uint16_t RxPayloadLen;
    uint16_t ActualRxPayloadLen;
    uint8_t *pTxPayload;
    uint8_t *pRxPayload;
} _SlCmdExt_t;

typedef struct _SlArgsData_t {
    uint8_t *pArgs;
    uint8_t *pData;
} _SlArgsData_t;

typedef struct {
    _WlanAddGetEapProfile_t Args;
    _i8 Strings[MAX_SSID_LEN + MAX_KEY_LEN + MAX_USER_LEN + MAX_ANON_USER_LEN];
} _SlProfileParams_t;

typedef struct {
    _WlanConnectEapCommand_t Args;
    _i8 Strings[MAX_SSID_LEN + MAX_KEY_LEN + MAX_USER_LEN + MAX_ANON_USER_LEN];
} _WlanConnectCmd_t;

/* IpV4 socket address */
typedef struct SlSockAddr_t {
    uint16_t sa_family;  /* Address family (e.g. , AF_INET)     */
    uint8_t sa_data[14]; /* Protocol- specific address information*/
} SlSockAddr_t;

typedef struct WifiProfileConfig {
    _WlanAddGetProfile_t common;
    // base station name
    char *ssid;
    char *key;
    // enterprise config
    // char user[MAX_USER_LEN];

} WifiProfileConfig;

typedef struct {
    uint32_t Long;
    uint16_t Short;
    uint8_t Byte1;
    uint8_t Byte2;
} cc3100_nwp_sync_pattern_t;

typedef struct CC3100_RomInfo {
    uint16_t majorVer;
    uint16_t minorVer;
    uint16_t ucSubMinorVerNum;
    uint16_t ucDay;
    uint16_t ucMonth;
    uint16_t ucYear;
} cc31xx_rom_info_t;

typedef enum wlan_security_t {
    SEC_TYPE_OPEN = 0,
    SEC_TYPE_WEP,
    SEC_TYPE_WPA_WPA2,
} cc31xx_wlan_security_t;

typedef struct {
    int8_t SecType;
    uint8_t SsidLen;
    uint8_t Priority;
    uint8_t Bssid[6];
    uint8_t PasswordLen;
    uint8_t WepKeyId;
} cc3100_nwp_80211_profile_t;

typedef struct cc3100_nwp_80211_profile_config_t {
    cc3100_nwp_80211_profile_t common;
    // base station name
    char *ssid;
    char *key;
    // enterprise config
    // char user[MAX_USER_LEN];
} cc3100_nwp_80211_profile_config_t;

typedef struct {
    uint16_t opcode;
    uint16_t len;
} cc3100_nwp_header_t;

typedef _SlResponseHeader_t cc3100_nwp_resp_header_t;

typedef struct cc3100_SpiStatusReg {
    uint8_t unknown;
    uint8_t rxs;
    uint8_t txs;
} cc3100_SpiStatusReg;

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

#ifdef __cplusplus
}
#endif

#endif /* CC31XX_PROTOCOL_INTERNAL_H */
       /** @} */