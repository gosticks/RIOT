#include "proto.h"
#include <stdio.h>
/**
 * @brief return the current rom info
 *
 * @return CC3200_RomInfo*
 */
CC3200_RomInfo *getDeviceRomInfo(void)
{
    return (CC3200_RomInfo *)(0x00000400);
}

/**
 * @brief alignDataLen returns the next supported transport length for a
 * provided length
 *
 */
// static inline uint16_t alignDataLen(uint16_t len) {
//     return ();
// }

/**
 * @brief removes the first element in the buffer of length len
 *
 */
void sliceFirstInBuffer(uint8_t *buf, int len)
{
    for (uint8_t i = 0; i < (len - 1); i++) {
        buf[i] = buf[i + 1];
    }
    buf[len - 1] = 0;
}

void printChars(char *str, uint16_t len)
{
    for (int i = 0; i < len; i++) {
        printf("%c", str[i]);
    }
}

void maskWifiInterrupt(void)
{
    (*(unsigned long *)N2A_INT_MASK_SET) = 0x1;
}
void unmaskWifiInterrupt(void)
{
    (*(unsigned long *)N2A_INT_MASK_CLR) = 0x1;
}