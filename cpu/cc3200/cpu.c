#include "cpu.h"
#include "vendor/hw_ints.h"
#include "periph_cpu.h"
#include "periph/init.h"

void cpu_init(void)
{
    /* initializes the Cortex-M core */
    cortexm_init();

    /* init board */
    PRCMCC3200MCUInit();

    /* 1 priority group */
    ROM_IntPriorityGroupingSet(0);
    /* trigger static peripheral initialization */
    periph_init();
}

// void periph_clk_enable(unsigned long ulPeripheral, unsigned long ulClkFlags)
// {
//   //
//   // Enable the specified peripheral clocks, Nothing to be done for PRCM_ADC
//   // as it is a dummy define for pinmux utility code generation
//   //
//   if(ulPeripheral != PRCM_ADC)
//   {
//     HWREG(ARCM_BASE + PRCM_PeriphRegsList[ulPeripheral].ulClkReg) |= ulClkFlags;
//   }


//   //
//   // Set the default clock for camera
//   //
//   if(ulPeripheral == PRCM_CAMERA)
//   {
//     HWREG(ARCM_BASE + APPS_RCM_O_CAMERA_CLK_GEN) = 0x0404;
//   }
// }