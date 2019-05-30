#include "cpu.h"
#include "vendor/hw_ints.h"
#include "periph_cpu.h"
#include "periph/init.h"

/**
 * Interrupt vector base address, defined by the linker
 */
// extern const void *_isr_vectors;

void cpu_init(void)
{
    /* initializes the Cortex-M core */
    cortexm_init();

    /* init board */
    PRCMCC3200MCUInit();

    /* 1 priority group */
    MAP_IntPriorityGroupingSet(0);
    /* trigger static peripheral initialization */
    periph_init();
}
