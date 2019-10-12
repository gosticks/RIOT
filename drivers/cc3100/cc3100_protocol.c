#include <stdbool.h>
#include <stdio.h>

#include "vendor/hw_adc.h"
#include "vendor/hw_common_reg.h"
#include "vendor/hw_ints.h"
#include "vendor/hw_mcspi.h"
#include "vendor/hw_memmap.h"
#include "vendor/hw_ocp_shared.h"
#include "vendor/hw_types.h"
#include "vendor/hw_udma.h"
#include "vendor/rom.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

#include "periph/spi.h"

#include "cc3100.h"
#include "include/cc3100_internal.h"
#include "include/cc3100_protocol.h"
