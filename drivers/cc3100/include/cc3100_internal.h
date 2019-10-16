#include <stdbool.h>
#include <stdint.h>

#ifndef CC3100_INTERNAL_H
#define CC3100_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cc3100_protocol.h"

#define cc3100_reg(addr) (*((volatile uint32_t *)(addr)))

// SPI SPEEDS
#define SPI_RATE_13M 13000000
#define SPI_RATE_20M 20000000
#define SPI_RATE_30M 30000000

#define REG_INT_MASK_SET 0x400F7088
#define REG_INT_MASK_CLR 0x400F708C
#define APPS_SOFT_RESET_REG 0x4402D000
#define OCP_SHARED_MAC_RESET_REG 0x4402E168
#define ROM_VERSION_ADDR 0x00000400

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

#define REQUEST_QUEUE_SIZE (2)

// TODO: finetune this shared buffer and maybe move it to dev
static uint8_t sharedBuffer[512];

/* RX Irqn handler type */
typedef void (*cc3100_rx_irqn_handler)(void);
#define cc3100_rx_irqn_handler cc3100_rx_irqn_handler

typedef struct cc3100_drv_con_info_t {
    uint8_t type; // type of current connection
    uint8_t ssidLen;
    uint8_t ssid[32];
    uint8_t bssid[6];
    bool connected;
} cc3100_drv_con_info_t;

typedef struct {
    uint8_t rate;
    uint8_t channel;
    int8_t rssi;
    uint8_t padding;
    uint32_t timestamp;
} cc31xx_ti_frame_header_t;

/**
 * @brief DriverMessage is used to send a message to the NWP. Below is a
 * simple diagram of a message to the NWP. Each block is transmitted a
 * separate
 *  +---------------------------+
 *  |                           |
 *  |        Msg Header         |   4 byte (OPCODE + length)
 *  |                           |
 *  +---------------------------+
 *  |                           |
 *  |      Cmd Description      |   n * 4 byte (length set by
 * cmdDescLen) |        (optional)         | | |
 *  +---------------------------+
 *  |                           |
 *  |      Payload Header       |   n * 4 byte (length set by
 * payloadLen) |        (optional)         | | |
 *  +---------------------------+
 *  |                           |
 *  |         Payload           |   n * 4 byte (length set by
 * payloadLen) |        (optional)         | | |
 *  +---------------------------+
 */
typedef struct {
    uint16_t opcode; /**< specifies opcode & total command size  */
    uint16_t resp_opcode;
    bool receiveFlagsViaRxPayload;

    void *desc_buf;    /**< command description */
    uint16_t desc_len; /**< length of descriptions*/

    /* uint16_t RespOpcode; response opcode */

    void *payload_buf;
    uint16_t payload_len;

    void *payload_hdr_buf;
    uint16_t payload_hdr_len;
} cc31xx_nwp_msg_t;

/**
 * @brief driver request object
 *
 */
typedef struct {
    uint8_t id;
    uint16_t opcode;

    // response description buffers
    uint8_t *desc_buf;
    uint16_t desc_len;

    // Payload buffers
    uint8_t *payload_buf;
    uint16_t payload_len;

    bool wait;

} cc31xx_nwp_req_t;

typedef struct {
    volatile cc31xx_nwp_req_t *queue[REQUEST_QUEUE_SIZE];
    uint8_t cur_len;
} cc31xx_nwp_queue_t;

/**
 * @brief CC31xx NWP response object
 *
 */
typedef struct {
    uint16_t res_len;     /**< full length of response respDesc + payload */
    uint16_t payload_len; /**< payload length */
    void *data;           /**< data buffer */
    void *payload;        /**< payload buffer */
} cc31xx_nwp_rsp_t;

/**
 * @brief internal queue used to store awaiting NWP request and handle incoming
 * responses
 *
 */
static cc31xx_nwp_queue_t _nwp_com = {
    .queue   = { NULL },
    .cur_len = 0,
};

/**
 * @brief used to notify blocking methods of a isr
 *
 */
static uint8_t _cc31xx_isr_state = 0;

/* static header buffer */
static cc3100_nwp_resp_header_t _cmd_header = {};
static cc31xx_ti_frame_header_t _ti_header  = {};

/**
 * @brief mask and unmask NWP data interrupt
 *
 */
static inline void mask_nwp_rx_irqn(void)
{
    (*(unsigned long *)N2A_INT_MASK_SET) = 0x1;
}
static inline void unmask_nwp_rx_irqn(void)
{
    (*(unsigned long *)N2A_INT_MASK_CLR) = 0x1;
}
void cc3100_cmd_handler(cc3100_t *dev, cc3100_nwp_resp_header_t *header);
int cc3100_read_from_nwp(cc3100_t *dev, void *buf, int len);
void cc3100_nwp_graceful_power_off(void);
void cc3100_nwp_power_on(void);
void cc3100_nwp_power_off(void);
void cc3100_nwp_rx_handler(void);
int cc3100_init_nwp(cc3100_t *dev);

uint8_t cc31xx_send_nwp_cmd(cc3100_t *dev, cc31xx_nwp_msg_t *msg,
                            cc31xx_nwp_rsp_t *res);
void cc31xx_send_header(cc3100_t *dev, cc3100_nwp_header_t *header);
int cc31xx_read_cmd_header(cc3100_t *dev, cc3100_nwp_resp_header_t *buf);
uint16_t _nwp_setup(cc3100_t *dev);
// int cc3100_read_from_nwp(void *buf, int len);
int cc31xx_send_to_nwp(cc3100_t *dev, const void *buf, int len);
#ifdef __cplusplus
}
#endif

#endif /* CC3200_INTERNAL_H */
       /** @} */