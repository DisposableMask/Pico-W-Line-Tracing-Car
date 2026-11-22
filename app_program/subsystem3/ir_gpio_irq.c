#include "subsystem3.h"
#include <sys/machine.h>

#define IO_BANK0_IRQ_NUMBER 13u
#define GPIO_EDGE_BOTH     0xCu
#define GPIO_EDGE_STATUS   0xF0u
#define GPIO_CORE0_ENABLE  0x100u
#define GPIO_CORE0_PENDING 0x120u

static UW get_edge_register(const UW offset, const UINT gpio)
{
  // Each register holds the interrupt bits for eight pins.
  return IO_BANK0_BASE + offset + 4u * (gpio / 8u);
}

static UW get_edge_mask(const UINT gpio)
{
  // Each pin has four bits. Enable its rising and falling edges.
  return GPIO_EDGE_BOTH << (4u * (gpio % 8u));
}

static bool acknowledge_edges(const UINT gpio)
{
  const UW pending_edges = in_w(edge_register(GPIO_CORE0_PENDING, gpio)) & edge_mask(gpio);

  // Writing back the pending bits acknowledges only those edges.
  if (pending_edges != 0u) {
    out_w(edge_register(GPIO_EDGE_STATUS, gpio), pending_edges);
  }
  return pending_edges != 0u;
}

static void ir_gpio_irq_handler(UINT interrupt_number)
{
  (void)interrupt_number;

  const bool left_changed = acknowledge_edges(installed_ir_sensors.front_left.gpio);
  const bool right_changed = acknowledge_edges(installed_ir_sensors.front_right.gpio);

  // Wake the worker once after checking both pins.
  if (left_changed || right_changed) {
    (void)tk_set_flg(ir_event_flag_id, IR_SAMPLE_REQUESTED);
  }
}

ER ir_gpio_irq_init(void)
{
  const T_DINT handler_settings = {
    .intatr = TA_HLNG,
    .inthdr = ir_gpio_irq_handler,
  };

  // Install the bank handler before enabling either pin.
  ER err = tk_def_int(IO_BANK0_IRQ_NUMBER, &handler_settings);
  if (err != E_OK) {
    return err;
  }

  const UINT left_gpio = installed_ir_sensors.front_left.gpio;
  const UINT right_gpio = installed_ir_sensors.front_right.gpio;

  // Clear stale edges, then route both pins to core 0.
  out_w(edge_register(GPIO_EDGE_STATUS, left_gpio), edge_mask(left_gpio));
  out_w(edge_register(GPIO_EDGE_STATUS, right_gpio), edge_mask(right_gpio));
  set_w(edge_register(GPIO_CORE0_ENABLE, left_gpio), edge_mask(left_gpio));
  set_w(edge_register(GPIO_CORE0_ENABLE, right_gpio), edge_mask(right_gpio));
  EnableInt(IO_BANK0_IRQ_NUMBER, 2);
  return E_OK;
}
