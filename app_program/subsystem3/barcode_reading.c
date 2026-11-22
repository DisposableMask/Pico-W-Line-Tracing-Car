#include "subsystem3.h"
#include "../robot_hal.h"
#include <sys/machine.h>

typedef struct {
  uint32_t timestamp;
  bool level_high;
} BarcodeEdge;

static BarcodeEdge latest_edge;
static bool edge_available;

void barcode_pin_changed(uint32_t gpio, uint32_t edges)
{
  const uint32_t timestamp = in_w(TIMER_TIMERAWL);
  (void)gpio;

  // A single edge tells us the new level. Both bits together are ambiguous.
  if (edges != HAL_GPIO_EDGE_RISE && edges != HAL_GPIO_EDGE_FALL) {
    return;
  }

  latest_edge = (BarcodeEdge) {
    .timestamp = timestamp,
    .level_high = edges == HAL_GPIO_EDGE_RISE,
  };

  edge_available = true;
  (void)tk_set_flg(barcode_edge_flag_id, edges);
}

void barcode_reading_task(INT start_code, void *task_context)
{
  (void)start_code;
  (void)task_context;

  const UINT both_edges = HAL_GPIO_EDGE_RISE | HAL_GPIO_EDGE_FALL;

  for (;;) {
    UINT received_edges;

    // Wake on either edge and clear the flag bits that woke the task.
    if (tk_wai_flg(barcode_edge_flag_id, both_edges,
                   TWF_ORW | TWF_BITCLR, &received_edges, TMO_FEVR) != E_OK) {
      continue;
    }

    // Both the callback and this task run on core 0. Pause interrupts for the copy.
    UINT interrupt_state;
    DI(interrupt_state);
    const BarcodeEdge edge = latest_edge;
    const bool captured = edge_available;
    edge_available = false;
    EI(interrupt_state);

    // Print in task context after interrupts are restored.
    if (captured) {
      tm_printf((const UB *)"[Barcode] level=%u timestamp=%u\n",
                (UINT)edge.level_high, (UINT)edge.timestamp);
    }
  }
}
