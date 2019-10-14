/*
 * Copyright (C) 2016 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 *
 * @file
 * @author Martine Lenders <mlenders@inf.fu-berlin.de>
 */

#include <assert.h>
#include <errno.h>
#include <string.h>

#include "net/ieee80211.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

const uint8_t ieee80211_addr_bcast[IEEE80211_ADDR_BCAST_LEN] =
        IEEE80211_ADDR_BCAST;

size_t ieee80211_set_frame_hdr(uint8_t *buf, const uint8_t *src,const uint8_t *dst, const uint8_t *bssid, uint16_t fc, uint8_t seq)
{
    DEBUG("ieee80211_set_frame_hdr\n");
    int pos      = 3; /* 0-1: FC */
    uint8_t type = (fc & IEEE80211_FCF_TYPE_MASK) >> 12;
    uint8_t subtype = (fc & IEEE80211_FCF_SUBTYPE_MASK) >> 8;
    
    /* set frame control field to the first 16 bit */
    (*(uint16_t *)&buf[1]) = fc;

    /* set duration field here */
    // TODO: set a value

    /* construct frame */


    /* set destination */
    buf[pos++] = dst[0];
    buf[pos++] = dst[1];
    buf[pos++] = dst[2];
    buf[pos++] = dst[3];
    buf[pos++] = dst[4];
    buf[pos++] = dst[5];

    /* if this is an ack complete */
    if (type == IEEE80211_FCF_TYPE_CTRL && subtype == IEEE80211_FCF_CTRL_SUBTYPE_WL_ACK) {
        return pos;
    }

    /* for other frames set the src field */
    buf[pos++] = src[0];
    buf[pos++] = src[1];
    buf[pos++] = src[2];
    buf[pos++] = src[3];
    buf[pos++] = src[4];
    buf[pos++] = src[5];

    
    /* return actual header length */
    return pos;
}

size_t ieee80211_get_frame_hdr_len(const uint8_t *mhr)
{
    DEBUG("ieee80211_get_frame_hdr_len\n");
    return 0;
    /* TODO: include security header implications */
    // uint8_t tmp;
    // size_t len = 3; /* 2 byte FCF, 1 byte sequence number */

    // /* figure out address sizes */
    // tmp = (mhr[1] & IEEE802154_FCF_DST_ADDR_MASK);
    // if (tmp == IEEE802154_FCF_DST_ADDR_SHORT) {
    //     len += 4; /* 2 byte dst PAN + 2 byte dst short address */
    // } else if (tmp == IEEE802154_FCF_DST_ADDR_LONG) {
    //     len += 10; /* 2 byte dst PAN + 2 byte dst long address */
    // } else if (tmp != IEEE802154_FCF_DST_ADDR_VOID) {
    //     return 0;
    // } else if (mhr[0] & IEEE802154_FCF_PAN_COMP) {
    //     /* PAN compression, but no destination address => illegal state */
    //     return 0;
    // }
    // tmp = (mhr[1] & IEEE802154_FCF_SRC_ADDR_MASK);
    // if (tmp == IEEE802154_FCF_SRC_ADDR_VOID) {
    //     return len;
    // } else {
    //     if (!(mhr[0] & IEEE802154_FCF_PAN_COMP)) {
    //         len += 2;
    //     }
    //     if (tmp == IEEE802154_FCF_SRC_ADDR_SHORT) {
    //         return len + 2;
    //     } else if (tmp == IEEE802154_FCF_SRC_ADDR_LONG) {
    //         return len + 8;
    //     }
    // }
    // return 0;
}

int ieee80211_get_src(const uint8_t *mhr, uint8_t *src)
{
    
    // int offset = 3; /* FCF: 0-1, Seq: 2 */
    // uint8_t tmp;

    // assert((src != NULL) && (src_pan != NULL));
    // tmp = mhr[1] & IEEE802154_FCF_DST_ADDR_MASK;
    // if (tmp == IEEE802154_FCF_DST_ADDR_SHORT) {
    //     if (mhr[0] & IEEE802154_FCF_PAN_COMP) {
    //         src_pan->u8[0] = mhr[offset];
    //         src_pan->u8[1] = mhr[offset + 1];
    //     }
    //     offset += 4;
    // } else if (tmp == IEEE802154_FCF_DST_ADDR_LONG) {
    //     if (mhr[0] & IEEE802154_FCF_PAN_COMP) {
    //         src_pan->u8[0] = mhr[offset];
    //         src_pan->u8[1] = mhr[offset + 1];
    //     }
    //     offset += 10;
    // } else if (tmp != IEEE802154_FCF_DST_ADDR_VOID) {
    //     return -EINVAL;
    // } else if (mhr[0] & IEEE802154_FCF_PAN_COMP) {
    //     /* PAN compression, but no destination address => illegal state */
    //     return -EINVAL;
    // }

    // tmp = mhr[1] & IEEE802154_FCF_SRC_ADDR_MASK;
    // if (tmp != IEEE802154_FCF_SRC_ADDR_VOID) {
    //     if (!(mhr[0] & IEEE802154_FCF_PAN_COMP)) {
    //         src_pan->u8[0] = mhr[offset++];
    //         src_pan->u8[1] = mhr[offset++];
    //     }
    // }
    // if (tmp == IEEE802154_FCF_SRC_ADDR_SHORT) {
    //     /* read src PAN and address in little endian */
    //     src[1] = mhr[offset++];
    //     src[0] = mhr[offset++];
    //     return 2;
    // } else if (tmp == IEEE802154_FCF_SRC_ADDR_LONG) {
    //     /* read src PAN and address in little endian */
    //     for (int i = 7; i >= 0; i--) {
    //         src[i] = mhr[offset++];
    //     }
    //     return 8;
    // } else if (tmp != IEEE802154_FCF_SRC_ADDR_VOID) {
    //     return -EINVAL;
    // }

    return 0;
}

int ieee80211_get_dst(const uint8_t *mhr, uint8_t *dst)
{
    DEBUG("ieee80211_get_dst\n");
    // int offset = 3; /* FCF: 0-1, Seq: 2 */
    // uint8_t tmp;

    // assert((dst != NULL) && (dst_pan != NULL));
    // tmp = mhr[1] & IEEE802154_FCF_DST_ADDR_MASK;
    // if (tmp == IEEE802154_FCF_DST_ADDR_SHORT) {
    //     /* read dst PAN and address in little endian */
    //     dst_pan->u8[0] = mhr[offset++];
    //     dst_pan->u8[1] = mhr[offset++];
    //     dst[1]         = mhr[offset++];
    //     dst[0]         = mhr[offset++];
    //     return 2;
    // } else if (tmp == IEEE802154_FCF_DST_ADDR_LONG) {
    //     dst_pan->u8[0] = mhr[offset++];
    //     dst_pan->u8[1] = mhr[offset++];
    //     for (int i = 7; i >= 0; i--) {
    //         dst[i] = mhr[offset++];
    //     }
    //     return 8;
    // } else if (tmp != IEEE802154_FCF_DST_ADDR_VOID) {
    //     return -EINVAL;
    // } else if (mhr[0] & IEEE802154_FCF_PAN_COMP) {
    //     /* PAN compression, but no destination address => illegal state */
    //     return -EINVAL;
    // }

    return 0;
}

/** @} */
