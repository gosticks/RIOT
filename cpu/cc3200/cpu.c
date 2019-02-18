#include "cpu.h"
#include "periph_cpu.h"
#include "periph/init.h"

// #define ENABLE_DEBUG (0)
// #include "debug.h"

void cpu_init(void)
{
    /* initializes the Cortex-M core */
    cortexm_init();

    PRCMCC3200MCUInit();

    /* 1 priority group */
    // MAP_IntPriorityGroupingSet(0);

    /* trigger static peripheral initialization */
    periph_init();
}
