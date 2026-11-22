/*
 *	subsystem3.c
 *	Subsystem 3 -- Consumer: creates its objects and starts its task.
 *
 *	APIs shown: tk_cre_mbx, tk_cre_mtx
 */

#include "subsystem3.h"
#include "../gpio_irq.h"

Installed_IR installed_ir_sensors = {
  .front_left  = { .gpio = 27u, .is_digital = true },
  .front_right = { .gpio = 0u, .is_digital = true },
  .barcode     = { .gpio = 6u, .is_digital = true }
};

ID ir_event_flag_id;
ID barcode_edge_flag_id;
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

  // Configure the barcode module's digital output as a Pico input.
  err = gpio_set_pin(installed_ir_sensors.barcode.gpio, GPIO_MODE_IN);
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

  // Give barcode edges their own flag so they don't wake the front IR worker.
  barcode_edge_flag_id = tk_cre_flg(&event_settings);
  if (barcode_edge_flag_id < E_OK) {
    (void)tk_del_flg(ir_event_flag_id);
    (void)tk_del_mtx(ir_snapshot_lock_id);

    ir_event_flag_id = 0;
    ir_snapshot_lock_id = 0;

    return barcode_edge_flag_id;
  }

  return E_OK;
}

static void ir_pin_changed(uint32_t gpio, uint32_t edges)
{
  (void)gpio;
  (void)edges;
  (void)tk_set_flg(ir_event_flag_id, IR_SAMPLE_REQUESTED);
}

static void barcode_pin_changed(uint32_t gpio, uint32_t edges)
{
  (void)gpio;
  (void)tk_set_flg(barcode_edge_flag_id, edges);
}

static void ir_worker_on_core0(INT start_code, void *task_context)
{
  const uint32_t both_edges = HAL_GPIO_EDGE_RISE | HAL_GPIO_EDGE_FALL;
  ER err = gpio_irq_init();

  // Register the front pins after the shared handler is installed.
  if (err == E_OK) {
    err = gpio_irq_register(
      installed_ir_sensors.front_left.gpio,
      both_edges,
      ir_pin_changed
    );
  }
  if (err == E_OK) {
    err = gpio_irq_register(
      installed_ir_sensors.front_right.gpio,
      both_edges,
      ir_pin_changed
    );
  }

  // Route both barcode pin edges to the barcode callback.
  if (err == E_OK) {
    err = gpio_irq_register(installed_ir_sensors.barcode.gpio, both_edges, barcode_pin_changed);
  }

  // Request the initial sample even if neither pin changes at boot.
  if (err == E_OK) {
    err = tk_set_flg(ir_event_flag_id, IR_SAMPLE_REQUESTED);
  }

  // Stop this worker if interrupt setup failed.
  if (err != E_OK) {
    tm_printf((const UB *)"[IR] Interrupt setup failed: %d\n", (INT)err);
    tk_ext_tsk();
    return;
  }

  ir_bench_task(start_code, task_context);
}

EXPORT ER subsystem3_start(void)
{
  // Start only after subsystem3_init() creates the event flag
  if (ir_event_flag_id <= 0) {

    return E_OBJ;
  }

  ID worker_task_id = app_start_task(ir_worker_on_core0, S3_PRI_MAIN, TP_PRC1, "ir_bench");

  return worker_task_id < E_OK ? worker_task_id : E_OK;
}
