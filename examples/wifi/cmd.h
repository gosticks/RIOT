#include "proto.h"
#include <stdbool.h>

// NWP config utils
void getDeviceInfo(SlVersionFull *ver);
int16_t getNetConfig(uint8_t configId, uint8_t *configOpt, uint8_t confLen, uint8_t *resp);
int16_t setNetConfig(uint8_t configId, uint8_t configOpt, uint8_t confLen, uint8_t *payload);
int16_t setWifiConfig(uint8_t configId, uint8_t configOpt, uint8_t confLen, uint8_t *payload);
int16_t setWifiPolicy(uint8_t type, uint8_t policy);
int16_t setWifiMode(uint8_t mode);
int16_t setWlanFilter(uint8_t filterOptions, const uint8_t *const inBuf, uint16_t bufLen);

int16_t sendRawTraceiverData(int16_t sock, uint8_t *buf, int16_t len, int16_t options);
int16_t recvRawTraceiverData(int16_t sock, void *buf, int16_t bufLen,
                             int16_t options);
// profile management
int16_t profileAdd(WifiProfileConfig *conf);
int16_t getProfile(int16_t index);
int16_t deleteProfile(int16_t index);
int16_t setSocketOptions(uint16_t sock, uint16_t level, uint16_t optionName,
                         const void *optionVal, uint8_t optionLen);

int16_t connect(WifiProfileConfig *conf);
int16_t disconnectFromWifi(void);
int16_t openSocket(int16_t domain, int16_t type, int16_t protocol);