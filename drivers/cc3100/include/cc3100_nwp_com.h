#include "cc3100.h"
#include "cc3100_internal.h"
#include "cc3100_protocol.h"

/**
 * @brief nwp commands
 *
 */
int16_t _nwp_get_net_cfg(cc3100_t *dev, cc31xx_net_cfg_t configId,
                         uint8_t *configOpt, uint8_t configLen, uint8_t *resp);
int16_t _nwp_set_net_cfg(cc3100_t *dev, uint8_t configId, uint8_t configOpt,
                         uint8_t configLen, uint8_t *payload);
int16_t _nwp_get_dvc_info(cc3100_t *dev, SlVersionFull *ver);
int16_t _nwp_set_wifi_cfg(cc3100_t *dev, uint8_t configId, uint8_t configOpt,
                          uint8_t configLen, uint8_t *resp);
int16_t _nwp_set_wifi_mode(cc3100_t *dev, uint8_t mode);
int16_t _nwp_connect(cc3100_t *dev, WifiProfileConfig *conf);
int16_t _nwp_disconnect(cc3100_t *dev);
int16_t _nwp_sock_create(cc3100_t *dev, int16_t domain, int16_t type,
                         int16_t protocol);
int16_t _nwp_set_sock_opt(cc3100_t *dev, uint16_t sock, uint16_t level,
                          uint16_t optionName, void *optionVal,
                          uint8_t optionLen);
int16_t _nwp_set_wifi_filter(cc3100_t *dev, uint8_t filterOptions,
                             uint8_t *inBuf, uint16_t bufLen);
int16_t _nwp_send_raw_frame(cc3100_t *dev, int16_t sock, void *buf, int16_t len,
                            int16_t options);
int16_t _nwp_send_frame_to(cc3100_t *dev, int16_t sock, void *buf, uint16_t len,
                           int16_t flags, SlSockAddr_t *to, uint16_t toLen);
int16_t _nwp_read_raw_frame(cc3100_t *dev, int16_t sock, void *buf,
                            int16_t bufLen, int16_t options);
int16_t _nwp_set_wifi_policy(cc3100_t *dev, uint8_t type, uint8_t policy);
int16_t _nwp_del_profile(cc3100_t *dev, int16_t index);
int16_t _nwp_get_profile(cc3100_t *dev, int16_t index);
int16_t _nwp_profile_add(cc3100_t *dev, WifiProfileConfig *conf);