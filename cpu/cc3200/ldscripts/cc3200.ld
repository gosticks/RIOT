/**
 * @addtogroup      cpu_cc3200/cpu_cc3101B
 * @{
 *
 * @file
 * @brief           linker script for cpu_cc3200/cpu_cc3101 MCUs
 *
 * @}
 */

 MEMORY
{
    rom (rx)  : ORIGIN = 0x00080000, LENGTH = 512K
    ram (w!rx) : ORIGIN = 0x20040000, LENGTH = 192K
}

/* MCU Specific Section Definitions */
/* SECTIONS
{
    .flashcca :
    {
        KEEP(*(.flashcca))
    } > cca
} */

INCLUDE cortexm_base.ld