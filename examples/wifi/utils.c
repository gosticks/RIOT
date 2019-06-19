#include "proto.h"

/**
 * @brief return the current rom info
 *
 * @return CC3200_RomInfo*
 */
CC3200_RomInfo *getDeviceRomInfo(void) {
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
void sliceFirstInBuffer(uint8_t *buf, int len) {
  for (uint8_t i = 0; i < (len - 1); i++) {
    buf[i] = buf[i + 1];
  }
  buf[len - 1] = 0;
}

void maskWifiInterrupt(void) { (*(unsigned long *)0x400F7088) = 0x1; }
void unmaskWifiInterrupt(void) { (*(unsigned long *)0x400F708C) = 0x1; }