
#include <stdint.h>

/**
 * @brief Frame Types
 * @{
 */
typedef enum {
    WL_MNG = 0 /**< Management type */,
    WL_CTRL, /**< Control type */
    WL_DATA  /**< Data type */
} wlan_80211_frame_type;
/** }@ */

/**
 * @brief Management frame syb types
 *
 */
typedef enum {
    as_req = 0,    /**< Association Request */
    as_resp,       /**< Association Response */
    ras_req,       /**< Reassociation Request */
    ras_resp,      /**< Reassociation Response */
    probe_req,     /**< Probe Response */
    probe_resp,    /**< Probe Response */
    timing_add,    /**< Timing Advertisement */
    reserved_1,    /**< Reserved */
    beacon,        /**< Beacon */
    atim,          /**< ATIM */
    dias,          /**< disassociation */
    auth,          /**< Authentication */
    deauth,        /**< Deauthentication */
    action,        /**< Action */
    action_no_ack, /**< Action No Ack */
    reserved_2,    /**< reserved 2 */
} wlan_80211_mng_subtype;
/** }@ */

/**
 * @brief Management frame syb types
 *
 */
typedef enum {
    beam_rp = 4,   /**< Beamform Report Poll */
    vht_ndp_an,    /**< VHT NDP Announcement */
    ctrl_fr_ext,   /**< Control Frame Extension */
    ctrl_wrapper,  /**< Control Wrapper */
    block_ack_req, /**< Block ACK request */
    block_ack,     /**< Block ACK */
    ps_poll,       /**< PS-Poll */
    rts,           /**< RTS */
    cts,           /**< CTS */
    WL_ACK,        /**< ACK */
    cf_end,        /**< CF End */
    cf_end_ack,    /**< CF End +CF Ack */
} wlan_80211_ctrl_subtype;
/** }@ */

typedef struct wlan_80211_frame_control_t {
    uint8_t version : 2;            // protocol version
    wlan_80211_frame_type type : 2; // type
    uint8_t subtype : 4;            // subtype
    bool to_ds : 1;                 // to destination flag
    bool from_ds : 1;               // from destination flag
    bool more_framents : 1;         // more frames flag
    bool retry : 1;                 // retry flag
    bool pm : 1;                    // power management
    bool more_data : 1;             // more_data
    bool protected_frame : 1;       // protected frame
    bool htc_order : 1; // +HTC / Order (set to 1 in a non-QoS Data Frame)
} wlan_80211_frame_control_t;

typedef struct wlan_80211_header_common_t {
    union {
        wlan_80211_frame_control_t bits; // frame control
        uint16_t raw;
    } fc;              /**< Frame Control */
    uint16_t duration; // duration

} wlan_80211_header_common_t;

typedef struct wlan_80211_cap_info_t {
    // uint8_t dmg; /**< DMG Parameters */
    uint8_t ess : 1;
    uint8_t ibss : 1;
    uint8_t cfp : 2;
    uint8_t privacy : 1;
    uint8_t short_preamble : 1;
    uint8_t pbcc : 1;
    uint8_t channel_agility : 1;
    uint8_t spec_mng : 1;          /**< Spectrum Management */
    uint8_t RESERVED : 1;          /**< RESERVED */
    uint8_t short_slot_time : 1;   /**< Short slot time */
    uint8_t auto_pwr_save : 1;     /**< Automatic Power Save Delivery */
    uint8_t radio_measure : 1;     /**< Readio Measurement */
    uint8_t dsss_ofmd : 1;         /**< DSSS OFMD */
    uint8_t delayed_block_ack : 1; /**< Delayed Block ACK */
    uint8_t im_block_ack : 1;      /**< Immediate Block ACK */
} wlan_80211_cap_info_t;

typedef struct wlan_80211_association_req_t {
    wlan_80211_header_common_t header;
    unsigned char recv[6]; // receiver MAC
    // uint8_t dest[6];   // destination MAC
    // uint8_t transmitter[6]; // transmitter MAC
    unsigned char src[6];     // src MAC
    unsigned char bssid[6];   // bssid MAC
    uint16_t seq_ctl;         // Sequence control
    uint16_t cap_info;        /**< Capabilities info */
    uint16_t listen_interval; /**< Listen Interval */
} wlan_80211_association_req_t;

typedef struct wlan_80211_ack_t {
    wlan_80211_header_common_t header;
    unsigned char recv[6]; // receiver MAC
} wlan_80211_ack_t;

typedef struct wlan_80211_basic_frame_t {
    wlan_80211_header_common_t header;
    unsigned char recv[6]; // receiver MAC
    unsigned char src[6];  // source MAC
} wlan_80211_basic_frame_t;