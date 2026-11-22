#include "robot_hal.h"

#include <sys/machine.h>
#include <tk/tkernel.h>

#define GPIO_RAW_EDGES_OFFSET     0xF0u
#define GPIO_CORE0_ENABLE_OFFSET  0x100u
#define GPIO_CORE0_PENDING_OFFSET 0x120u

static UW gpio_irq_register_address(const UW offset, const uint32_t gpio)
{
  // Each register holds the interrupt bits for eight pins.
  return IO_BANK0_BASE + offset + 4u * (gpio / 8u);
}

void hal_gpio_irq_enable(uint32_t gpio, uint32_t events)
{
  const UINT shift = 4u * (gpio % 8u);
  const UW edge_bits = events << shift;
  // Acknowledge stale edges before enabling this pin on core 0.
  out_w(gpio_irq_register_address(GPIO_RAW_EDGES_OFFSET, gpio), edge_bits);
  set_w(gpio_irq_register_address(GPIO_CORE0_ENABLE_OFFSET, gpio), edge_bits);
}

static UW get_edge_mask(const UINT gpio)
{
  // Get this pin's rising and falling edge bits.
  return (HAL_GPIO_EDGE_RISE | HAL_GPIO_EDGE_FALL) << (4u * (gpio % 8u));
}

uint32_t hal_gpio_irq_take_events(uint32_t gpio)
{
  const UINT shift = 4u * (gpio % 8u);
  const UW pending_edges = in_w(gpio_irq_register_address(GPIO_CORE0_PENDING_OFFSET, gpio)) & get_edge_mask(gpio);

    // The raw status register clears only the edge bits written as ones.
    if (pending_edges != 0u) {
      out_w(gpio_irq_register_address(GPIO_RAW_EDGES_OFFSET, gpio), pending_edges);
    }
    return pending_edges >> shift;
}
