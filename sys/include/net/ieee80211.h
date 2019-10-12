
/**
 * @defgroup    net_ieee80211 IEEE802.11
 * @ingroup     net
 * @brief       IEEE802.11 header definitions and utility functions
 * @{
 *
 * @file
 * @brief       IEEE 802.11 header definitions
 *
 */

#ifndef NET_IEEE80211_H
#define NET_IEEE80211_H

#include <stdint.h>
#include <stdlib.h>

#include "byteorder.h"
#include "net/eui64.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Default start frame delimiter
 */
#define IEEE802154_SFD (0xa7)

/**
 * @brief IEEE 802.11 address lengths
 * @{
 */
#define IEEE80211_ADDRESS_LEN (6U) /**< default (48-bit) address */
/**
 * @}
 */

/**
 * @brief IEEE802.11 FCF field definitions
 * @{
 */
/* 802.11n max header size 802.11(a/b/g) requires only 32 */
#define IEEE80211_MAX_HDR_LEN (36U)
#define IEEE80211_MIN_FRAME_LEN (IEEE80211_MAX_HDR_LEN)

#define IEEE802154_FCF_LEN (2U)
#define IEEE802154_FCS_LEN (2U)

#define IEEE802154_FCF_TYPE_MASK (0x07)
#define IEEE802154_FCF_TYPE_BEACON (0x00)
#define IEEE802154_FCF_TYPE_DATA (0x01)
#define IEEE802154_FCF_TYPE_ACK (0x02)
#define IEEE802154_FCF_TYPE_MACCMD (0x03)

#define IEEE802154_FCF_SECURITY_EN (0x08) /**< enable security */
#define IEEE802154_FCF_FRAME_PEND (0x10)  /**< follow-up frame is pending */
#define IEEE802154_FCF_ACK_REQ \
    (0x20) /**< acknowledgement requested from receiver */
#define IEEE802154_FCF_PAN_COMP (0x40) /**< compress source PAN ID */

#define IEEE802154_FCF_DST_ADDR_MASK (0x0c)
#define IEEE802154_FCF_DST_ADDR_VOID (0x00) /**< no destination address */
#define IEEE802154_FCF_DST_ADDR_RESV (0x04) /**< reserved address mode */
#define IEEE802154_FCF_DST_ADDR_SHORT \
    (0x08) /**< destination address length is 2 */
#define IEEE802154_FCF_DST_ADDR_LONG \
    (0x0c) /**< destination address length is 8 */

#define IEEE802154_FCF_VERS_MASK (0x30)
#define IEEE802154_FCF_VERS_V0 (0x00)
#define IEEE802154_FCF_VERS_V1 (0x10)

#define IEEE802154_FCF_SRC_ADDR_MASK (0xc0)
#define IEEE802154_FCF_SRC_ADDR_VOID (0x00)  /**< no source address */
#define IEEE802154_FCF_SRC_ADDR_RESV (0x40)  /**< reserved address mode */
#define IEEE802154_FCF_SRC_ADDR_SHORT (0x80) /**< source address length is 2 \
                                              */
#define IEEE802154_FCF_SRC_ADDR_LONG (0xc0)  /**< source address length is 8 */
/** @} */

/**
 * @brief   Channel ranges
 * @{
 */
#define IEEE802154_CHANNEL_MIN_SUBGHZ \
    (0U) /**< Minimum channel for sub-GHz band */
#define IEEE802154_CHANNEL_MAX_SUBGHZ \
    (10U)                            /**< Maximum channel for sub-GHz band */
#define IEEE802154_CHANNEL_MIN (11U) /**< Minimum channel for 2.4 GHz band */
#define IEEE802154_CHANNEL_MAX (26U) /**< Maximum channel for 2.4 GHz band */
/** @} */

/* 802.11g maximum (MSDU) frame size without support for MPDU/A-MPDU */
#define IEEE80211_FRAME_LEN_MAX (2304U) /**< maximum frame length */

/**
 * @brief   Special address defintions
 * @{
 */
/**
 * @brief   Static initializer for broadcast address
 */
#define IEEE80211_ADDR_BCAST               \
    {                                      \
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff \
    }

/**
 * @brief   Length in byte of @ref IEEE80211_ADDR_BCAST_LEN
 */
#define IEEE80211_ADDR_BCAST_LEN (IEEE80211_ADDRESS_LEN)

/**
 * @brief   Broadcast address
 */
extern const uint8_t ieee80211_addr_bcast[IEEE80211_ADDRESS_LEN];
/** @} */

/**
 * @defgroup net_ieee802154_conf    IEEE802.15.4 compile configurations
 * @ingroup  config
 * @{
 */
/**
 * @brief IEEE802.15.4 default sub-GHZ channel
 */
#ifndef IEEE802154_DEFAULT_SUBGHZ_CHANNEL
#define IEEE802154_DEFAULT_SUBGHZ_CHANNEL (5U)
#endif

/**
 * @brief IEEE802.11 default channel
 */
#ifndef IEEE80211_DEFAULT_CHANNEL
#define IEEE80211_DEFAULT_CHANNEL (7U)
#endif

/**
 * @brief IEEE802.15.4 default sub-GHZ page
 */
#ifndef IEEE802154_DEFAULT_SUBGHZ_PAGE
#define IEEE802154_DEFAULT_SUBGHZ_PAGE (2U)
#endif

/**
 * @brief IEEE802.15.4 default PANID
 */
#ifndef IEEE802154_DEFAULT_PANID
#define IEEE802154_DEFAULT_PANID (0x0023U)
#endif

/**
 * @brief IEEE802.15.4 Broadcast PANID
 */
#ifndef IEEE802154_PANID_BCAST
#define IEEE802154_PANID_BCAST \
    {                          \
        0xff, 0xff             \
    }
#endif

/**
 * @brief IEEE802.15.4 default TX power (in dBm)
 */
#ifndef IEEE802154_DEFAULT_TXPOWER
#define IEEE802154_DEFAULT_TXPOWER (0)
#endif
/** @} */

/**
 * @brief   Initializes an IEEE 802.15.4 MAC frame header in @p buf.
 *
 * @pre Resulting header must fit in memory allocated at @p buf.
 *
 * @see IEEE Std 802.15.4-2011, 5.2.1 General MAC frame format.
 *
 * If @p dst is NULL the IEEE802154_FCF_ACK_REQ will be unset to prevent
 * flooding the network.
 *
 * @param[out] buf      Target memory for frame header.
 * @param[in] src       Source address for frame in network byteorder.
 *                      May be NULL if @ref IEEE802154_FCF_SRC_ADDR_VOID is set
 *                      in @p flags.
 * @param[in] src_len   Length of @p src. Legal values are:
 *                      * 0 (will set @ref IEEE802154_FCF_SRC_ADDR_VOID in MHR)
 *                      * 2 (will set @ref IEEE802154_FCF_SRC_ADDR_SHORT in MHR)
 *                      * 8 (will set @ref IEEE802154_FCF_SRC_ADDR_LONG in MHR)
 * @param[in] dst       Destination address for frame in network byteorder.
 *                      May be NULL if @ref IEEE802154_FCF_SRC_ADDR_VOID is set
 *                      in @p flags.
 * @param[in] dst_len   Length of @p dst. Legal values are:
 *                      * 0 (will set @ref IEEE802154_FCF_DST_ADDR_VOID in MHR)
 *                      * 2 (will set @ref IEEE802154_FCF_DST_ADDR_SHORT in MHR)
 *                      * 8 (will set @ref IEEE802154_FCF_DST_ADDR_LONG in MHR)
 * @param[in] src_pan   Source PAN ID in little-endian. May be 0 if
 *                      @ref IEEE802154_FCF_PAN_COMP is set in @p flags.
 *                      Otherwise, it will be ignored, when
 *                      @ref IEEE802154_FCF_PAN_COMP is set.
 * @param[in] dst_pan   Destination PAN ID in little-endian.
 * @param[in] flags     Flags for the frame. These are interchangable with the
 *                      first byte of the IEEE 802.15.4 FCF. This means that
 *                      it encompasses the type values,
 *                      @ref IEEE802154_FCF_SECURITY_EN,
 *                      @ref IEEE802154_FCF_FRAME_PEND, and
 *                      @ref IEEE802154_FCF_ACK_REQ.
 * @param[in] seq       Sequence number for frame.
 *
 * The version field in the FCF will be set implicitly to version 1.
 *
 * @return  Size of frame header on success.
 * @return  0, on error (flags set to unexpected state).
 */
size_t ieee80211_set_frame_hdr(uint8_t *buf, const uint8_t *src, size_t src_len,
                               const uint8_t *dst, size_t dst_len,
                               uint8_t flags, uint8_t seq);

/**
 * @brief   Get length of MAC header.
 *
 * @todo include security header implications
 *
 * @param[in] mhr   MAC header.
 *
 * @return  Length of MAC header on success.
 * @return  0, on error (source mode or destination mode set to reserved).
 */
size_t ieee80211_get_frame_hdr_len(const uint8_t *mhr);

/**
 * @brief   Gets source address from MAC header.
 *
 * @pre (@p src != NULL) && (@p src_pan != NULL)
 *
 * @param[in] mhr       MAC header.
 * @param[out] src      Source address in network byte order in MAC header.
 *
 * @return   Length of source address.
 * @return  -EINVAL, if @p mhr contains unexpected flags.
 */
int ieee80211_get_src(const uint8_t *mhr, uint8_t *src);

/**
 * @brief   Gets destination address from MAC header.
 *
 * @pre (@p dst != NULL) && (@p dst_pan != NULL)
 *
 * @param[in] mhr       MAC header.
 * @param[out] dst      Destination address in network byte order in MAC header.
 *
 * @return   Length of destination address.
 * @return  -EINVAL, if @p mhr contains unexpected flags.
 */
int ieee80211_get_dst(const uint8_t *mhr, uint8_t *dst);

/**
 * @brief   Gets sequence number from MAC header.
 *
 * @pre length of allocated space at @p mhr > 3
 *
 * @param[in] mhr   MAC header.
 *
 * @return  The sequence number in @p mhr.
 */
static inline uint8_t ieee80211_get_seq(const uint8_t *mhr)
{
    return mhr[2];
}

/**
 * @brief generate EUI64 based address from a mac address
 *
 * @param eui64
 * @param addr
 * @param addr_len
 * @return eui64_t*
 */
static inline eui64_t *ieee80211_get_iid(eui64_t *eui64, const uint8_t *addr,
                                         size_t addr_len)
{
    if (addr_len != 6) {
        return NULL;
    }
    int i = 0;

    eui64->uint8[0] = eui64->uint8[1] = 0;
    /* invert universal/local bit as of RFC4291 section 2.5.1 */
    eui64->uint8[0] = addr[i++] ^ 0x02;
    eui64->uint8[1] = addr[i++];
    eui64->uint8[2] = addr[i++];
    eui64->uint8[3] = 0xff;
    eui64->uint8[4] = 0xfe;
    eui64->uint8[5] = addr[i++];
    eui64->uint8[6] = addr[i++];
    eui64->uint8[7] = addr[i++];

    return eui64;
}

#ifdef __cplusplus
}
#endif

#endif /* NET_IEEE80211_H */
/** @} */
