
#ifndef CC3200_WIFI_PROTO
#define CC3200_WIFI_PROTO

#include "protocol.h"
#include "vendor/hw_common_reg.h"
#include "vendor/hw_memmap.h"

#include <stdint.h>

/* N2A_INT_MASK_SET -                   (COMMON_REG_BASE +
 * COMMON_REG_O_NW_INT_MASK_SET)  */
#define N2A_INT_MASK_SET (COMMON_REG_BASE + COMMON_REG_O_NW_INT_MASK_SET)

/* N2A_INT_MASK_CLR -                   (COMMON_REG_BASE +
 * COMMON_REG_O_NW_INT_MASK_CLR)  */
#define N2A_INT_MASK_CLR (COMMON_REG_BASE + COMMON_REG_O_NW_INT_MASK_CLR)

// base address of the wifi spi peripherials
#define WIFI_SPI_BASE 0x44022000
#define WIFI_REG (cc3200_spi_t *)WIFI_SPI_BASE

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

// SPI SPEEDS
#define SPI_RATE_13M 13000000
#define SPI_RATE_20M 20000000
#define SPI_RATE_30M 30000000

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

typedef void (*SimpleLinkEventHandler)(void);
#define SimpleLinkEventHandler SimpleLinkEventHandler

typedef struct CC3200_RomInfo {
    uint16_t majorVer;
    uint16_t minorVer;
    uint16_t ucSubMinorVerNum;
    uint16_t ucDay;
    uint16_t ucMonth;
    uint16_t ucYear;
} CC3200_RomInfo;

typedef enum wlan_security_t {
    SEC_TYPE_OPEN = 0,
    SEC_TYPE_WEP,
    SEC_TYPE_WPA_WPA2,
} wlan_security_t;

typedef struct WifiProfileConfig {
    _WlanAddGetProfile_t common;
    // base station name
    char *ssid;
    char *key;
    // enterprise config
    // char user[MAX_USER_LEN];

} WifiProfileConfig;

/* IpV4 socket address */
typedef struct SlSockAddr_t {
    uint16_t sa_family;  /* Address family (e.g. , AF_INET)     */
    uint8_t sa_data[14]; /* Protocol- specific address information*/
} SlSockAddr_t;

typedef struct {
    _SlGenericHeader_t GenHeader;
    uint8_t TxPoolCnt;
    uint8_t DevStatus;
    uint8_t SocketTXFailure;
    uint8_t SocketNonBlocking;
} cc3200_SlResponseHeader;

typedef union {
    _DeviceSetGet_t Cmd;
    _DeviceSetGet_t Rsp;
} cc3200_DeviceMsgGet_u;

typedef struct {
    wifi_opcode Opcode;
    uint8_t TxDescLen;
    uint8_t RxDescLen;
} WifiCtrlCmd;

typedef struct WifiModule {
    int fd;
} WifiModule;

typedef struct cc3200_SpiStatusReg {
    uint8_t unknown;
    uint8_t rxs;
    uint8_t txs;
} cc3200_SpiStatusReg;

typedef struct {
    uint32_t ChipId;
    uint8_t FwVersion[4];
    uint8_t PhyVersion[4];
    uint8_t NwpVersion[4];
    uint16_t RomVersion;
    uint16_t Padding;
} SlVersionFull;

typedef struct {
    wifi_opcode Opcode;
    uint8_t TxDescLen;
    uint8_t RxDescLen;
} _SlCmdCtrl_t;

typedef struct {
    _u16 TxPayloadLen;
    _u16 RxPayloadLen;
    _u16 ActualRxPayloadLen;
    _u8 *pTxPayload;
    _u8 *pRxPayload;
} _SlCmdExt_t;

typedef struct _SlArgsData_t {
    _u8 *pArgs;
    _u8 *pData;
} _SlArgsData_t;

// typedef struct _SlPoolObj_t {
//   _SlSyncObj_t SyncObj;
//   _u8 *pRespArgs;
//   _u8 ActionID;
//   _u8 AdditionalData; /* use for socketID and one bit which indicate supprt
//   IPV6
//                          or not (1=support, 0 otherwise) */
//   _u8 NextIndex;

// } _SlPoolObj_t;

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

// typedef struct _SlActionLookup_t {
//   _u8 ActionID;
//   _u16 ActionAsyncOpcode;
//   _SlSpawnEntryFunc_t AsyncEventHandler;

// } _SlActionLookup_t;

// typedef struct {
//   _u8 TxPoolCnt;
//   _SlLockObj_t TxLockObj;
//   _SlSyncObj_t TxSyncObj;
// } _SlFlowContCB_t;

typedef enum {
    RECV_RESP_CLASS,
    CMD_RESP_CLASS,
    ASYNC_EVT_CLASS,
    DUMMY_MSG_CLASS
} _SlRxMsgClass_e;

// typedef struct {
//   _u8 *pAsyncBuf; /* place to write pointer to buffer with CmdResp's Header +
//                      Arguments */
//   _u8 ActionIndex;
//   _SlSpawnEntryFunc_t AsyncEvtHandler; /* place to write pointer to
//   AsyncEvent
//                                           handler (calc-ed by Opcode)   */
//   _SlRxMsgClass_e RxMsgClass;          /* type of Rx message          */
// } AsyncExt_t;

typedef _u8 _SlSd_t;

// typedef struct {
//   _SlCmdCtrl_t *pCmdCtrl;
//   _u8 *pTxRxDescBuff;
//   _SlCmdExt_t *pCmdExt;
//   AsyncExt_t AsyncExt;
// } _SlFunctionParams_t;

#endif
