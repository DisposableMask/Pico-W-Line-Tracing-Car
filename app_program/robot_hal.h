#ifndef ROBOT_HAL_H
#define ROBOT_HAL_H

#include <stdint.h>

#define HAL_GPIO_EDGE_FALL (0x4u)
#define HAL_GPIO_EDGE_RISE (0x8u)
#define HAL_IRQ_IO_BANK0   (13u)

void hal_gpio_irq_enable(uint32_t pin, uint32_t events);
uint32_t hal_gpio_irq_take_events(uint32_t pin, uint32_t mask);

#endif
