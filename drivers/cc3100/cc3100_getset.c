
#include <string.h>
#include <errno.h>

#include "cc3100.h"
#include "cc3100_internal.h"
#include "cc3100_registers.h"
#include "periph/spi.h"

#define ENABLE_DEBUG    (1)
#include "debug.h"

void cc3100_get_addr(cc3100_t *dev, uint8_t *addr)
{
    // request address from the NWP
}

void cc3100_set_channel(cc3100_t *dev) {

}

void cc3100_get_channel(cc3100_t *dev) {

}

void cc3100_set_txpower(cc3100_t *dev) {

}

void cc3100_get_txpower(cc3100_t *dev) {

}

int cc2420_set_option(cc3100_t *dev, uint16_t option, bool state) {
    return 0;
}