#include "subsystem3.h"
#include <sys/machine.h>

static IR_Snapshot fl_last_state;
static IR_Snapshot fr_last_state;

static void ir_bench_task(INT stacd, void *exinf)
{
  (void)stacd;
  (void)exinf;


  const UW initial_levels = in_w(GPIO_IN);
  const uint32_t start_time = in_w(TIMER_TIMERAWL);

  // bitmask gpio pins to get ir sensor gpio pin, read value
  fl_last_state.reading = (uint16_t)(initial_levels >> installed_ir_sensors.front_left.gpio) & 1u;
  fr_last_state.reading = (uint16_t)(initial_levels >> installed_ir_sensors.front_right.gpio) & 1u;

  fl_last_state.timestamp = start_time;
  fr_last_state.timestamp = start_time;

  // print it to serial monitor
  tm_printf(
    (const UB *)"initial left=%u right=%u\n",
    (UINT)fl_last_state.timestamp,
    (UINT)fr_last_state.timestamp
  );

  for (;;) {
    UW levels = in_w(GPIO_IN);
    uint32_t sampled_timestamp = in_w(TIMER_TIMERAWL);

    IR front_left = installed_ir_sensors.front_left;
    IR front_right = installed_ir_sensors.front_right;

    // bitmask gpio pins to get ir sensor gpio pin, read value
    front_left.reading = (uint16_t)(levels >> front_left.gpio) & 1u;
    front_right.reading = (uint16_t)(levels >> front_right.gpio) & 1u;

    if (front_left.reading != fl_last_state.reading) {
      const uint32_t elapsed_time = sampled_timestamp - fl_last_state.timestamp;
      fl_last_state.reading = front_left.reading;
      fl_last_state.timestamp = sampled_timestamp;

      // print it to serial monitor
      tm_printf(
        (const UB *)"left=%u right=%u\nleft_elapsed=%u\n",
        (UINT)front_left.reading,
        (UINT)front_right.reading,
        (UINT)elapsed_time
      );
    }

    if (front_right.reading != fr_last_state.reading) {
      const uint32_t elapsed_time = sampled_timestamp - fr_last_state.timestamp;
      fr_last_state.reading = front_right.reading;
      fr_last_state.timestamp = sampled_timestamp;
      // print it to serial monitor
      tm_printf(
        (const UB *)"left=%u right=%u\nright_elapsed=%u\n",
        (UINT)front_left.reading,
        (UINT)front_right.reading,
        (UINT)elapsed_time
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
