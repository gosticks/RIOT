#include "net/gnrc.h"
#include "net/gnrc/netif/ieee80211.h"
#include "net/netdev/ieee80211.h"

#ifdef MODULE_GNRC_IPV6
#include "net/ipv6/hdr.h"
#endif

#define ENABLE_DEBUG (1)
#include "debug.h"

#if defined(MODULE_OD) && ENABLE_DEBUG
#include "od.h"
#endif

static int _send(gnrc_netif_t *netif, gnrc_pktsnip_t *pkt);
static gnrc_pktsnip_t *_recv(gnrc_netif_t *netif);

static const gnrc_netif_ops_t ieee80211_ops = {
    .send = _send,
    .recv = _recv,
    .get  = gnrc_netif_get_from_netdev,
    .set  = gnrc_netif_set_from_netdev,
};

gnrc_netif_t *gnrc_netif_ieee80211_create(char *stack, int stacksize,
                                          char priority, char *name,
                                          netdev_t *dev)
{
    DEBUG("gnrc_netif_ieee80211_create\n");
    return gnrc_netif_create(stack, stacksize, priority, name, dev,
                             &ieee80211_ops);
}

static gnrc_pktsnip_t *_make_netif_hdr(uint8_t *mhr)
{
    DEBUG("_make_netif_hdr\n");
    gnrc_netif_hdr_t *hdr;
    gnrc_pktsnip_t *snip;
    uint8_t src[IEEE80211_ADDRESS_LEN], dst[IEEE80211_ADDRESS_LEN];
    int src_len, dst_len;

    dst_len = ieee80211_get_dst(mhr, dst);
    src_len = ieee80211_get_src(mhr, src);
    if ((dst_len < 0) || (src_len < 0)) {
        DEBUG("_make_netif_hdr: unable to get addresses\n");
        return NULL;
    }
    /* allocate space for header */
    snip = gnrc_netif_hdr_build(src, (size_t)src_len, dst, (size_t)dst_len);
    if (snip == NULL) {
        DEBUG("_make_netif_hdr: no space left in packet buffer\n");
        return NULL;
    }
    hdr = snip->data;
    /* set broadcast flag for broadcast destination */
    if ((dst_len == 2) && (dst[0] == 0xff) && (dst[1] == 0xff)) {
        hdr->flags |= GNRC_NETIF_HDR_FLAGS_BROADCAST;
    }
    /* set flags for pending frames */
    if (mhr[0] & IEEE802154_FCF_FRAME_PEND) {
        hdr->flags |= GNRC_NETIF_HDR_FLAGS_MORE_DATA;
    }
    return snip;
}

#if MODULE_GNRC_NETIF_DEDUP
static inline bool _already_received(gnrc_netif_t *netif,
                                     gnrc_netif_hdr_t *netif_hdr, uint8_t *mhr)
{
    const uint8_t seq = ieee80211_get_seq(mhr);

    return (netif->last_pkt.seq == seq) &&
           (netif->last_pkt.src_len == netif_hdr->src_l2addr_len) &&
           (memcmp(netif->last_pkt.src, gnrc_netif_hdr_get_src_addr(netif_hdr),
                   netif_hdr->src_l2addr_len) == 0);
}
#endif /* MODULE_GNRC_NETIF_DEDUP */

static gnrc_pktsnip_t *_recv(gnrc_netif_t *netif)
{
    DEBUG("RECV: \n");

    netdev_t *dev = netif->dev;
    netdev_ieee80211_rx_info_t rx_info;
    gnrc_pktsnip_t *pkt = NULL;
    int bytes_expected  = dev->driver->recv(dev, NULL, 0, NULL);

    if (bytes_expected >= (int)IEEE802154_MIN_FRAME_LEN) {
        int nread;

        pkt = gnrc_pktbuf_add(NULL, NULL, bytes_expected, GNRC_NETTYPE_UNDEF);
        if (pkt == NULL) {
            DEBUG("_recv_ieee80211: cannot allocate pktsnip.\n");
            /* Discard packet on netdev device */
            dev->driver->recv(dev, NULL, bytes_expected, NULL);
            return NULL;
        }
        nread = dev->driver->recv(dev, pkt->data, bytes_expected, &rx_info);
        if (nread <= 0) {
            gnrc_pktbuf_release(pkt);
            return NULL;
        }
#ifdef MODULE_NETSTATS_L2
        netif->stats.rx_count++;
        netif->stats.rx_bytes += nread;
#endif

        if (netif->flags & GNRC_NETIF_FLAGS_RAWMODE) {
            /* Raw mode, skip packet processing, but provide rx_info via
             * GNRC_NETTYPE_NETIF */
            gnrc_pktsnip_t *netif_snip = gnrc_netif_hdr_build(NULL, 0, NULL, 0);
            if (netif_snip == NULL) {
                DEBUG("_recv_ieee80211: no space left in packet buffer\n");
                gnrc_pktbuf_release(pkt);
                return NULL;
            }
            gnrc_netif_hdr_t *hdr = netif_snip->data;
            hdr->lqi              = rx_info.lqi;
            hdr->rssi             = rx_info.rssi;
            gnrc_netif_hdr_set_netif(hdr, netif);
            LL_APPEND(pkt, netif_snip);
        } else {
            /* Normal mode, try to parse the frame according to IEEE 802.15.4 */
            gnrc_pktsnip_t *ieee80211_hdr, *netif_hdr;
            gnrc_netif_hdr_t *hdr;
#if ENABLE_DEBUG
            char src_str[GNRC_NETIF_HDR_L2ADDR_PRINT_LEN];
#endif
            size_t mhr_len = ieee80211_get_frame_hdr_len(pkt->data);

            /* nread was checked for <= 0 before so we can safely cast it to
             * unsigned */
            if ((mhr_len == 0) || ((size_t)nread < mhr_len)) {
                DEBUG("_recv_ieee80211: illegally formatted frame received\n");
                gnrc_pktbuf_release(pkt);
                return NULL;
            }
            nread -= mhr_len;
            /* mark IEEE 802.15.4 header */
            ieee80211_hdr = gnrc_pktbuf_mark(pkt, mhr_len, GNRC_NETTYPE_UNDEF);
            if (ieee80211_hdr == NULL) {
                DEBUG("_recv_ieee80211: no space left in packet buffer\n");
                gnrc_pktbuf_release(pkt);
                return NULL;
            }
            netif_hdr = _make_netif_hdr(ieee80211_hdr->data);
            if (netif_hdr == NULL) {
                DEBUG("_recv_ieee80211: no space left in packet buffer\n");
                gnrc_pktbuf_release(pkt);
                return NULL;
            }

            hdr = netif_hdr->data;

#ifdef MODULE_L2FILTER
            if (!l2filter_pass(dev->filter, gnrc_netif_hdr_get_src_addr(hdr),
                               hdr->src_l2addr_len)) {
                gnrc_pktbuf_release(pkt);
                gnrc_pktbuf_release(netif_hdr);
                DEBUG("_recv_ieee80211: packet dropped by l2filter\n");
                return NULL;
            }
#endif
#ifdef MODULE_GNRC_NETIF_DEDUP
            if (_already_received(netif, hdr, ieee80211_hdr->data)) {
                gnrc_pktbuf_release(pkt);
                gnrc_pktbuf_release(netif_hdr);
                DEBUG("_recv_ieee80211: packet dropped by deduplication\n");
                return NULL;
            }
            memcpy(netif->last_pkt.src, gnrc_netif_hdr_get_src_addr(hdr),
                   hdr->src_l2addr_len);
            netif->last_pkt.src_len = hdr->src_l2addr_len;
            netif->last_pkt.seq     = ieee80211_get_seq(ieee80211_hdr->data);
#endif /* MODULE_GNRC_NETIF_DEDUP */

            hdr->lqi  = rx_info.lqi;
            hdr->rssi = rx_info.rssi;
            gnrc_netif_hdr_set_netif(hdr, netif);
            dev->driver->get(dev, NETOPT_PROTO, &pkt->type, sizeof(pkt->type));
#if ENABLE_DEBUG
            DEBUG("_recv_ieee80211: received packet from %s of length %u\n",
                  gnrc_netif_addr_to_str(gnrc_netif_hdr_get_src_addr(hdr),
                                         hdr->src_l2addr_len, src_str),
                  nread);
#if defined(MODULE_OD)
            od_hex_dump(pkt->data, nread, OD_WIDTH_DEFAULT);
#endif
#endif
            gnrc_pktbuf_remove_snip(pkt, ieee80211_hdr);
            LL_APPEND(pkt, netif_hdr);
        }

        DEBUG("_recv_ieee80211: reallocating.\n");
        gnrc_pktbuf_realloc_data(pkt, nread);
    } else if (bytes_expected > 0) {
        DEBUG("_recv_ieee80211: received frame is too short\n");
        dev->driver->recv(dev, NULL, bytes_expected, NULL);
    }

    return pkt;
}

static int _send(gnrc_netif_t *netif, gnrc_pktsnip_t *pkt)
{
    DEBUG("craft a packet sir: \n");
    netdev_t *dev             = netif->dev;
    netdev_ieee80211_t *state = (netdev_ieee80211_t *)netif->dev;
    gnrc_netif_hdr_t *netif_hdr;
    const uint8_t *src, *dst = NULL;
    int res = 0;
    uint8_t mhr[IEEE80211_MAX_HDR_LEN];
    uint8_t flags = (uint8_t)(state->flags & NETDEV_IEEE80211_SEND_MASK);

    /* mac packet FC */
    uint16_t fc = 0;

    /* set data frame type (subtype zero is default data frame) */
    fc |= IEEE80211_FCF_TYPE_DATA << IEEE80211_FC_TYPE_OFFSET;

    //  flags |= IEEE802154_FCF_TYPE_DATA;
    if (pkt == NULL) {
        DEBUG("_send_ieee80211: pkt was NULL\n");
        return -EINVAL;
    }
    if (pkt->type != GNRC_NETTYPE_NETIF) {
            DEBUG("_send_ieee80211: first header is not generic netif header\n"); 
            return -EBADMSG;
    }
    netif_hdr = pkt->data;
    if (netif_hdr->flags & GNRC_NETIF_HDR_FLAGS_MORE_DATA) {
        /* Set frame pending field */
        flags |= IEEE80211_FCF_FRAME_MORE_DATA;
    }
    /* prepare destination address */
    if (netif_hdr->flags & /* If any of these flags is set assume
    broadcast */
        (GNRC_NETIF_HDR_FLAGS_BROADCAST | GNRC_NETIF_HDR_FLAGS_MULTICAST)) {
        dst     = ieee80211_addr_bcast;
    } else {
        dst     = gnrc_netif_hdr_get_dst_addr(netif_hdr);
    }
    src = gnrc_netif_hdr_get_src_addr(netif_hdr);
   
    /* fill MAC header, seq should be set by device */
    if ((res = ieee80211_set_frame_hdr(
        mhr, src, dst, NULL, flags,
                                       state->seq++)) == 0) {
        DEBUG("_send_ieee80211: Error preperaring frame\n");
        return -EINVAL;
    }

    /* prepare iolist for netdev / mac layer */
    iolist_t iolist = { .iol_next = (iolist_t *)pkt->next,
                        .iol_base = mhr,
                        .iol_len  = (size_t)res };

    #ifdef MODULE_NETSTATS_L2
        if (netif_hdr->flags &
            (GNRC_NETIF_HDR_FLAGS_BROADCAST |
            GNRC_NETIF_HDR_FLAGS_MULTICAST)) { netif->stats.tx_mcast_count++;
        } else {
            netif->stats.tx_unicast_count++;
        }
    #endif

    res = dev->driver->send(dev, &iolist);

    /* release old data */
    gnrc_pktbuf_release(pkt);
    return 0;
}
/** @} */
