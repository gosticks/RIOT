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
 * @brief           CPU specific definitions and functions for peripheral
 * handling
 *
 * @author          Wladislaw Meixner <wladislaw.meixner@campus.lmu.de>
 */

#include <stdbool.h>
#include <stdint.h>

#include "board.h"
#include "cpu.h"
#include "periph/gpio.h"
#include "periph_conf.h"
#include "sched.h"
#include "thread.h"
#include "vendor/hw_gpio.h"
#include "vendor/hw_memmap.h"
#include "vendor/hw_ocp_shared.h"

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

#define GPIO_PORT_MASK (0xfffff000) /**< bit mask for GPIO port addr  */
#define NOT_A_PORT 0
#define NOT_A_PIN 0
#define NOT_A_GPIO 66
#define PAD_MODE_MASK 0x0000000F
#define PAD_STRENGTH_MASK 0x000000E0
#define PAD_TYPE_MASK 0x00000310
#define PAD_CONFIG_BASE ((OCP_SHARED_BASE + OCP_SHARED_O_GPIO_PAD_CONFIG_0))

/**
 * @brief gpio base addresses
 *
 */
static unsigned long ports[] = {GPIOA0_BASE, GPIOA1_BASE, GPIOA2_BASE,
                                GPIOA3_BASE, GPIOA4_BASE};

/**
 * @brief pin to GPIO pin numbers mappings
 *
 */
static const unsigned long pin_to_gpio_num[64] = {
    10,  11,  12,  13,  14,  15,  16,  17,  255, 255, 18,  19,  20,
    21,  22,  23,  24,  40,  28,  29,  25,  255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 31,  255, 255, 255, 255, 0,   255, 32,
    30,  255, 1,   255, 2,   3,   4,   5,   6,   7,   8,   9};

/**
 * @brief check if a port base is valid
 *
 * @param port
 * @return true
 * @return false
 */
bool _gpioPortBaseValid(unsigned long port) {
  return ((port == GPIOA0_BASE) || (port == GPIOA1_BASE) ||
          (port == GPIOA2_BASE) || (port == GPIOA3_BASE) ||
          (port == GPIOA4_BASE));
}

/**
 * @brief     Extract the pin number of the given pin
 */
static inline uint8_t _pin_num(gpio_t pin) { return (pin & 0x3f); }

/**
 * @brief     Extract the port number of a given pin
 */
static inline uint8_t _port_num(gpio_t pin) { return (pin >> 6) & 0x3; }

/**
 * @brief get pin mask for a given pin
 *
 * @param dev
 * @return gpio pin offset for port
 */
uint8_t _gpio_pin_mask(uint8_t dev) {
  return 1 << (pin_to_gpio_num[_pin_num(dev)] % GPIO_PINS_PER_PORT);
}
/**
 * @brief _gpio_pin_to_port returns the port base address for a pin
 *
 * @param dev external pin number
 * @return port base address
 */
unsigned long _gpio_pin_to_port(uint8_t port) { return ports[port]; }

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
 * @brief Access GPIO low-level device
 *
 * @param[in] pin   gpio pin
 *
 * @return          pointer to gpio low level device address
 */
static inline cc3200_gpio_t *gpio(gpio_t pin) {
  return (cc3200_gpio_t *)(_gpio_pin_to_port(_port_num(pin)));
}

void gpio_init_af(gpio_t dev, uint32_t strength, uint32_t type) {
  // does not support analog pin types, but not a problem for GPIO
  uint8_t pin = _pin_num(dev);
  uint8_t gpio_pin = pin_to_gpio_num[pin];

  // copied from TIs PinConfigSet. The register is not documented so for now
  // only replecate behaviour.

  // enable input
  HWREG(0x4402E144) &= ~((0x80 << gpio_pin) & (0x1E << 8));

  // compute pin register
  unsigned long regAddr = (gpio_pin << 2) + PAD_CONFIG_BASE;

  // write config
  HWREG(regAddr) = ((HWREG(regAddr) & ~(PAD_STRENGTH_MASK | PAD_TYPE_MASK)) |
                    (strength | type));
}

void gpio_pin_mode_set(gpio_t dev, uint32_t mode) {
  // does not support analog pin types, but not a problem for GPIO
  uint8_t pin = _pin_num(dev);
  uint8_t gpio_pin = pin_to_gpio_num[pin];

  // compute pin register
  unsigned long regAddr = (gpio_pin << 2) + PAD_CONFIG_BASE;

  // set mode
  HWREG(regAddr) = (((HWREG(regAddr) & ~PAD_MODE_MASK) | mode) & ~(3 << 10));
}

int gpio_init(gpio_t dev, gpio_mode_t mode) {
  uint8_t pin = _pin_num(dev);

  // get pin mask
  uint8_t ipin = _gpio_pin_mask(dev);

  // make sure pin is in the default state (this is quicker then reading first)
  gpio_init_af(dev, PIN_STRENGTH_2MA, PIN_TYPE_STD);
  gpio_pin_mode_set(dev, PIN_MODE_0);

  // set gpio direction IN/OUT
  if (mode == GPIO_OUT) {
    gpio(dev)->dir |= ipin;
  } else {
    gpio(dev)->dir &= ipin;
  }

  switch (mode) {
  case GPIO_IN:
  case GPIO_OUT:
  case GPIO_OD:
    gpio_init_af(dev, PIN_STRENGTH_2MA, PIN_TYPE_STD);
    break;
  case GPIO_OD_PU:
  case GPIO_IN_PU:
    gpio_init_af(dev, PIN_STRENGTH_2MA, PIN_TYPE_STD_PU);
    break;
  case GPIO_IN_PD:
    gpio_init_af(dev, PIN_STRENGTH_2MA, PIN_TYPE_STD_PD);
    break;
  }

  return 0;
}

#ifdef MODULE_PERIPH_GPIO_IRQ

void isr_gpio_a0(void) { handle_isr((cc3200_gpio_t *)GPIOA0_BASE); }

void isr_gpio_a1(void) { handle_isr((cc3200_gpio_t *)GPIOA1_BASE); }

void isr_gpio_a2(void) { handle_isr((cc3200_gpio_t *)GPIOA2_BASE); }

void isr_gpio_a3(void) { handle_isr((cc3200_gpio_t *)GPIOA3_BASE); }

/**
 * @brief isr interrupt handler
 *
 * @param portAddr base address of the GPIO PORT
 */
void handle_isr(cc3200_gpio_t *u) {
  uint32_t state = u->mis;

  // clear interrupt
  u->icr = state;
  for (int i = 0; i < 8; i++) {
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
void gpio_irq_enable(gpio_t dev) { gpio(dev)->im |= _gpio_pin_mask(dev); }

/**
 * @brief disable GPIO interrupt
 * @param dev pin
 */
void gpio_irq_disable(gpio_t dev) { gpio(dev)->im &= ~(_gpio_pin_mask(dev)); }

int gpio_init_int(gpio_t dev, gpio_mode_t mode, gpio_flank_t flank,
                  gpio_cb_t cb, void *arg) {

  /* Note: gpio_init() also checks if the gpio is enabled. */
  int res = gpio_init(dev, mode);
  if (res != 0) {
    return res;
  }

  assert(flank != GPIO_NONE);

  uint8_t portNum = _port_num(dev);
  uint8_t pinNum = _pin_num(dev);
  uint8_t bit = _gpio_pin_mask(dev);
  uint8_t portAddr = _gpio_pin_to_port(dev);

  // store callback information;
  isr_ctx[portNum][pinNum].cb = cb;
  isr_ctx[portNum][pinNum].arg = arg;

  ROM_IntMasterDisable();

  // clear interrupt specified pin
  gpio(dev)->ICR = bit;

  // configure active flanks
  gpio(dev)->ibe =
      (flank & GPIO_BOTH) ? gpio(dev)->ibe | bit : gpio(dev)->ibe & ~bit;
  gpio(dev)->is =
      (flank & GPIO_LOW) ? gpio(dev)->is | bit : gpio(dev)->is & ~bit;
  gpio(dev)->iev =
      (flank & GPIO_RISING) ? gpio(dev)->iev | bit : gpio(dev)->iev & ~bit;

  // enable gpio interripts
  gpio(dev)->im |= bit;

  // register interrupt handlers
  // TODO: replace with cortex common
  switch (portBase) {
  case GPIOA0_BASE:
    ROM_GPIOIntRegister(portBase, isr_gpio_a0);
    ROM_IntEnable(INT_GPIOA0);
    break;
  case GPIOA1_BASE:
    ROM_GPIOIntRegister(portBase, isr_gpio_a1);
    ROM_IntEnable(INT_GPIOA1);
    break;
  case GPIOA2_BASE:
    ROM_GPIOIntRegister(portBase, isr_gpio_a2);
    ROM_IntEnable(INT_GPIOA2);
    break;
  case GPIOA3_BASE:
    ROM_GPIOIntRegister(portBase, isr_gpio_a3);
    ROM_IntEnable(INT_GPIOA3);
    break;
  }

  ROM_IntMasterEnable();

  return 0;
}
#endif

/**
 * @brief gpio_write writes to a GPIO pin dev (external) the value value
 *
 * @param dev external pin
 * @param value value to be written (will be masked)
 */
void gpio_write(gpio_t dev, int value) {
  uint8_t port = _port_num(dev);
  unsigned char ipin = _gpio_pin_mask(dev);
  unsigned long portAddr = _gpio_pin_to_port(port);
  // write to pin at portBase + pinOffset
  HWREG(portAddr + (0x00000000 + (ipin << 2))) =
      _gpio_pin_value_mask(pin_to_gpio_num[_pin_num(dev)], value);
}

/**
 * @brief read a pins value
 *
 * @param dev external pin
 * @return int current value of a pin
 */
int gpio_read(gpio_t dev) {
  uint8_t port = _port_num(dev);
  unsigned char ipin = _gpio_pin_mask(dev);
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
void gpio_set(gpio_t dev) { gpio_write(dev, HIGH); }

/**
 * @brief set a pins value to LOW (0)
 *
 * @param dev external pin
 */
void gpio_clear(gpio_t dev) { gpio_write(dev, LOW); }

/**
 * @brief toggle a gpio pins value
 *
 * @param dev external pin
 */
void gpio_toggle(gpio_t dev) {
  gpio_read(dev) ? gpio_clear(dev) : gpio_set(dev);
}
