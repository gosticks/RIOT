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

void _print_frame_type(uint16_t fc)
{
    uint8_t type = ((uint8_t)(fc & IEEE80211_FCF_TYPE_MASK) << 2);
    DEBUG("TYPE %d \n", type);
    uint8_t subtype = ((uint8_t)(fc & IEEE80211_FCF_TYPE_MASK) << 4);
    char *name      = "";
    if (type == IEEE80211_FCF_TYPE_CTRL) {
        if (subtype == IEEE80211_FCF_CTRL_SUBTYPE_WL_ACK) {
            name = "ACK";
        }
    } else if (type == IEEE80211_FCF_TYPE_DATA) {
        name = "DATA";
    } else if (type == IEEE80211_FCF_TYPE_MNG) {
        switch (subtype) {
        case IEEE80211_FCF_MNG_SUBTYPE_DEAUTH:
            name = "DEAUTH";
            break;
        case IEEE80211_FCF_MNG_SUBTYPE_WL_BEACON:
            name = "BEACON";
            break;
        case IEEE80211_FCF_MNG_SUBTYPE_AS_REQ:
            name = "ASSOCIATION_REQ";
            break;

        default:
            name = "UNKNOWN";
            break;
        }
    }
    DEBUG("Frame: %s type=%d subtype=%d \n", name, type, subtype);
}

size_t ieee80211_set_frame_hdr(uint8_t *buf, const uint8_t *src,
                               const uint8_t *dst, const uint8_t *bssid,
                               uint16_t fc, uint16_t seq)
{
    DEBUG("ieee80211_set_frame_hdr\n");
    int pos         = 4; /* 0-1: FC */
    uint8_t type    = ((uint8_t)(fc & IEEE80211_FCF_TYPE_MASK) << 2);
    uint8_t subtype = ((uint8_t)(fc & IEEE80211_FCF_TYPE_MASK) << 4);

    /* set FC */
    (*(uint16_t *)&buf[1]) = fc;

    /* Duration/ID */

    /* set duration field here */
    // TODO: set a value
    // _print_frame_type(fc);

    /* if this is an ack we are done here */
    if (type == IEEE80211_FCF_TYPE_CTRL &&
        subtype == IEEE80211_FCF_CTRL_SUBTYPE_WL_ACK) {
        DEBUG("ACK frame \n");
        buf[pos++] = dst[0];
        buf[pos++] = dst[1];
        buf[pos++] = dst[2];
        buf[pos++] = dst[3];
        buf[pos++] = dst[4];
        buf[pos++] = dst[5];
        return pos;
    }

    /* Set header addresses based on the to and from DS value
    +-----+-------+-----------+------------+-----------+-----------+
    | To  | From  | Address 1 |  Address 2 | Address 3 | Address 4 |
    |  DS |   DS  |           |            |           |           |
    +-----+-------+-----------+------------+-----------+-----------+
    |  0  |   0   |  RA = DA  |   TA = SA  |   BSSID   |     -     |
    +-----+-------+-----------+------------+-----------+-----------+
    |  0  |   1   |  RA = DA  | TA = BSSID |     SA    |     -     |
    +-----+-------+-----------+------------+-----------+-----------+
    |  1  |   0   |   BSSID   |   TA = SA  |     DA    |     -     |
    +-----+-------+-----------+------------+-----------+-----------+
    |  1  |   1   |     RA    |     TA     |     DA    |     SA    |
    +-----+-------+-----------+------------+-----------+-----------+
    */

    /* set Address 1 field */
    /* FROM_DS = 0 && TO_DS = *  */
    if (0 == (fc & IEEE80211_FCF_FRAME_TO_DS)) {
        /* set destination */
        buf[pos++] = dst[0];
        buf[pos++] = dst[1];
        buf[pos++] = dst[2];
        buf[pos++] = dst[3];
        buf[pos++] = dst[4];
        buf[pos++] = dst[5];

        /* TO_DS = 1 */
        if (fc & IEEE80211_FCF_FRAME_FROM_DS) {
            /* address 2 */
            buf[pos++] = src[0];
            buf[pos++] = src[1];
            buf[pos++] = src[2];
            buf[pos++] = src[3];
            buf[pos++] = src[4];
            buf[pos++] = src[5];
            /* address 3 */
            buf[pos++] = bssid[0];
            buf[pos++] = bssid[1];
            buf[pos++] = bssid[2];
            buf[pos++] = bssid[3];
            buf[pos++] = bssid[4];
            buf[pos++] = bssid[5];
        } else {
            /* address 2 */
            buf[pos++] = bssid[0];
            buf[pos++] = bssid[1];
            buf[pos++] = bssid[2];
            buf[pos++] = bssid[3];
            buf[pos++] = bssid[4];
            buf[pos++] = bssid[5];

            /* address 3 */
            buf[pos++] = src[0];
            buf[pos++] = src[1];
            buf[pos++] = src[2];
            buf[pos++] = src[3];
            buf[pos++] = src[4];
            buf[pos++] = src[5];
        }

        /* FROM_DS = 1 && TO_DS = 0  */
    } else if ((fc & IEEE80211_FCF_FRAME_TO_DS) &&
               ((fc & IEEE80211_FCF_FRAME_FROM_DS) == 0)) {
        /* set bssid as address 1 */
        buf[pos++] = bssid[0];
        buf[pos++] = bssid[1];
        buf[pos++] = bssid[2];
        buf[pos++] = bssid[3];
        buf[pos++] = bssid[4];
        buf[pos++] = bssid[5];

        /* address 2 */
        buf[pos++] = src[0];
        buf[pos++] = src[1];
        buf[pos++] = src[2];
        buf[pos++] = src[3];
        buf[pos++] = src[4];
        buf[pos++] = src[5];

        /* FROM_DS = 1 && TO_DS = 1 */
    } else {
        /* only used for inter network communication by APs */
        DEBUG("FROM_DS = 1 && TO_DS = 1 case not handled\n");
    }

    /* Sequence control */
    (*(uint16_t *)&buf[pos + 1]) = seq;
    pos += 2;

    /* Address 3 */
    if ((fc & IEEE80211_FCF_FRAME_TO_DS) &&
        ((fc & IEEE80211_FCF_FRAME_FROM_DS) == 0)) {
        buf[pos++] = dst[0];
        buf[pos++] = dst[1];
        buf[pos++] = dst[2];
        buf[pos++] = dst[3];
        buf[pos++] = dst[4];
        buf[pos++] = dst[5];
    }

    /* return actual header length */
    return pos;
}

size_t ieee80211_get_frame_hdr_len(const uint8_t *mhr)
{
    DEBUG("ieee80211_get_frame_hdr_len\n");

    int offset      = 3; /* FCF: 0-3 */
    uint16_t fc     = ((uint16_t)mhr[0] << 8) | mhr[1];
    uint8_t type    = ((uint8_t)(fc & IEEE80211_FCF_TYPE_MASK) << 2);
    uint8_t subtype = ((uint8_t)(fc & IEEE80211_FCF_TYPE_MASK) << 4);
    uint8_t tmp;

    if (type == IEEE80211_FCF_TYPE_CTRL &&
        subtype == IEEE80211_FCF_CTRL_SUBTYPE_WL_ACK) {
        /* FC + ID + Address 1 */
        return 10;
    }

    if ((fc & IEEE80211_FCF_FRAME_TO_DS) &&
        (fc & IEEE80211_FCF_FRAME_FROM_DS)) {
        /* FC + Duration/ID + addr 1 + addr 2 + addr 3 + SEQ + addr 3 */
        /* TODO: 802.11n requires 4 more bytes */
        return 32;
    } else {
        return 22;
    }
}

int ieee80211_get_src(const uint8_t *mhr, uint8_t *src)
{
    int offset      = 4; /* FCF: 0-3 */
    uint16_t fc     = ((uint16_t)mhr[0] << 8) | mhr[1];
    uint8_t type    = ((uint8_t)(fc & IEEE80211_FCF_TYPE_MASK) << 2);
    uint8_t subtype = ((uint8_t)(fc & IEEE80211_FCF_TYPE_MASK) << 4);
    DEBUG("FC %x \n", fc);

    // _print_frame_type(fc);

    if ((fc & IEEE80211_FCF_FRAME_FROM_DS) == 0) {
        offset = 10;
    } else if ((fc & IEEE80211_FCF_FRAME_TO_DS) == 0) {
        offset = 16;
    } else {
        DEBUG("FRAME TYPE NOT COVERED");
        return 0;
    }

    /* read address */
    src[0] = mhr[offset++];
    src[1] = mhr[offset++];
    src[2] = mhr[offset++];
    src[3] = mhr[offset++];
    src[4] = mhr[offset++];
    src[5] = mhr[offset++];

    return IEEE80211_ADDRESS_LEN;
}

int ieee80211_get_dst(const uint8_t *mhr, uint8_t *dst)
{
    int offset      = 4; /* FCF: 0-3 */
    uint16_t fc     = ((uint16_t)mhr[0] << 8) | mhr[1];
    uint8_t type    = ((uint8_t)(fc & IEEE80211_FCF_TYPE_MASK) << 2);
    uint8_t subtype = ((uint8_t)(fc & IEEE80211_FCF_TYPE_MASK) << 4);

    if (fc & IEEE80211_FCF_FRAME_TO_DS) {
        offset = 10;
    }

    /* read address */
    dst[0] = mhr[offset++];
    dst[1] = mhr[offset++];
    dst[2] = mhr[offset++];
    dst[3] = mhr[offset++];
    dst[4] = mhr[offset++];
    dst[5] = mhr[offset++];

    return IEEE80211_ADDRESS_LEN;
}

/** @} */
