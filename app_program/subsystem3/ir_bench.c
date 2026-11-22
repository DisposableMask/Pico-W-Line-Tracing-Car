#include "subsystem3.h"
#include <sys/machine.h>

static void ir_bench_task(INT stacd, void *exinf)
{
  (void)stacd;
  (void)exinf;

  for (;;) {
    UW levels = in_w(GPIO_IN);

    // bitmask gpio pins to get ir sensor gpio pin, read value
    ir_sensors.front_left.reading = (uint16_t)(levels >> ir_sensors.front_left.gpio) & 1u;
    ir_sensors.front_right.reading = (uint16_t)(levels >> ir_sensors.front_right.gpio) & 1u;

    // print it to serial monitor
    tm_printf(
      (const UB *)"left=%u right=%u\n",
      (UINT)ir_sensors.front_left.reading,
      (UINT)ir_sensors.front_right.reading
    );

    // task repeat delay in ms
    tk_dly_tsk(10);
  }
}

ER ir_bench_start(void)
{
  ID tid = app_start_task(ir_bench_task, S3_PRI_MAIN, 0, "ir_bench");
  return tid < E_OK ? tid : E_OK;
}
