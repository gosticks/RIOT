#ifndef CC3200_WIFI_PROTO
#define CC3200_WIFI_PROTO

#include <stdint.h>

// base address of the wifi spi peripherials
#define WIFI_SPI_BASE 0x44022000

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
// TODO: for now a copy of TI's mactros
/*  2 LSB of the N2H_SYNC_PATTERN are for sequence number
only in SPI interface
support backward sync pattern */
#define N2H_SYNC_PATTERN (uint32_t)0xABCDDCBA
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
  uint16_t Opcode;
  uint16_t Len;
} cc3200_CmdHeader;

typedef struct {
  cc3200_CmdHeader GenHeader;
  uint8_t TxPoolCnt;
  uint8_t DevStatus;
  uint8_t SocketTXFailure;
  uint8_t SocketNonBlocking;
} cc3200_SlResponseHeader;

typedef struct {
  uint16_t Status;
  uint16_t DeviceSetId;
  uint16_t Option;
  uint16_t ConfigLen;
} cc3200_DeviceSetGet_t;

typedef union {
  cc3200_DeviceSetGet_t Cmd;
  cc3200_DeviceSetGet_t Rsp;
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

typedef struct cc3200_hwinfo {
  unsigned long u1;
  unsigned long u2;
  unsigned long u3;
  unsigned long u4;
} cc3200_hwinfo;

typedef struct cc3200_rev {
  mscpi_reg u1;
  mscpi_reg u2;
  mscpi_reg u3;
} cc3200_rev;

typedef struct CC3200_MCSPI {
  unsigned long rev;    // hardware revision
  cc3200_hwinfo hwinfo; // hardware info (HDL generics)
  // TODO: proper sysconfig struct
  char pad0[240];          // Sysconfig
  cc3200_rev sys_rev;      // IRQEnable
  mscpi_reg sys_conf;      // System config
  mscpi_reg sys_status;    // Sysstatus
  mscpi_reg irq_status;    // IRQStatus
  mscpi_reg irq_enable;    // IRQEnable
  mscpi_reg wakeup_enable; // Wakeupenable
  mscpi_reg sys_test;      // system test mode
  mscpi_reg module_ctl;    // MODULE CTL
  mscpi_reg ch0_conf_ctl;  // CH0CONF CTL
  mscpi_reg stat;          // CH0 Status register
  mscpi_reg ctrl;          // CH0 Control register
  mscpi_reg tx0;           // single spi transmit word
  mscpi_reg rx0;           // single spi receive word
} CC3200_MCSPI;


typedef struct {
  uint32_t Long;
  uint16_t Short;
  uint8_t Byte1;
  uint8_t Byte2;
} _SlSyncPattern_t;

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

#endif