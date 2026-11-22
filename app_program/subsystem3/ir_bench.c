#include "subsystem3.h"
#include <sys/machine.h>

static void ir_bench_task(INT stacd, void *exinf)
{
  (void)stacd;
  (void)exinf;

  IR_Snapshot fl_last_state = {
    .sensor = installed_ir_sensors.front_left
  };

  IR_Snapshot fr_last_state = {
    .sensor = installed_ir_sensors.front_right
  };

  for (uint32_t current_tick = 0;;current_tick++) {
    UW levels = in_w(GPIO_IN);

    IR front_left = installed_ir_sensors.front_left;
    IR front_right = installed_ir_sensors.front_right;

    // bitmask gpio pins to get ir sensor gpio pin, read value
    front_left.reading = (uint16_t)(levels >> front_left.gpio) & 1u;
    front_right.reading = (uint16_t)(levels >> front_right.gpio) & 1u;

    if (
      is_ir_equal(front_left, fl_last_state.sensor) &&
      front_left.reading != fl_last_state.sensor.reading
    ) {
      fl_last_state.sensor.reading = front_left.reading;
      fl_last_state.timestamp = current_tick;

      // print it to serial monitor
      tm_printf(
        (const UB *)"left=%u right=%u\nleft_timestamp=%u\n",
        (UINT)front_left.reading,
        (UINT)front_right.reading,
        (UINT)fl_last_state.timestamp
      );
    }

    if (
      is_ir_equal(front_right, fr_last_state.sensor) &&
      front_right.reading != fr_last_state.sensor.reading
    ) {
      fr_last_state.sensor.reading = front_right.reading;
      fr_last_state.timestamp = current_tick;
      // print it to serial monitor
      tm_printf(
        (const UB *)"left=%u right=%u\nright_timestamp=%u\n",
        (UINT)front_left.reading,
        (UINT)front_right.reading,
        (UINT)fr_last_state.timestamp
      );
    }


    // print it to serial monitor
//   tm_printf(
//     (const UB *)"left=%u right=%u\n",
//     (UINT)ir_sensors.front_left.reading,
//     (UINT)ir_sensors.front_right.reading
//   );

    // task repeat delay in ms
    tk_dly_tsk(10);
  }
}

ER ir_bench_start(void)
{
  ID tid = app_start_task(ir_bench_task, S3_PRI_MAIN, 0, "ir_bench");
  return tid < E_OK ? tid : E_OK;
}
