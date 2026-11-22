#include "subsystem3.h"
#include <sys/machine.h>


typedef struct ir_snapshot_slot_t {
  IR_Snapshot snapshot;
  bool has_sample;
} IR_SnapshotSlot;

static IR_SnapshotSlot last_left;
static IR_SnapshotSlot last_right;
static IR_SnapshotSlot last_barcode;

static IR_SnapshotSlot *find_snapshot_slot(const uint8_t gpio) {
  // Match the pin to one of the 3 installed sensors
  if (gpio == installed_ir_sensors.front_left.gpio) {
    return &last_left;
  }

  if (gpio == installed_ir_sensors.front_right.gpio) {
    return &last_right;
  }

  if (gpio == installed_ir_sensors.barcode.gpio) {
    return &last_barcode;
  }

  return NULL;
}

void ir_bench_task(INT start_code, void *task_context)
{
  (void)start_code;
  (void)task_context;

  for (;;) {
    // Wait for a request before reading the front pins
    UINT received_flags;
    const ER wait_result = tk_wai_flg(
      ir_event_flag_id,
      IR_SAMPLE_REQUESTED,
      TWF_ORW | TWF_BITCLR,
      &received_flags,
      TMO_FEVR
    );

    if (wait_result != E_OK) {
      continue;
    }

    const UW pin_levels = in_w(GPIO_IN);
    const uint32_t sampled_time = in_w(TIMER_TIMERAWL);

    const IR_Snapshot left_sample = {
      .reading = (pin_levels >> installed_ir_sensors.front_left.gpio) & 1u,
      .timestamp = sampled_time,
    };
    const IR_Snapshot right_sample = {
      .reading = (pin_levels >> installed_ir_sensors.front_right.gpio) & 1u,
      .timestamp = sampled_time,
    };

    // Hold the lock while reading and updating the saved slots.
    if (tk_loc_mtx(ir_snapshot_lock_id, TMO_FEVR) != E_OK) {
      continue;
    }

    const bool first_sample = !last_left.has_sample;
    const bool left_changed = left_sample.reading != last_left.snapshot.reading;
    const bool right_changed = right_sample.reading != last_right.snapshot.reading;
    const uint32_t left_elapsed = sampled_time - last_left.snapshot.timestamp;
    const uint32_t right_elapsed = sampled_time - last_right.snapshot.timestamp;

    // Save the first left reading, including zero, then save only changes.
    if (!last_left.has_sample || left_changed) {
      last_left.snapshot = left_sample;
      last_left.has_sample = true;
    }

    // Save the right reading independently using the same rule.
    if (!last_right.has_sample || right_changed) {
      last_right.snapshot = right_sample;
      last_right.has_sample = true;
    }

    (void)tk_unl_mtx(ir_snapshot_lock_id);

    // Print the initial pair once, then report elapsed time for each change.
    if (first_sample) {
      tm_printf((const UB *)"initial left=%u right=%u\n",
                (UINT)left_sample.reading, (UINT)right_sample.reading);
    } else {
      // Report the left sensor only when its level changed.
      if (left_changed) {
        tm_printf((const UB *)"left=%u elapsed=%u\n",
                  (UINT)left_sample.reading, (UINT)left_elapsed);
      }
      // Report the right sensor only when its level changed.
      if (right_changed) {
        tm_printf((const UB *)"right=%u elapsed=%u\n",
                  (UINT)right_sample.reading, (UINT)right_elapsed);
      }
    }
  }
}

ER ir_get_snapshot(uint8_t gpio, IR_Snapshot *snapshot_out)
{
  const IR_SnapshotSlot *slot = find_snapshot_slot(gpio);

  // Reject an unknown pin or a missing place to copy the snapshot
  if (slot == NULL || snapshot_out == NULL) {
    return E_PAR;
  }

  // Setup must have created the lock before a task can read a snapshot
  if (ir_snapshot_lock_id <= 0) {
    return E_OBJ;
  }

  // Hold the lock while copying the reading and timestamp together
  ER err = tk_loc_mtx(ir_snapshot_lock_id, TMO_FEVR);
  if (err != E_OK) {
    return err;
  }

  // A slot becomes readable after its first successful capture
  const bool sample_available = slot->has_sample;
  if (sample_available) {
    *snapshot_out = slot->snapshot;
  }

  err = tk_unl_mtx(ir_snapshot_lock_id);

  // Report an unlock error before returning the sample status.
  if (err != E_OK) {
    return err;
  }
  return sample_available ? E_OK : E_OBJ;
}
