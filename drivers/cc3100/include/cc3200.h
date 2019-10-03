#include <stdio.h>

#include "cc3200_params.h"

typedef struct {
    /* device specific fields */
    cc3200_params_t params; /**< hardware interface configuration */
    /* device state fields */
    uint8_t state;    /**< current state of the radio */
    uint16_t options; /**< state of used options */
} cc3200_t;