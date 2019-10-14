
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
#define IEEE80211_SFD (0xa7)

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

#define IEEE80211_FCF_LEN (2U)
#define IEEE80211_FCS_LEN (2U)

/** IEEE802.11 frame types */
#define IEEE80211_FCF_TYPE_MNG (0U)
#define IEEE80211_FCF_TYPE_CTRL (1U)
#define IEEE80211_FCF_TYPE_DATA (2U)
#define IEEE80211_FC_TYPE_OFFSET (12U)

/* FC flags */
#define IEEE80211_FCF_FRAME_MORE_DATA (0x4)
#define IEEE80211_FCF_FRAME_TO_DS (0x40)
#define IEEE80211_FCF_FRAME_FROM_DS (0x80)

#define IEEE80211_FCF_TYPE_MASK (0x3000)
#define IEEE80211_FCF_SUBTYPE_MASK (0xF00)

#define IEEE80211_FCF_VERS_MASK (0xC000)
#define IEEE80211_FCF_VERS_V0 (0x00)

#define IEEE80211_FCF_SUBTYPE_FC_VAL (subtype) subtype << 8

/** IEEE802.11 frame subtypes */
typedef enum {
    IEEE80211_FCF_MNG_SUBTYPE_AS_REQ = 0,    /**< Association Request */
    IEEE80211_FCF_MNG_SUBTYPE_AS_RESP,       /**< Association Response */
    IEEE80211_FCF_MNG_SUBTYPE_RAS_REQ,       /**< Reassociation Request */
    IEEE80211_FCF_MNG_SUBTYPE_RAS_RESP,      /**< Reassociation Response */
    IEEE80211_FCF_MNG_SUBTYPE_PROBE_REQ,     /**< Probe Response */
    IEEE80211_FCF_MNG_SUBTYPE_PROBE_RESP,    /**< Probe Response */
    IEEE80211_FCF_MNG_SUBTYPE_TIMING_ADD,    /**< Timing Advertisement */
    IEEE80211_FCF_MNG_SUBTYPE_RESERVED_1,    /**< Reserved */
    IEEE80211_FCF_MNG_SUBTYPE_WL_BEACON,     /**< Beacon */
    IEEE80211_FCF_MNG_SUBTYPE_ATIM,          /**< ATIM */
    IEEE80211_FCF_MNG_SUBTYPE_DIAS,          /**< disassociation */
    IEEE80211_FCF_MNG_SUBTYPE_AUTH,          /**< Authentication */
    IEEE80211_FCF_MNG_SUBTYPE_DEAUTH,        /**< Deauthentication */
    IEEE80211_FCF_MNG_SUBTYPE_ACTION,        /**< Action */
    IEEE80211_FCF_MNG_SUBTYPE_ACTION_NO_ACK, /**< Action No Ack */
    IEEE80211_FCF_MNG_SUBTYPE_RESERVED_2,    /**< reserved 2 */
} ieee80211_mng_subtype_t;

typedef enum {
    IEEE80211_FCF_CTRL_SUBTYPE_BEAM_RP = 4,   /**< Beamform Report Poll */
    IEEE80211_FCF_CTRL_SUBTYPE_VHT_NDP_AN,    /**< VHT NDP Announcement */
    IEEE80211_FCF_CTRL_SUBTYPE_CTRL_FR_EXT,   /**< Control Frame Extension */
    IEEE80211_FCF_CTRL_SUBTYPE_CTRL_WRAPPER,  /**< Control Wrapper */
    IEEE80211_FCF_CTRL_SUBTYPE_BLOCK_ACK_REQ, /**< Block ACK request */
    IEEE80211_FCF_CTRL_SUBTYPE_BLOCK_ACK,     /**< Block ACK */
    IEEE80211_FCF_CTRL_SUBTYPE_PS_POLL,       /**< PS-Poll */
    IEEE80211_FCF_CTRL_SUBTYPE_RTS,           /**< RTS */
    IEEE80211_FCF_CTRL_SUBTYPE_CTS,           /**< CTS */
    IEEE80211_FCF_CTRL_SUBTYPE_WL_ACK,        /**< ACK */
    IEEE80211_FCF_CTRL_SUBTYPE_CF_END,        /**< CF End */
    IEEE80211_FCF_CTRL_SUBTYPE_CF_END_ACK,    /**< CF End +CF Ack */
} ieee80211_ctrl_subtype_t;

/* Frame control field bitfield */
typedef struct {
    uint8_t version : 2;            // protocol version
    uint8_t type : 2; // type
    uint8_t subtype : 4;            // subtype
    uint8_t to_ds : 1;                 // to destination flag
    uint8_t from_ds : 1;               // from destination flag
    uint8_t more_framents : 1;         // more frames flag
    uint8_t retry : 1;                 // retry flag
    uint8_t pm : 1;                    // power management
    uint8_t more_data : 1;             // more_data
    uint8_t protected_frame : 1;       // protected frame
    uint8_t htc_order : 1; // +HTC / Order (set to 1 in a non-QoS Data Frame)
} ieee80211_frame_control_t;

#define IEEE80211_FCF_DST_ADDR_MASK (0x0c)
#define IEEE80211_FCF_DST_ADDR_VOID (0x00) /**< no destination address */


#define IEEE80211_FCF_SRC_ADDR_MASK (0xc0)
#define IEEE80211_FCF_SRC_ADDR_VOID (0x00) /**< no source address */
#define IEEE80211_FCF_SRC_ADDR_RESV (0x40) /**< reserved address mode */
/** @} */

/**
 * @brief   Channel ranges
 * @{
 */
#define IEEE80211_CHANNEL_MIN_SUBGHZ \
    (0U) /**< Minimum channel for sub-GHz band */
#define IEEE80211_CHANNEL_MAX_SUBGHZ \
    (10U)                            /**< Maximum channel for sub-GHz band */
#define IEEE80211_CHANNEL_MIN (1U) /**< Minimum channel for 2.4 GHz band */
#define IEEE80211_CHANNEL_MAX (12U) /**< Maximum channel for 2.4 GHz band */
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
 * @defgroup net_ieee80211_conf    IEEE802.11 compile configurations
 * @ingroup  config
 * @{
 */

/**
 * @brief IEEE802.11 default channel
 */
#ifndef IEEE80211_DEFAULT_CHANNEL
#define IEEE80211_DEFAULT_CHANNEL (7U)
#endif


/**
 * @brief IEEE802.11 default TX power (in dBm)
 */
#ifndef IEEE80211_DEFAULT_TXPOWER
#define IEEE80211_DEFAULT_TXPOWER (0)
#endif
/** @} */

/**
 * @brief   Initializes an IEEE 802.11 MAC frame header in @p buf.
 *
 *
 * @param[out] buf      Target memory for frame header.
 * @param[in] src       Source address for frame in network byteorder.s.
 * @param[in] dst       Destination address for frame in network byteorder.
 * @param[in] bssid     Currently associated BSSID.
 * @param[in] flags     Frame Control field flags
 * @param[in] seq       Sequence number for frame.
 *
 * The version field in the FCF will be set implicitly to version 1.
 *
 * @return  Size of frame header on success.
 * @return  0, on error (flags set to unexpected state).
 */
size_t ieee80211_set_frame_hdr(uint8_t *buf, const uint8_t *src, 
                               const uint8_t *dst, const uint8_t *bssid,
                               uint16_t fc, uint8_t seq);

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
