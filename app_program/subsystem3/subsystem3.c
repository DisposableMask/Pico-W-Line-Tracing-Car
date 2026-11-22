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

ID ir_event_flag_id;
ID ir_snapshot_lock_id;

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

  // Create the lock that protects each saved reading and timestamp
  const T_CMTX lock_settings = { .mtxatr = TA_INHERIT };
  ir_snapshot_lock_id = tk_cre_mtx(&lock_settings);
  if (ir_snapshot_lock_id < E_OK) {
    return ir_snapshot_lock_id;
  }

  // Create the worker's wake-up flag and release the lock if setup fails
  const T_CFLG event_settings = { 0 };
  ir_event_flag_id = tk_cre_flg(&event_settings);

  if (ir_event_flag_id < E_OK) {
    (void)tk_del_mtx(ir_snapshot_lock_id);
    ir_snapshot_lock_id = 0;

    return ir_event_flag_id;
  }
  return E_OK;
}

EXPORT ER subsystem3_start(void)
{
  if (ir_event_flag_id <= 0) {

    return E_OBJ;
  }

  ID worker_task_id = app_start_task(ir_bench_task, S3_PRI_MAIN, 0, "ir_bench");
  if (worker_task_id < E_OK) {

    return worker_task_id;
  }

  return tk_set_flg(ir_event_flag_id, IR_SAMPLE_REQUESTED);
}
