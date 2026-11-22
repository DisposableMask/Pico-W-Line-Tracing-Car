/*
 *	subsystem3.c
 *	Subsystem 3 -- Consumer: creates its objects and starts its task.
 *
 *	APIs shown: tk_cre_mbx, tk_cre_mtx
 */

#include "subsystem3.h"

Installed_IR installed_ir_sensors = {
  .front_left  = { .gpio = 27u, .is_digital = true },
  .front_right = { .gpio = 0u, .is_digital = true },
  .barcode     = { .gpio = 30u, .is_digital = true }
};

EXPORT ER subsystem3_init(void)
{
  /* Set up IR driver and shared reading here */
  tm_printf((const UB *)"c::subsystem3_init()");

  // Initialise pin, check status, if fail early return
  ER err = gpio_set_pin(installed_ir_sensors.front_left.gpio, GPIO_MODE_IN);
  if (err != E_OK) {
    return err;
  }

  // Initialise pin, return status
  err = gpio_set_pin(installed_ir_sensors.front_right.gpio, GPIO_MODE_IN);
  if (err != E_OK) {
    return err;
  }
}

EXPORT ER subsystem3_start(void)
{
  return ir_bench_start();
}
