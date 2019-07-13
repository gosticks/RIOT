/*
 * Copyright (C) 2014 Loci Controls Inc.
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup        cpu_cc3200_uart CC3200 UART
 * @ingroup         cpu_cc3200_regs
 * @{
 *
 * @file
 * @brief           CC3200 UART interface
 *
 * @author          Wladislaw Meixner <wladislaw.meixner@campus.lmu.de>
 */

#ifndef CC3200_UART_H
#define CC3200_UART_H

#include "cc3200.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef unsigned long cc3200_uart_reg;

    typedef struct cc3200_uart_t
    {
        cc3200_uart_reg dr; /**< UART Data Register */
        union {
            cc3200_uart_reg rsr; /**< UART receive status and error clear */
            cc3200_uart_reg ecr; /**< UART receive status and error clear */
        } cc3200_uart_dr;

        cc3200_reg_t RESERVED1[4];

        union {
            cc3200_uart_reg raw; // flag
            struct
            {
                cc3200_uart_reg CTS : 1;        /**< Clear to send (UART1 only) */
                cc3200_uart_reg DSR : 1;        /**< Data Set Ready */
                cc3200_uart_reg DCD : 1;        /**< Data Carrier Detect */
                cc3200_uart_reg BUSY : 1;       /**< UART busy */
                cc3200_uart_reg RXFE : 1;       /**< UART receive FIFO empty */
                cc3200_uart_reg TXFF : 1;       /**< UART transmit FIFO full */
                cc3200_uart_reg RXFF : 1;       /**< UART receive FIFO full */
                cc3200_uart_reg TXFE : 1;       /**< UART transmit FIFO empty */
                cc3200_uart_reg RI : 1;         /**< UART ring indicator */
                cc3200_uart_reg RESERVED1 : 23; /**< Reserved bits */
            } bits;
        } flags;

        cc3200_uart_reg RESERVED2; /**< Reserved byte */
        cc3200_uart_reg ILPR;      /**< UART IrDA Low-Power Register */
        cc3200_uart_reg IBRD;      /**< UART Integer Baud-Rate Divisor */
        cc3200_uart_reg FBRD;      /**< UART Fractional Baud-Rate Divisor */

        /**
     * @brief Line control register
     */
        union {
            cc3200_uart_reg raw; /**< UART Line Control Register */
            struct
            {
                cc3200_uart_reg BRK : 1;       /**< UART send break */
                cc3200_uart_reg PEN : 1;       /**< UART parity enable */
                cc3200_uart_reg EPS : 1;       /**< UART even parity select */
                cc3200_uart_reg STP2 : 1;      /**< UART two stop bits select */
                cc3200_uart_reg FEN : 1;       /**< UART enable FIFOs */
                cc3200_uart_reg WLEN : 2;      /**< UART word length */
                cc3200_uart_reg SPS : 1;       /**< UART stick parity select */
                cc3200_uart_reg RESERVED : 24; /**< Reserved bits */
            } bits;
        } LCRH;

        /**
     * @brief Control register
     */
        union {
            cc3200_uart_reg raw; /**< UART Control */
            struct
            {
                cc3200_uart_reg UARTEN : 1;      /**< UART enable */
                cc3200_uart_reg SIREN : 1;       /**< UART SIR enable */
                cc3200_uart_reg SIRLP : 1;       /**< UART SIR low-power mode */
                cc3200_uart_reg RESERVED11 : 1;  /**< Reserved bits */
                cc3200_uart_reg EOT : 1;         /**< End of transmission */
                cc3200_uart_reg HSE : 1;         /**< High-speed enable */
                cc3200_uart_reg LIN : 1;         /**< LIN mode enable */
                cc3200_uart_reg LBE : 1;         /**< UART loop back enable */
                cc3200_uart_reg TXE : 1;         /**< UART transmit enable */
                cc3200_uart_reg RXE : 1;         /**< UART receive enable */
                cc3200_uart_reg RESERVED12 : 4;  /**< Reserved bits */
                cc3200_uart_reg RTSEN : 1;       /**< U1RTS Hardware flow control enable */
                cc3200_uart_reg CTSEN : 1;       /**< U1CTS Hardware flow control enable */
                cc3200_uart_reg RESERVED13 : 16; /**< Reserved bits */
            } bits;
        } CTL;

        /**
     * @brief Interrupt FIFO level select register
     */
        union {
            cc3200_uart_reg raw; /**< UART interrupt FIFO Level Select */
            struct
            {
                cc3200_uart_reg TXIFLSEL : 3;  /**< UART transmit interrupt FIFO level select */
                cc3200_uart_reg RXIFLSEL : 3;  /**< UART receive interrupt FIFO level select */
                cc3200_uart_reg RESERVED : 26; /**< Reserved bits */
            } bits;
        } IFLS;

        /**
     * @brief Interrupt mask register
     */
        union {
            cc3200_uart_reg raw; /**< UART Interrupt Mask */
            struct
            {
                cc3200_uart_reg RESERVED3 : 4;  /**< Reserved bits */
                cc3200_uart_reg RXIM : 1;       /**< UART receive interrupt mask */
                cc3200_uart_reg TXIM : 1;       /**< UART transmit interrupt mask */
                cc3200_uart_reg RTIM : 1;       /**< UART receive time-out interrupt mask */
                cc3200_uart_reg FEIM : 1;       /**< UART framing error interrupt mask */
                cc3200_uart_reg PEIM : 1;       /**< UART parity error interrupt mask */
                cc3200_uart_reg BEIM : 1;       /**< UART break error interrupt mask */
                cc3200_uart_reg OEIM : 1;       /**< UART overrun error interrupt mask */
                cc3200_uart_reg RESERVED2 : 1;  /**< Reserved bits */
                cc3200_uart_reg NINEBITM : 1;   /**< 9-bit mode interrupt mask */
                cc3200_uart_reg LMSBIM : 1;     /**< LIN mode sync break interrupt mask */
                cc3200_uart_reg LME1IM : 1;     /**< LIN mode edge 1 interrupt mask */
                cc3200_uart_reg LME5IM : 1;     /**< LIN mode edge 5 interrupt mask */
                cc3200_uart_reg RESERVED1 : 16; /**< Reserved bits */
            } bits;
        } IM;

        cc3200_uart_reg RIS; /**< UART Raw Interrupt Status */

        /**
     * @brief Masked interrupt status register
     */
        union {
            cc3200_uart_reg raw; /**< UART Masked Interrupt Status */
            struct
            {
                cc3200_uart_reg RESERVED8 : 4;   /**< Reserved bits */
                cc3200_uart_reg RXMIS : 1;       /**< UART receive masked interrupt status */
                cc3200_uart_reg TXMIS : 1;       /**< UART transmit masked interrupt status */
                cc3200_uart_reg RTMIS : 1;       /**< UART receive time-out masked interrupt status */
                cc3200_uart_reg FEMIS : 1;       /**< UART framing error masked interrupt status */
                cc3200_uart_reg PEMIS : 1;       /**< UART parity error masked interrupt status */
                cc3200_uart_reg BEMIS : 1;       /**< UART break error masked interrupt status */
                cc3200_uart_reg OEMIS : 1;       /**< UART overrun error masked interrupt status */
                cc3200_uart_reg RESERVED9 : 1;   /**< Reserved bits */
                cc3200_uart_reg NINEBITMIS : 1;  /**< 9-bit mode masked interrupt status */
                cc3200_uart_reg LMSBMIS : 1;     /**< LIN mode sync break masked interrupt status */
                cc3200_uart_reg LME1MIS : 1;     /**< LIN mode edge 1 masked interrupt status */
                cc3200_uart_reg LME5MIS : 1;     /**< LIN mode edge 5 masked interrupt status */
                cc3200_uart_reg RESERVED10 : 16; /**< Reserved bits */
            } bits;
        } MIS;

        cc3200_uart_reg ICR;            /**< UART Interrupt Clear Register */
        cc3200_uart_reg DMACTL;         /**< UART DMA Control */
        cc3200_uart_reg RESERVED3[17];  /**< Reserved addresses */
        cc3200_uart_reg LCTL;           /**< UART LIN Control */
        cc3200_uart_reg LSS;            /**< UART LIN Snap Shot */
        cc3200_uart_reg LTIM;           /**< UART LIN Timer */
        cc3200_uart_reg RESERVED4[2];   /**< Reserved addresses */
        cc3200_uart_reg NINEBITADDR;    /**< UART 9-bit self Address */
        cc3200_uart_reg NINEBITAMASK;   /**< UART 9-bit self Address Mask */
        cc3200_uart_reg RESERVED5[965]; /**< Reserved addresses */
        cc3200_uart_reg PP;             /**< UART Peripheral Properties */
        cc3200_uart_reg RESERVED6;      /**< Reserved addresses */
        cc3200_uart_reg CC;             /**< UART Clock Configuration */
        cc3200_uart_reg RESERVED7[13];  /**< Reserved addresses */
    } cc3200_uart_t;

#define UART0 (cc3200_uart_t *)(0x4000C000) /**< UART0 Instance */
#define UART1 (cc3200_uart_t *)(0x4000D000) /**< UART1 Instance */

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* CC3200_UART_H */
       /** @} */