

#include "periph/gpio.h"
#include "periph/spi.h"
#include "vendor/hw_udma.h"
#include "vendor/hw_ints.h"
#include "vendor/hw_memmap.h"
#include "driverlib/prcm.h"
#include "driverlib/pin.h"
#include "driverlib/spi.h"
#include "driverlib/utils.h"

typedef struct WifiModule {
    int fd;
} WifiModule;

typedef struct CC3200_RomInfo
{
    unsigned short ucMajorVerNum;
    unsigned short ucMinorVerNum;
    unsigned short ucSubMinorVerNum;
    unsigned short ucDay;
    unsigned short ucMonth;
    unsigned short ucYear;
}CC3200_RomInfo;

typedef void (*SimpleLinkEventHandler)(void);

#define SimpleLinkEventHandler SimpleLinkEventHandler

//SPI rate determined by the ROM version
#define ROM_VER_PG1_21                  1
#define ROM_VER_PG1_32                  2
#define ROM_VER_PG1_33                  3
#define SPI_RATE_13M 		            13000000
#define SPI_RATE_20M                    20000000
#define REG_INT_MASK_SET                0x400F7088
#define REG_INT_MASK_CLR                0x400F708C
#define APPS_SOFT_RESET_REG             0x4402D000
#define OCP_SHARED_MAC_RESET_REG        0x4402E168
#define ROM_VERSION_ADDR                0x00000400

// TODO: move to separate file
#define INT_PRIORITY_LVL_0      0x00
#define INT_PRIORITY_LVL_1      0x20
#define INT_PRIORITY_LVL_2      0x40
#define INT_PRIORITY_LVL_3      0x60
#define INT_PRIORITY_LVL_4      0x80
#define INT_PRIORITY_LVL_5      0xA0
#define INT_PRIORITY_LVL_6      0xC0
#define INT_PRIORITY_LVL_7      0xE0

WifiModule* test;
unsigned char g_ucDMAEnabled = 0;

/**
 * @brief
 * 
 * @return CC3200_RomInfo* 
 */
CC3200_RomInfo* _get_cc3200_rom_info(void) {
    return (CC3200_RomInfo *)(0x00000400);
}

void _mask_wifi_interrupt(void) {
    HWREG(0x400F7088) = 0x1;
}


int _register_wifi_interrupt_handler(SimpleLinkEventHandler handler){
    MAP_IntRegister(INT_NWPIC, handler);
    MAP_IntPrioritySet(INT_NWPIC, 0x20);
    MAP_IntPendClear(INT_NWPIC);
    MAP_IntEnable(INT_NWPIC);
    return 0;
}

void _clear_wifi_interrupt_handler(void) {
    MAP_IntDisable(INT_NWPIC);
	MAP_IntUnregister(INT_NWPIC);
	MAP_IntPendClear(INT_NWPIC);

    // TODO: also clear any IO specific parts
}

void _power_off_wifi(void) {
    // must delay 300 usec to enable the NWP to finish all sl_stop activities
	UtilsDelay(300*80/3);

    // mask network processor interrupt interrupt
    _mask_wifi_interrupt();

    //Switch to PFM Mode
    HWREG(0x4402F024) &= 0xF7FFFFFF;

    //sl_stop eco for PG1.32 devices
    HWREG(0x4402E16C) |= 0x2;

    UtilsDelay(800000);
}

void _power_on_wifi(void) {
    //bring the 1.32 eco out of reset
    HWREG(0x4402E16C) &= 0xFFFFFFFD;

    // NWP Wakeup
    HWREG(0x44025118) = 1;

    // UnMask Host interrupt
    (*(unsigned long *)REG_INT_MASK_CLR) = 0x1;
}

void _wifi_handler(void *value) {
    if (value != NULL) {
        puts(value);
    }
    _mask_wifi_interrupt();
}

/**
 * @brief init wifi SPI and return FD
 * 
 * @return int 
 */
int _init_wifi_spi(void) {
    // TODO: later remove check when replaced calls
    // get rom version.
    unsigned long ulBase = LSPI_BASE;
    unsigned long ulSpiBitRate = 0;
    CC3200_RomInfo* rom_info = _get_cc3200_rom_info();

    
    //
    // Configure SPI interface
	//

    if(rom_info->ucMinorVerNum == ROM_VER_PG1_21 )
    {
    	ulSpiBitRate = SPI_RATE_13M;
    }
    else if(rom_info->ucMinorVerNum == ROM_VER_PG1_32)
    {
    	ulSpiBitRate = SPI_RATE_13M;
    }
    else if(rom_info->ucMinorVerNum >= ROM_VER_PG1_33)
    {
    	ulSpiBitRate = SPI_RATE_20M;
    }
    //NWP master interface
    // ulBase = LSPI_BASE;

    MAP_SPIConfigSetExpClk(ulBase,MAP_PRCMPeripheralClockGet(PRCM_LSPI),
                    ulSpiBitRate,SPI_MODE_MASTER,SPI_SUB_MODE_0,
                    (SPI_SW_CTRL_CS |
                    SPI_4PIN_MODE |
                    SPI_TURBO_OFF |
                    SPI_CS_ACTIVEHIGH |
                    SPI_WL_32));

	if(MAP_PRCMPeripheralStatusGet(PRCM_UDMA))
	{
	  g_ucDMAEnabled = (HWREG(UDMA_BASE + UDMA_O_CTLBASE) != 0x0) ? 1 : 0;
	}

    MAP_SPIEnable(ulBase);
    return 1;
}


void _init_wifi_module(void) {
    _power_off_wifi();

    // register callback when wifi module is powered back on
    _register_wifi_interrupt_handler((SimpleLinkEventHandler)_wifi_handler);

    _power_on_wifi();
}


/**
 * @brief prepare wifi module to be operational
 * 
 */
void init_wifi(void) {
    gpio_set(LED_RED);
    int fd = _init_wifi_spi();
    _init_wifi_module();
    gpio_clear(LED_RED);
    gpio_set(LED_GREEN);

    WifiModule t = {.fd = fd};
    test = &t;
}
