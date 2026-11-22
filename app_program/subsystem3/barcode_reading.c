#include "subsystem3.h"
#include "../robot_hal.h"
#include <sys/machine.h>

typedef struct {
  uint32_t timestamp;
  bool level_high;
} BarcodeEdge;

#define BARCODE_EDGE_QUEUE_SIZE 32u
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

static bool classify_widths(const uint32_t widths_us[9], char pattern[10])
{
  bool widths_valid = true;

  for (unsigned int i = 0; i < 9u; ++i) {
    // Mark zero widths invalid and keep building the diagnostic pattern.
    if (widths_us[i] == 0u) {
      widths_valid = false;
    }
    pattern[i] = '0';
  }
  pattern[9] = '\0';

  for (unsigned int colour = 0; colour < 2u; ++colour) {
    const unsigned int wide_count = colour == 0u ? 2u : 1u;
    uint32_t smallest_wide = 0;

    // Mark the widest elements of this colour in their original positions.
    for (unsigned int pick = 0; pick < wide_count; ++pick) {
      unsigned int widest = 9u;
      for (unsigned int i = colour; i < 9u; i += 2u) {
        if (pattern[i] == '0' &&
            (widest == 9u || widths_us[i] > widths_us[widest])) {
          widest = i;
        }
      }
      pattern[widest] = '1';
      smallest_wide = widths_us[widest];
    }

    uint64_t narrow_sum = 0;
    for (unsigned int i = colour; i < 9u; i += 2u) {
      if (pattern[i] == '0') {
        narrow_sum += widths_us[i];
      }
    }

    // Require the smallest wide element to be at least 1.4 times the
    // average of the three narrow elements of this colour.
    if (15u * (uint64_t)smallest_wide < 7u * narrow_sum) {
      widths_valid = false;
    }
  }

  return widths_valid;
}

static char decode_character(const char pattern[10])
{
  static const uint16_t letter_patterns[] = {
    0x109u, 0x049u, 0x148u, 0x019u, 0x118u, 0x058u, 0x00du,
    0x10cu, 0x04cu, 0x01cu, 0x103u, 0x043u, 0x142u, 0x013u,
    0x112u, 0x052u, 0x007u, 0x106u, 0x046u, 0x016u, 0x181u,
    0x0c1u, 0x1c0u, 0x091u, 0x190u, 0x0d0u,
  };

  uint16_t bits = 0;
  for (unsigned int i = 0; i < 9u; ++i) {
    bits = (uint16_t)((bits << 1u) | (pattern[i] == '1'));
  }

  // The opening and closing markers use the same pattern.
  if (bits == 0x094u) {
    return '*';
  }

  for (unsigned int i = 0; i < sizeof(letter_patterns) / sizeof(letter_patterns[0]); ++i) {
    if (bits == letter_patterns[i]) {
      return (char)('A' + i);
    }
  }

  return '\0';
}

static char decode_and_print_character(const uint32_t widths_us[9], bool looking_for_start)
{
  char pattern[10];
  const bool widths_valid = classify_widths(widths_us, pattern);
  const char character = widths_valid ? decode_character(pattern) : '\0';

  // While searching, print only a valid opening marker.
  if (looking_for_start && character != '*') {
    return '\0';
  }

  for (unsigned int i = 0; i < 9u; ++i) {
    tm_printf((const UB *)"[Barcode] width[%u] %s=%u us\n",
              (UINT)i, (i % 2u == 0u) ? "black" : "white",
              (UINT)widths_us[i]);
  }
  tm_printf((const UB *)"[Barcode] pattern=%s widths_valid=%u\n",
            pattern, (UINT)widths_valid);
  // Report the character immediately after its nine widths.
  if (character != '\0') {
    tm_printf((const UB *)"[Barcode] character=%c\n", character);
  }
  return character;
}

void barcode_reading_task(INT start_code, void *task_context)
{
  (void)start_code;
  (void)task_context;

  const UINT wake_flags = HAL_GPIO_EDGE_RISE | HAL_GPIO_EDGE_FALL | BARCODE_EDGE_LOST;

  uint32_t widths_us[9];
  uint32_t previous_edge_at = 0;
  unsigned int width_count = 0;
  bool capturing = false;
  bool looking_for_start = true;

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
        
      // Start the character on an edge into black.
      if (!capturing) {
        if (is_black) {
          previous_edge_at = edge.timestamp;
          width_count = 0;
          capturing = true;
        }
        continue;
      }

      // The first bar must end on white. Later edges must alternate.
      const bool expected_black = (width_count % 2u) != 0u;
      if (is_black != expected_black) {
        capturing = false;
        looking_for_start = true;
        continue;
      }

      widths_us[width_count++] = edge.timestamp - previous_edge_at;
      previous_edge_at = edge.timestamp;

      // Decode one character as soon as its nine widths are ready.
      if (width_count == 9u) {
        const char character =
            decode_and_print_character(widths_us, looking_for_start);

        // Keep seven widths and try the next black bar as a start candidate.
        if (looking_for_start && character != '*') {
          for (unsigned int i = 0; i < 7u; ++i) {
            widths_us[i] = widths_us[i + 2u];
          }
          width_count = 7u;
          continue;
        }

        // The first '*' opens the scan. A closing '*' or bad character resets it.
        if (character == '*') {
          looking_for_start = !looking_for_start;
        } else if (character == '\0') {
          looking_for_start = true;
        }

        capturing = false;
      }
    }

    // Discard the partial character if any edge was lost.
    if (edge_lost) {
      capturing = false;
      looking_for_start = true;
      width_count = 0;
      tm_printf((const UB *)"[Barcode] Edge lost. Capture discarded.\n");
    }
  }
}
