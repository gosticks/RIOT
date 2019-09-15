#include "include/cc3200_internal.h"
#include "include/cc3200_protocol.h"
#include <stdbool.h>
#include <stddef.h>

#define ENABLE_DEBUG (1)

// TODO: finetune this shared buffer and maybe move it to dev
static uint8_t sharedBuffer[512];

// TODO: maybe merge with dev or params of the driver
cc3200_drv_state_t state = { .requestQueue = { NULL },
                             .curReqCount  = 0,
                             .con          = { .connected = 0 } };

/* forward declarations */
void cc3200_add_to_drv_queue(volatile cc3200_drv_req_t *req);
uint8_t cc3200_remove_from_drv_queue(volatile cc3200_drv_req_t *req);
void cc3200_nwp_rx_handler(void *value);
/**
 * @brief power on nwp, register RX irqn handler and await nwp response
 *
 * @return int
 */
int cc3200_init_nwp(void)
{
    // register callback when wifi module is powered back on
    cc3200_register_nwp_rx_irqn((cc3200_rx_irqn_handler)cc3200_nwp_rx_handler);

    // disable uDMA channels
    DEBUG("[NWP] powering on\n");

    volatile cc3200_drv_req_t r = { .Opcode         = 0x0008,
                                    .Waiting        = true,
                                    .DescBufferSize = 0 };
    addToQueue(&r);

    powerOnWifi();
    DEBUG("[WIFI] waiting for NWP power on response\n");
    // TODO: add a timeout
    while (r.Waiting) {
    }
    DEBUG("[WIFI] NWP booted\n");
    return 0;
}

/**
 * @brief cc3200_nwp_rx_handler is the default RX handlers for the NWP
 *
 */
void cc3200_nwp_rx_handler(void *value)
{
    (void)value;
    // keep track of the current command count
    // handledIrqsCount++;
    mask_nwp_rx_irqn();

    cc3200_SlResponseHeader cmd;
    read_cmd_header(&cmd);
    cc3200_cmd_handler(&cmd);

    cortexm_isr_end();
}

/**
 * @brief cc3200_cmd_handler handles requested driver command responses
 * @param header 
 */
void cc3200_cmd_handler(cc3200_SlResponseHeader *header)
{
    DEBUG("[NWP] RECV: \033[1;32m%x \033[0m, len=%d\n",
          header->GenHeader.Opcode, header->GenHeader.Len);

    volatile cc3200_drv_req_t *req = NULL;

    // check if any command is waiting on a response if so we can use
    // its buffer to avoid having to copy the data
    for (uint8_t i = 0; state.curReqCount != 0 && i < REQUEST_QUEUE_SIZE; i++) {
        if (state.requestQueue[i] == NULL ||
            state.requestQueue[i]->Opcode != header->GenHeader.Opcode) {
            continue;
        }
        req          = state.requestQueue[i];
        req->Waiting = false;
        removeFromQueue(state.requestQueue[i]);
    }

    // when we have a request read the buffer to the request
    if (req != NULL) {
        int16_t remainder = header->GenHeader.Len - req->DescBufferSize;
        if (remainder < 0) {
            remainder = 0;
        }
        if (req->DescBufferSize > 0 && remainder >= 0) {
            read(req->DescBuffer, req->DescBufferSize);
        }

        // payload can sometimes be smaller then expected
        if (remainder < req->PayloadBufferSize) {
            // DEBUG("NWP: Read payload %d bytes \n", (remainder + 3) & ~0x03);
            read(req->PayloadBuffer, (remainder + 3) & ~0x03);
            remainder = 0;
        } else {
            remainder -= req->PayloadBufferSize;
            // DEBUG("NWP: Read payload %d bytes \n", req->PayloadBufferSize);
            if (req->PayloadBufferSize > 0 && remainder >= 0) {
                read(req->PayloadBuffer, req->PayloadBufferSize);
            }
        }

        // read all remaining data
        if (remainder > 0) {
            read(sharedBuffer, remainder);
        }
    } else {
        // otherwise read everything into shared buffer;
        read(sharedBuffer, header->GenHeader.Len);
    }

    // handle commands
    // switch (header->GenHeader.Opcode) {
    // case SL_OPCODE_WLAN_WLANASYNCCONNECTEDRESPONSE:
    //     handleWlanConnectedResponse();
    //     break;
    // case SL_OPCODE_NETAPP_IPACQUIRED:
    //     handleIpAcquired();
    //     break;
    // }

    unmask_nwp_rx_irqn();
}

/**
 * @brief register irqn handler
 *
 * @param handler to be registered
 */
static inline cc3200_register_nwp_rx_irqn(cc3200_rx_irqn_handler handler)
{
    ROM_IntRegister(INT_NWPIC, hanlder);
    ROM_IntPrioritySet(INT_NWPIC, 0x20);
    ROM_IntPendClear(INT_NWPIC);
    ROM_IntEnable(INT_NWPIC);
}

/**
 * @brief cc3200_unregister_nwp_rx_irqn unregisters NWP rx irqn handler
 *
 */
static inline cc3200_unregister_nwp_rx_irqn(void)
{
    ROM_IntDisable(INT_NWPIC);
    ROM_IntUnregister(INT_NWPIC);
    ROM_IntPendClear(INT_NWPIC);
}

/**
 * @brief add driver request to the queue, the queue will be checked after each
 * succeffull driver RX callback
 *
 * @param cc3200_drv_req_t
 */
void cc3200_add_to_drv_queue(volatile cc3200_drv_req_t *req)
{
    // wait till the queue is free
    while (state.curReqCount >= REQUEST_QUEUE_SIZE) {
    }
    state.requestQueue[state.curReqCount] = req;
    state.curReqCount++;
}

/**
 * @brief remove a driver request from the driver queue in normal operations
 * this happens after a timeout on the command or a NWP response
 *
 * @param cc3200_drv_req_t
 * @return uint8_t
 */
uint8_t cc3200_remove_from_drv_queue(volatile cc3200_drv_req_t *req)
{
    for (uint8_t i = 0; i < REQUEST_QUEUE_SIZE; i++) {
        if (state.requestQueue[i] == req) {
            state.requestQueue[i] = NULL;
            state.curReqCount--;
            return 1;
        }
    }
    return 0;
}

static inline void mask_nwp_rx_irqn(void)
{
    (*(unsigned long *)N2A_INT_MASK_SET) = 0x1;
}
static inline void unmask_nwp_rx_irqn(void)
{
    (*(unsigned long *)N2A_INT_MASK_CLR) = 0x1;
}