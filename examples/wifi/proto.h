#ifndef CC3200_WIFI_PROTO
#define CC3200_WIFI_PROTO

#include "protocol.h"
#include <stdint.h>

// base address of the wifi spi peripherials
#define WIFI_SPI_BASE 0x44022000
#define WIFI_REG (cc3200_spi_t *)WIFI_SPI_BASE

// ROM VERSIONS
#define ROM_VER_PG1_21 1
#define ROM_VER_PG1_32 2
#define ROM_VER_PG1_33 3

#define REG_INT_MASK_SET 0x400F7088
#define REG_INT_MASK_CLR 0x400F708C
#define APPS_SOFT_RESET_REG 0x4402D000
#define OCP_SHARED_MAC_RESET_REG 0x4402E168
#define ROM_VERSION_ADDR 0x00000400

// TODO: maybe remove
#define INT_PRIORITY_LVL_0 0x00
#define INT_PRIORITY_LVL_1 0x20
#define INT_PRIORITY_LVL_2 0x40
#define INT_PRIORITY_LVL_3 0x60
#define INT_PRIORITY_LVL_4 0x80
#define INT_PRIORITY_LVL_5 0xA0
#define INT_PRIORITY_LVL_6 0xC0
#define INT_PRIORITY_LVL_7 0xE0

// SPI SPEEDS
#define SPI_RATE_13M 13000000
#define SPI_RATE_20M 20000000

#define CPU_TO_NET_CHIP_SYNC_PATTERN                                           \
  { 0xBBDDEEFF, 0x4321, 0x34, 0x12 }
#define CPU_TO_NET_CHIP_CNYS_PATTERN                                           \
  { 0xBBDDEEFF, 0x8765, 0x78, 0x56 }

#define mscpi_reg uint32_t
#define wifi_opcode uint16_t

// Wifi module comunication utils
#define N2H_SYNC_PATTERN_SEQ_NUM_BITS                                          \
  ((uint32_t)0x00000003) /* Bits 0..1    - use the 2 LBS for seq num */
#define N2H_SYNC_PATTERN_SEQ_NUM_EXISTS                                        \
  ((uint32_t)0x00000004) /* Bit  2       - sign that sequence number exists in \
                            the sync pattern */
#define N2H_SYNC_PATTERN_MASK                                                  \
  ((uint32_t)0xFFFFFFF8) /* Bits 3..31   - constant SYNC PATTERN */
#define N2H_SYNC_SPI_BUGS_MASK                                                 \
  ((uint32_t)0x7FFF7F7F) /* Bits 7,15,31 - ignore the SPI (8,16,32 bites bus)  \
                            error bits  */
#define BUF_SYNC_SPIM(pBuf) ((*(uint32_t *)(pBuf)) & N2H_SYNC_SPI_BUGS_MASK)
#define N2H_SYNC_SPIM (N2H_SYNC_PATTERN & N2H_SYNC_SPI_BUGS_MASK)
#define N2H_SYNC_SPIM_WITH_SEQ(TxSeqNum)                                       \
  ((N2H_SYNC_SPIM & N2H_SYNC_PATTERN_MASK) | N2H_SYNC_PATTERN_SEQ_NUM_EXISTS | \
   ((TxSeqNum) & (N2H_SYNC_PATTERN_SEQ_NUM_BITS)))
#define MATCH_WOUT_SEQ_NUM(pBuf) (BUF_SYNC_SPIM(pBuf) == N2H_SYNC_SPIM)
#define MATCH_WITH_SEQ_NUM(pBuf, TxSeqNum)                                     \
  (BUF_SYNC_SPIM(pBuf) == (N2H_SYNC_SPIM_WITH_SEQ(TxSeqNum)))
#define N2H_SYNC_PATTERN_MATCH(pBuf, TxSeqNum)                                 \
  (((*((uint32_t *)pBuf) & N2H_SYNC_PATTERN_SEQ_NUM_EXISTS) &&                 \
    (MATCH_WITH_SEQ_NUM(pBuf, TxSeqNum))) ||                                   \
   (!(*((uint32_t *)pBuf) & N2H_SYNC_PATTERN_SEQ_NUM_EXISTS) &&                \
    (MATCH_WOUT_SEQ_NUM(pBuf))))

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
  uint32_t FwVersion[4];
  uint8_t PhyVersion[4];
} _SlPartialVersion;

typedef struct {
  _SlPartialVersion ChipFwAndPhyVersion;
  uint32_t NwpVersion[4];
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
