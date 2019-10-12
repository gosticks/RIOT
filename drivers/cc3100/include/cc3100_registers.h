
#ifndef CC3100_REGISTERS_H
#define CC3100_REGISTERS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name    Internal device option flags
 * @{
 */
#define CC3100_OPT_AUTOACK          (0x0001)    /**< auto ACKs active */
#define CC3100_OPT_CSMA             (0x0002)    /**< CSMA active */
#define CC3100_OPT_PROMISCUOUS      (0x0004)    /**< promiscuous mode
                                                 *   active */
#define CC3100_OPT_PRELOADING       (0x0008)    /**< preloading enabled */
#define CC3100_OPT_TELL_TX_START    (0x0010)    /**< notify MAC layer on TX
                                                 *   start */
#define CC3100_OPT_TELL_TX_END      (0x0020)    /**< notify MAC layer on TX
                                                 *   finished */
#define CC3100_OPT_TELL_RX_START    (0x0040)    /**< notify MAC layer on RX
                                                 *   start */
#define CC3100_OPT_TELL_RX_END      (0x0080)    /**< notify MAC layer on RX
                                                 *   finished */
/** @} */


#ifdef __cplusplus
}
#endif

#endif /* CC2420_REGISTERS_H */
/** @} */
