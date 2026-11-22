#include "subsystem3.h"
#include "../robot_hal.h"
#include <sys/machine.h>

typedef struct {
  uint32_t timestamp;
  bool level_high;
} BarcodeEdge;

#define BARCODE_EDGE_QUEUE_SIZE 16u

static BarcodeEdge edge_queue[BARCODE_EDGE_QUEUE_SIZE];
static uint8_t read_index;
static uint8_t write_index;

static bool take_barcode_edge(BarcodeEdge *edge_out)
{
  UINT interrupt_state;
  DI(interrupt_state);

  // Copy one record before letting the interrupt handler run again.
  const bool edge_available = read_index != write_index;
  if (edge_available) {
    *edge_out = edge_queue[read_index];
    read_index = (read_index + 1u) % BARCODE_EDGE_QUEUE_SIZE;
  }

  EI(interrupt_state);
  return edge_available;
}

void barcode_pin_changed(uint32_t gpio, uint32_t edges)
{
  const uint32_t timestamp = in_w(TIMER_TIMERAWL);
  (void)gpio;

  // A single edge tells us the new level. Both bits together are ambiguous.
  if (edges != HAL_GPIO_EDGE_RISE && edges != HAL_GPIO_EDGE_FALL) {
    return;
  }

  const uint8_t next_write_index = (write_index + 1u) % BARCODE_EDGE_QUEUE_SIZE;

  // Leave one slot unused so equal indices mean the queue is empty.
  if (next_write_index == read_index) {
    return;
  }

  edge_queue[write_index] = (BarcodeEdge) {
    .timestamp = timestamp,
    .level_high = edges == HAL_GPIO_EDGE_RISE,
  };
  write_index = next_write_index;
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

    // One wake-up can cover several edges. Drain them in arrival order.
    BarcodeEdge edge;
    while (take_barcode_edge(&edge)) {
      tm_printf((const UB *)"[Barcode] level=%u timestamp=%u\n",
                (UINT)edge.level_high, (UINT)edge.timestamp);
    }
  }
}
