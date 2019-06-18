
/**
 * @brief   Override SPI clock speed values
 * @{
 */
#define HAVE_SPI_CLK_T 1
typedef enum {
  SPI_CLK_100KHZ = 100000,  /**< drive the SPI bus with 100KHz */
  SPI_CLK_400KHZ = 400000,  /**< drive the SPI bus with 400KHz */
  SPI_CLK_1MHZ = 1000000,   /**< drive the SPI bus with 1MHz */
  SPI_CLK_4MHZ = 4000000,   /**< drive the SPI bus with 4MHz */
  SPI_CLK_5MHZ = 5000000,   /**< drive the SPI bus with 5MHz */
  SPI_CLK_10MHZ = 10000000, /**< drive the SPI bus with 10MHz */
  SPI_CLK_13MHZ = 13000000, /**< drive the SPI bus with 13MHz */
  SPI_CLK_20MHZ = 13000000, /**< drive the SPI bus with 20MHz */
} spi_clk_t;
/** @} */

/**
 * @brief   Override SPI mode settings
 * @{
 */
#define HAVE_SPI_MODE_T 1
typedef enum {
  SPI_MODE_0 = 0x00000000, /**< CPOL=0, CPHA=0 */
  SPI_MODE_1 = 0x00000001, /**< CPOL=0, CPHA=1 */
  SPI_MODE_2 = 0x00000002, /**< CPOL=1, CPHA=0 */
  SPI_MODE_3 = 0x00000003, /**< CPOL=1, CPHA=1 */
} spi_mode_t;
/** @} */

typedef struct {
    unsigned long mosi; /**< pin used for MOSI */
    unsigned long miso; /**< pin used for MISO */
    unsigned long sck;  /**< pin used for SCK */
    unsigned long cs;   /**< pin used for CS */
} spi_pins_t;

/**
 * @brief   SPI configuration data structure
 * @{
 */
typedef struct {
  unsigned long base_addr; /**< bus address */
  unsigned long gpio_port; /**< GPIO port */
  spi_pins_t pins;
} spi_conf_t;
/** @} */

typedef struct cc3200_spi_hwinfo {
    unsigned long u1;
    unsigned long u2;
    unsigned long u3;
    unsigned long u4;
} cc3200_spi_hwinfo;

typedef struct cc3200_spi_sys_rev {
    unsigned long u0;
    unsigned long u1;
    unsigned long u2;
} cc3200_spi_sys_rev;

typedef struct cc3200_spi_t {
  unsigned long rev; // hardware revision
  cc3200_spi_hwinfo hwinfo; // hardware info (HDL generics)
  char gap_0[240]; // Sysconfig
  cc3200_spi_sys_rev  sys_rev;        // System revision number
  unsigned long sys_conf;      // System config
  unsigned long sys_status;    // Sysstatus
  unsigned long irq_status;    // IRQStatus
  unsigned long irq_enable;    // IRQEnable
  unsigned long wakeup_enable; // Wakeupenable
  unsigned long sys_test;      // system test mode
  unsigned long module_ctl;    // MODULE CTL
  unsigned long ch0_conf_ctl;  // CH0CONF CTL
  unsigned long stat;          // CH0 Status register
  unsigned long ctrl;          // CH0 Control register
  unsigned long tx0;           // single spi transmit word
  unsigned long rx0;           // single spi receive word
} cc3200_spi_t;