#include "vectors_cortexm.h"

/* define a local dummy handler as it needs to be in the same compilation unit
 * as the alias definition */
void dummy_handler(void)
{
    dummy_handler_default();
}

/* CC3200 specific interrupt vectors */
WEAK_DEFAULT void isr_gpio_porta0(void);
WEAK_DEFAULT void isr_gpio_porta1(void);
WEAK_DEFAULT void isr_gpio_porta2(void);
WEAK_DEFAULT void isr_gpio_porta3(void);
WEAK_DEFAULT void isr_uart0(void);
WEAK_DEFAULT void isr_uart1(void);
WEAK_DEFAULT void isr_i2c0(void);
WEAK_DEFAULT void isr_adc0_seq0(void);
WEAK_DEFAULT void isr_adc0_seq1(void);
WEAK_DEFAULT void isr_adc0_seq2(void);
WEAK_DEFAULT void isr_adc0_seq3(void);
WEAK_DEFAULT void isr_wdt(void);
WEAK_DEFAULT void isr_timer0a(void);
WEAK_DEFAULT void isr_timer0b(void);
WEAK_DEFAULT void isr_timer1a(void);
WEAK_DEFAULT void isr_timer1b(void);
WEAK_DEFAULT void isr_timer2a(void);
WEAK_DEFAULT void isr_timer2b(void);
WEAK_DEFAULT void isr_flashctl(void);
WEAK_DEFAULT void isr_timer3a(void);
WEAK_DEFAULT void isr_timer3b(void);
WEAK_DEFAULT void isr_udma_sw(void);
WEAK_DEFAULT void isr_udma_error(void);
WEAK_DEFAULT void isr_sha(void);
WEAK_DEFAULT void isr_aes(void);
WEAK_DEFAULT void isr_des(void);
WEAK_DEFAULT void isr_sdhost(void);
WEAK_DEFAULT void isr_i2s(void);
WEAK_DEFAULT void isr_camera(void);
WEAK_DEFAULT void isr_shared_spi(void);
WEAK_DEFAULT void isr_generic_spi(void);
WEAK_DEFAULT void isr_link_spi(void);
WEAK_DEFAULT void isr_nwp(void);
WEAK_DEFAULT void isr_prcm(void);

//*****************************************************************************
//
// The vector table.
//
//*****************************************************************************
ISR_VECTOR(1)
const isr_t vector_cpu[CPU_IRQ_NUMOF] = {
    [0] = isr_gpio_porta0, // GPIO Port A0
    [1] = isr_gpio_porta1, // GPIO Port A1
    [2] = isr_gpio_porta2, // GPIO Port A2
    [3] = isr_gpio_porta3, // GPIO Port A3
    // [20] =  Reserved
    [5] = isr_uart0, // UART0 Rx and Tx
    [6] = isr_uart1, // UART1 Rx and Tx
    // [23] =  Reserved
    [8] = isr_i2c0, // I2C0 Master and Slave
    // [25 - 29] = Reserved,
    [14] = isr_adc0_seq0, // ADC Channel 0
    [15] = isr_adc0_seq1, // ADC Channel 1
    [16] = isr_adc0_seq2, // ADC Channel 2
    [17] = isr_adc0_seq3, // ADC Channel 3
    [18] = isr_wdt,       // Watchdog Timer
    [19] = isr_timer0a,   // Timer 0 subtimer A  35*4 = 0x8C
    [20] = isr_timer0b,   // Timer 0 subtimer B
    [21] = isr_timer1a,   // Timer 1 subtimer A
    [22] = isr_timer1b,   // Timer 1 subtimer B
    [23] = isr_timer2a,   // Timer 2 subtimer A
    [24] = isr_timer2b,   // Timer 2 subtimer B
    // Reserved
    [29] = isr_flashctl, // Flash
    // Reserved
    [35] = isr_timer3a, // Timer 3 subtimer A
    [36] = isr_timer3b, // Timer 3 subtimer B
    // Reserved
    [46] = isr_udma_sw,    // uDMA Software Transfer
    [47] = isr_udma_error, // uDMA Error
    // Reserved
    [148] = isr_sha, // SHA
    // Reserved
    [151] = isr_aes, // AES
    // Reserved
    [153] = isr_des,    // DES
                        // Reserved
    [159] = isr_sdhost, // SDHost
    // Reserved
    [161] = isr_i2s, // I2S
    // Reserved
    [163] = isr_camera, // Camera
    // Reserved
    // [184] = RAM WR Error (no handler yet)
    // Reserved
    [171] = isr_nwp,  // NWP to APPS Interrupt
    [172] = isr_prcm, // Power, Reset and Clock module
    // Reserved
    [175] = isr_shared_spi,  // Shared SPI
    [176] = isr_generic_spi, // Generic SPI
    [177] = isr_link_spi,    // Link SPI
};