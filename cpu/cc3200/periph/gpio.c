/*
 * Copyright (C) 2019 Ludwig Maximilian Universität 
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup        cpu_cc3200
 * @{
 *
 * @file
 * @brief           CPU specific definitions and functions for peripheral handling
 *
 * @author          Wladislaw Meixner <wladislaw.meixner@campus.lmu.de>
 */

#include <stdint.h>
#include <stdbool.h>

#include "cpu.h"
#include "sched.h"
#include "thread.h"
#include "periph/gpio.h"
#include "vendor/hw_gpio.h"
#include "board.h"
#include "periph_conf.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

/**
 * PIN value HIGH/LOW IRQ trigger
 */
#define HIGH 1
#define LOW 0

#ifdef MODULE_PERIPH_GPIO_IRQ

/**
 * @brief   static callback memory
 */

static gpio_isr_ctx_t isr_ctx[4][8];
#endif /* MODULE_PERIPH_GPIO_IRQ */

#define PAD_CONFIG_BASE ((OCP_SHARED_BASE + \
                          OCP_SHARED_O_GPIO_PAD_CONFIG_0))
#define NOT_A_PORT 0
#define NOT_A_PIN 0
#define NOT_A_GPIO 66

// GPIO PORTS
static unsigned long ulReg[]=
{
    GPIOA0_BASE,
    GPIOA1_BASE,
    GPIOA2_BASE,
    GPIOA3_BASE,
    GPIOA4_BASE
};


/**
 * @brief PIN to PAD matrix
 * 
 */
static const unsigned long g_ulPinToPadMap[64] =
    {
        10, 11, 12, 13, 14, 15, 16, 17, 255, 255, 18,
        19, 20, 21, 22, 23, 24, 40, 28, 29, 25, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        31, 255, 255, 255, 255, 0, 255, 32, 30, 255, 1,
        255, 2, 3, 4, 5, 6, 7, 8, 9};

static const unsigned long gpio_to_mcu_pin[32] = {
	49, 54, 56, 57, 58, 59, 60, 61, 62, 63, 0, 1, 2, 3, 4, 5, 6, 7, NOT_A_GPIO, NOT_A_GPIO, NOT_A_GPIO, NOT_A_GPIO, 14, 15, 16, 20, NOT_A_GPIO, NOT_A_GPIO, 17, NOT_A_GPIO, 52, 44 
};

/**
 * @brief check if a port base is valid
 * 
 * @param port 
 * @return true 
 * @return false 
 */
bool  _gpioPortBaseValid(unsigned long port) {
	return((port == GPIOA0_BASE) ||
		(port == GPIOA1_BASE) ||
		(port == GPIOA2_BASE) ||
		(port == GPIOA3_BASE) ||             
		(port == GPIOA4_BASE));
}

/**
 * @brief     Extract the pin number of the given pin
 */
static inline uint8_t _pin_num(gpio_t pin)
{
    return (pin & 0x3f);
}

/**
 * @brief     Extract the port number of the given pin
 */
static inline uint8_t _port_num(gpio_t pin)
{
    return (pin >> 6) & 0x3;
}

static inline int8_t _mcu_pin_num(gpio_t pin) {
	return gpio_to_mcu_pin[pin];
}

/**
 * @brief _gpio_pin_to_imux converts external pin numbers (e.g. printed on the board) to internal pin numbers
 * 
 * @param dev 
 * @return gpio pin offset for port
 */
uint8_t _gpio_pin_to_imux(uint8_t pin) {
	return 1 << (pin % GPIO_PINS_PER_PORT);
}
/**
 * @brief _gpio_pin_to_port returns the port base address for a pin
 * 
 * @param dev external pin number
 * @return port base address 
 */
unsigned long _gpio_pin_to_port(uint8_t port) {
	return ulReg[port];
}

/**
 * @brief returns the masked value for a external pin
 * 
 * @param pin external pin
 * @param val value to be masked
 * @return masked value for a given pin with value val 
 */
unsigned char _gpio_pin_value_mask(uint8_t pin, unsigned char val) {
	return val << (pin % 8);
}

/**
 * @brief configure pin type and pin streng based on TIs pin.c/PinConfigGet
 * 
 * @param pin 
 * @param strength 
 * @param type 
 */
// void _set_pin_config(unsigned long pin, unsigned long type) {
// 	// get pin padding
// 	unsigned long pad = g_ulPinToPadMap[pin & 0x3F];
// 	if (type == GPIO_IN_ANALOG) {
// 		// isolate the input
// 		HWREG(0x4402E144) |= ((0x80 << ulPad) & (0x1E << 8));

// 		// get register address
//     	ulPad = ((ulPad << 2) + PAD_CONFIG_BASE);

// 	    // isolate the output
//     	HWREG(ulPad) = 0xC00;
// 	} else {
// 		// enable input
// 		// only for digital pins
// 		HWREG(0x4402E144) &= ~((0x80 << pad) & (0x1E << 8));

// 		// compute pin address
// 		pad = ((pad << 2) + PAD_CONFIG_BASE);

// 		// write the configuration
// 		// 0x00000020 is PIN_STRENGTH_2MA
// 		// 0x00000000 is PIN_TYPE_STD 
// 		// TODO: move constans     
// 		HWREG(pad) = ((HWREG(pad) & ~(PAD_STRENGTH_MASK | PAD_TYPE_MASK)) |
// 						(0x00000020 | type));

// 		// set the pin mode
// 		HWREG(pad) = (((HWREG(pad) & ~PAD_MODE_MASK) | mode) & ~(3 << 10);
// 	}
// }
/**
 * @brief set gpio to read or write mode
 * 
 * @param portAddr base address of a GPIO port
 * @param pins bitmap of which pins mode should be set
 * @param inOrOut GPIO_IN | GPIO_OUT
 */
// void _set_gpio_dir_mode(unsigned portAddr, uint8_t pins, uint8_t inOrOut) {
// 	HWREG(portAddr + GPIO_O_GPIO_DIR) = ((inOrOut & 1) ?
//                                   (HWREG(portAddr + GPIO_O_GPIO_DIR) | pins) :
//                                   (HWREG(portAddr + GPIO_O_GPIO_DIR) & ~(pins)));
// }

/**
 * @brief 
 * 
 * @param dev 
 * @param mode 
 * @return int 
 */
int gpio_init(gpio_t dev, gpio_mode_t mode)
{
	uint8_t port = _port_num(dev);
	uint8_t pin = _pin_num(dev);
	unsigned long mcuPinNum = _mcu_pin_num(pin);

	if (mcuPinNum == NOT_A_GPIO) {
		return -1;
	}

	// get gpio port pin 
	uint8_t ipin = _gpio_pin_to_imux(pin);
	unsigned long portAddr = _gpio_pin_to_port(port);


	DEBUG("GPIO %"PRIu32", PORT: %u, PIN: %u\n", (uint32_t)dev, port, pin);

	// set pin to GPIO mode (does not to be done for only gpio pins but has no consequence and ist faster the reading first)
	MAP_PinTypeGPIO(mcuPinNum, PIN_MODE_0, false);


	// set gpio direction IN/OUT
	if (mode == GPIO_OUT) {
		MAP_GPIODirModeSet(portAddr, ipin, GPIO_DIR_MODE_OUT);
	} else {
		MAP_GPIODirModeSet(portAddr, ipin, GPIO_DIR_MODE_IN);
	}

	switch (mode) {
	case GPIO_IN:
	case GPIO_OUT:
	case GPIO_OD:
		MAP_PinConfigSet(mcuPinNum, PIN_STRENGTH_2MA, PIN_TYPE_STD);
		break;
	case GPIO_OD_PU:
	case GPIO_IN_PU:
		MAP_PinConfigSet(mcuPinNum, PIN_STRENGTH_2MA, PIN_TYPE_STD_PU);
		break;
	case GPIO_IN_PD:
		MAP_PinConfigSet(mcuPinNum, PIN_STRENGTH_2MA, PIN_TYPE_STD_PD);
		break;
	}

	return 0;
}

#ifdef MODULE_PERIPH_GPIO_IRQ

void isr_gpio_a0(void)
{
	handle_isr(GPIOA0_BASE);
}

void isr_gpio_a1(void)
{
	handle_isr(GPIOA1_BASE);
}

void isr_gpio_a2(void)
{
	handle_isr(GPIOA2_BASE);
}

void isr_gpio_a3(void)
{
	handle_isr(GPIOA3_BASE);
}

/**
 * @brief isr interrupt handler
 * 
 * @param portAddr base address of the GPIO PORT
 */
void handle_isr(uint32_t portAddr)
{
	uint32_t state = HWREG(portAddr + GPIO_O_GPIO_MIS);

	MAP_GPIOIntClear(portAddr, state);

	for (int i = 0; i < 8; i++)
	{
		if (state & (1 << i)) {
            isr_ctx[port_num][i].cb(isr_ctx[port_num][i].arg);
        }
	}

	cortexm_isr_end();
}


/**
 * @brief enable GPIO interrupt 
 * @param dev pin
 */
void gpio_irq_enable(gpio_t dev)
{
	// uint8_t bit = _gpio_pin_to_imux(dev);
	// uint8_t portAddr = _gpio_pin_to_port(dev);
	// MAP_GPIOIntEnable(portAddr, bit);
}

/**
 * @brief disable GPIO interrupt 
 * @param dev pin
 */
void gpio_irq_disable(gpio_t dev)
{
	// uint8_t bit = _gpio_pin_to_imux(dev);
	// uint8_t portAddr = _gpio_pin_to_port(dev);
	// MAP_GPIOIntDisable(portAddr, bit);
}

int gpio_init_int(gpio_t dev, gpio_mode_t mode, gpio_flank_t flank,
				  gpio_cb_t cb, void *arg)
{

	/* Note: gpio_init() also checks if the gpio is enabled. */
	int res = gpio_init(dev, mode);
	if (res != 0) {
		return res;
	}

	uint8_t portNum = _port_num(dev);
	uint8_t pinNum = _pin_num(dev);
	uint8_t bit = _gpio_pin_to_imux(dev);
	uint8_t portAddr = _gpio_pin_to_port(dev);

	// store callback information;
	isr_ctx[portNum][pinNum].cb = cb;
	isr_ctx[portNum][pinNum].arg = arg;

	MAP_IntMasterDisable();
	MAP_GPIOIntClear(portBase, bit);

	// configure active flanks
	switch (flank) {
		case GPIO_LOW:
			MAP_GPIOIntTypeSet(portAddr, bit, 0x00000002);
			break;
		case GPIO_BOTH:
			MAP_GPIOIntTypeSet(portAddr, bit, 0x00000001);
			break;
		case GPIO_RISING:
			MAP_GPIOIntTypeSet(portAddr, bit, 0x00000004);
			break;
		case GPIO_FALLING:
			MAP_GPIOIntTypeSet(portAddr, bit, 0x00000000);
			break;
		default:
			return -1;
	}

	MAP_GPIOIntEnable(portAddr, bit);

	switch (portBase) {
	case GPIOA0_BASE:
		MAP_GPIOIntRegister(portBase, isr_gpio_a0);
		MAP_IntEnable(INT_GPIOA0);
		break;
	case GPIOA1_BASE:
		MAP_GPIOIntRegister(portBase, isr_gpio_a1);
		MAP_IntEnable(INT_GPIOA1);
		break;
	case GPIOA2_BASE:
		MAP_GPIOIntRegister(portBase, isr_gpio_a2);
		MAP_IntEnable(INT_GPIOA2);
		break;
	case GPIOA3_BASE:
		MAP_GPIOIntRegister(portBase, isr_gpio_a3);
		MAP_IntEnable(INT_GPIOA3);
		break;
	}

	MAP_IntMasterEnable();

	return 0;
}
#endif

/**
 * @brief gpio_write writes to a GPIO pin dev (external) the value value
 * 
 * @param dev external pin
 * @param value value to be written (will be masked)
 */
void gpio_write(gpio_t dev, int value)
{
	uint8_t port = _port_num(dev);
	uint8_t pin = _pin_num(dev);
	unsigned char ipin = _gpio_pin_to_imux(pin);
	unsigned long portAddr = _gpio_pin_to_port(port); 
	// write to pin at portBase + pinOffset 
	HWREG(portAddr + (0x00000000 + (ipin << 2))) = _gpio_pin_value_mask(pin,value);
}

/**
 * @brief read a pins value
 * 
 * @param dev external pin
 * @return int current value of a pin
 */
int gpio_read(gpio_t dev)
{
	uint8_t port = _port_num(dev);
	uint8_t pin = _pin_num(dev);
	unsigned char ipin = _gpio_pin_to_imux(pin);
	unsigned long portAddr = _gpio_pin_to_port(port); 

	// read from pin at portBase + pinOffset
	// cast value to int {0, 1}
	return (HWREG(portAddr + (0x00000000 + (ipin << 2))) ? HIGH : LOW);
}

/**
 * @brief set a pins value to HIGH (1)
 * 
 * @param dev external pin
 */
void gpio_set(gpio_t dev)
{
	gpio_write(dev, HIGH);
}

/**
 * @brief set a pins value to LOW (0)
 * 
 * @param dev external pin
 */
void gpio_clear(gpio_t dev)
{
	gpio_write(dev, LOW);
}


/**
 * @brief toggle a gpio pins value
 * 
 * @param dev external pin
 */
void gpio_toggle(gpio_t dev)
{
	gpio_read(dev) ? gpio_clear(dev) : gpio_set(dev);
}

