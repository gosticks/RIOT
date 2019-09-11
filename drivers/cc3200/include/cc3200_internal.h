
// SPI SPEEDS
#define SPI_RATE_13M 13000000
#define SPI_RATE_20M 20000000
#define SPI_RATE_30M 30000000

// ROM VERSIONS
#define ROM_VER_PG1_21 1
#define ROM_VER_PG1_32 2
#define ROM_VER_PG1_33 3

#define REG_INT_MASK_SET 0x400F7088
#define REG_INT_MASK_CLR 0x400F708C
#define APPS_SOFT_RESET_REG 0x4402D000
#define OCP_SHARED_MAC_RESET_REG 0x4402E168
#define ROM_VERSION_ADDR 0x00000400

#define SL_RAW_RF_TX_PARAMS_CHANNEL_SHIFT (0)
#define SL_RAW_RF_TX_PARAMS_RATE_SHIFT (6)
#define SL_RAW_RF_TX_PARAMS_POWER_SHIFT (11)
#define SL_RAW_RF_TX_PARAMS_PREAMBLE_SHIFT (15)

#define SL_RAW_RF_TX_PARAMS(chan, rate, power, preamble) \
    ((chan << SL_RAW_RF_TX_PARAMS_CHANNEL_SHIFT) |       \
     (rate << SL_RAW_RF_TX_PARAMS_RATE_SHIFT) |          \
     (power << SL_RAW_RF_TX_PARAMS_POWER_SHIFT) |        \
     (preamble << SL_RAW_RF_TX_PARAMS_PREAMBLE_SHIFT))

#define SL_POLICY_CONNECTION (0x10)
#define SL_POLICY_SCAN (0x20)
#define SL_POLICY_PM (0x30)
#define SL_POLICY_P2P (0x40)

#define VAL_2_MASK(position, value) ((1 & (value)) << (position))
#define MASK_2_VAL(position, mask) (((1 << position) & (mask)) >> (position))

#define SL_CONNECTION_POLICY(Auto, Fast, Open, anyP2P, autoSmartConfig) \
    (VAL_2_MASK(0, Auto) | VAL_2_MASK(1, Fast) | VAL_2_MASK(2, Open) |  \
     VAL_2_MASK(3, anyP2P) | VAL_2_MASK(4, autoSmartConfig))
#define SL_SCAN_POLICY_EN(policy) (MASK_2_VAL(0, policy))
#define SL_SCAN_POLICY(Enable) (VAL_2_MASK(0, Enable))

#define SL_NORMAL_POLICY (0)
#define SL_LOW_LATENCY_POLICY (1)
#define SL_LOW_POWER_POLICY (2)
#define SL_ALWAYS_ON_POLICY (3)
#define SL_LONG_SLEEP_INTERVAL_POLICY (4)
