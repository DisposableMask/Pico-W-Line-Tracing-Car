#include "subsystem3.h"
#include "../robot_hal.h"
#include <sys/machine.h>

typedef struct {
  uint32_t timestamp;
  bool level_high;
} BarcodeEdge;

#define BARCODE_EDGE_QUEUE_SIZE 16u
#define BARCODE_EDGE_LOST 1u

// Set to false if your bench test showed that black reads 0.
#define BARCODE_BLACK_IS_HIGH true

static BarcodeEdge edge_queue[BARCODE_EDGE_QUEUE_SIZE];
static uint8_t read_index;
static uint8_t write_index;
static bool capture_invalid;

static bool take_barcode_edge(BarcodeEdge *edge_out, bool *edge_lost)
{
  UINT interrupt_state;
  DI(interrupt_state);

  // Discard queued records from the broken capture before returning any edge.
  *edge_lost = capture_invalid;
  if (*edge_lost) {
    read_index = write_index;
    capture_invalid = false;
  }

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

  const uint8_t next_write_index = (write_index + 1u) % BARCODE_EDGE_QUEUE_SIZE;

  const bool edge_is_ambiguous = edges != HAL_GPIO_EDGE_RISE &&
    edges != HAL_GPIO_EDGE_FALL;
  const bool queue_is_full = next_write_index == read_index;

  // Wake the task to discard the capture if an edge can't be recorded.
  if (edge_is_ambiguous || queue_is_full) {
    capture_invalid = true;
    (void)tk_set_flg(barcode_edge_flag_id, BARCODE_EDGE_LOST);

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

  const UINT wake_flags = HAL_GPIO_EDGE_RISE | HAL_GPIO_EDGE_FALL | BARCODE_EDGE_LOST;
  uint32_t black_started_at = 0;
  bool measuring_black = false;

  for (;;) {
    UINT received_edges;

    // Wake when an edge arrives or the capture needs to be discarded.
    if (tk_wai_flg(barcode_edge_flag_id, wake_flags,
                   TWF_ORW | TWF_BITCLR, &received_edges, TMO_FEVR) != E_OK) {
      continue;
    }

    // One wake-up can cover several edges. Drain them in arrival order.
    BarcodeEdge edge;
    bool edge_lost;
    while (take_barcode_edge(&edge, &edge_lost)) {
      const bool is_black = edge.level_high == BARCODE_BLACK_IS_HIGH;

      // Entering black starts the measurement. Returning to white ends it.
      if (is_black) {
        black_started_at = edge.timestamp;
        measuring_black = true;
      } else if (measuring_black) {
        const uint32_t width_us = edge.timestamp - black_started_at;
        measuring_black = false;
        tm_printf((const UB *)"[Barcode] black_width_us=%u\n", (UINT)width_us);
      }
    }

    // Drop an unfinished bar so the next white edge can't finish it.
    if (edge_lost) {
      measuring_black = false;
      tm_printf((const UB *)"[Barcode] Edge lost. Capture discarded.\n");
    }
  }
}
