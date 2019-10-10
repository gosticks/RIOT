/**
 * @ingroup    drivers_cc3100
 * @{
 *
 * @file
 * @brief      Netdev interface for the CC3100
 *
 * @author     Wladislaw Meixner <wladislaw.meixner@campus.lmu.de>
 *
 */

#ifndef CC3200_NETDEV_H
#define CC3200_NETDEV_H

#include "net/netdev.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Reference to the netdev device driver struct
 */
extern const netdev_driver_t netdev_driver_cc3100;

#ifdef __cplusplus
}
#endif

#endif /* CC3200_NETDEV_H */
/** @} */
