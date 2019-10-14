/**
 * @ingroup     drivers_netdev_ieee80211
 * @{
 *
 * @file
 * @author  Wladislaw Meixner
 */

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include "net/eui64.h"
#include "net/netdev.h"
#include "random.h"

#include "net/netdev/ieee80211.h"
#include "net/netopt.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

/**
 * @brief
 *
 * @param dev
 * @param value
 * @param max_len
 * @return int
 */
static int _get_iid(netdev_ieee80211_t *dev, eui64_t *value, size_t max_len)
{
    DEBUG("%s()\n", __FUNCTION__);
    uint8_t addr[IEEE80211_ADDRESS_LEN];
    uint16_t addr_len = IEEE80211_ADDRESS_LEN;

    assert(max_len >= sizeof(eui64_t));

    /* get device network address */
    dev->netdev.driver->get(&dev->netdev, NETOPT_SRC_LEN, &addr,
                            sizeof(addr_len));

    /* convert address to IID */
    ieee80211_get_iid(value, addr, addr_len);

    return sizeof(eui64_t);
}

void netdev_ieee80211_reset(netdev_ieee80211_t *dev)
{
    DEBUG("netdev_ieee80211_reset\n");
}

int netdev_ieee80211_get(netdev_ieee80211_t *dev, netopt_t opt, void *value,
                         size_t max_len)
{
    DEBUG("%s(%s)\n", __FUNCTION__, netopt2str(opt));

    int res = -ENOTSUP;

    switch (opt) {
    case NETOPT_ADDRESS:
        assert(max_len >= IEEE80211_ADDRESS_LEN);
        memcpy(value, dev->addr, sizeof(dev->addr));
        return sizeof(dev->addr);
    case NETOPT_ADDR_LEN:
    case NETOPT_SRC_LEN:
        assert(max_len == sizeof(int16_t));
        *((uint16_t *)value) = IEEE80211_ADDRESS_LEN;
        return sizeof(uint16_t);
    /* compute size of the MAC PDU */
    case NETOPT_MAX_PDU_SIZE:
        assert(max_len >= sizeof(int16_t));
        *((uint16_t *)value) = IEEE80211_FRAME_LEN_MAX - IEEE80211_MAX_HDR_LEN;
        return sizeof(uint16_t);
    case NETOPT_CHANNEL:
        assert(max_len == sizeof(uint16_t));
        *((uint16_t *)value) = (uint16_t)dev->chan;
        res                  = sizeof(dev->chan);
        break;
    /* set device type to unknown for now, when finished add
     * NETDEV_TYPE_IEEE80211 type */
    case NETOPT_DEVICE_TYPE:
        *((uint16_t *)value) = NETDEV_TYPE_IEEE80211;
        return sizeof(uint16_t);
    case NETOPT_IPV6_IID:
        res = _get_iid(dev, value, max_len);
        break;
    default:
        DEBUG("%s: %s not supported\n", __func__, netopt2str(opt));
        break;
    }

    return res;
}

int netdev_ieee80211_set(netdev_ieee80211_t *dev, netopt_t opt,
                         const void *value, size_t len)
{
    DEBUG("%s(%s)\n", __FUNCTION__, netopt2str(opt));
    int res = -ENOTSUP;

    switch (opt) {
    case NETOPT_CHANNEL: {
        assert(len == sizeof(uint16_t));
        uint16_t chan = *((uint16_t *)value);
        /* real validity needs to be checked by device, since sub-GHz and
         * 2.4 GHz band radios have different legal values. Here we only
         * check that it fits in an 8-bit variabl*/
        assert(chan <= UINT8_MAX);
        dev->chan = chan;
        res       = sizeof(uint16_t);
        break;
    }
    case NETOPT_ADDRESS:
        assert(len <= sizeof(dev->addr));
        memset(dev->addr, 0, sizeof(dev->addr));
        memcpy(dev->addr, value, len);
        res = sizeof(dev->addr);
        break;
    case NETOPT_ADDR_LEN:
    case NETOPT_SRC_LEN:
        break;
        // #ifdef MODULE_GNRC
        //     case NETOPT_PROTO:
        //         assert(len == sizeof(gnrc_nettype_t));
        //         dev->proto = *((gnrc_nettype_t *)value);
        //         res        = sizeof(gnrc_nettype_t);
        //         break;
        // #endif
    default:
        break;
    }
    return res;
}

int netdev_ieee80211_dst_filter(netdev_ieee80211_t *dev, const uint8_t *mhr)
{
    DEBUG("netdev_ieee80211_dst_filter\n");
    (void)dev;
    (void)mhr;
    // uint8_t dst_addr[IEEE802154_ADDRESS_LEN];

    // int addr_len = ieee80211_get_dst(mhr, dst_addr);

    // /* check destination address */
    // if (((addr_len == IEEE802154_SHORT_ADDRESS_LEN) &&
    //      (memcmp(dev->short_addr, dst_addr, addr_len) == 0 ||
    //       memcmp(ieee802154_addr_bcast, dst_addr, addr_len) == 0)) ||
    //     ((addr_len == IEEE802154_LONG_ADDRESS_LEN) &&
    //      (memcmp(dev->long_addr, dst_addr, addr_len) == 0))) {
    //     return 0;
    // }

    return 1;
}

/** @} */
