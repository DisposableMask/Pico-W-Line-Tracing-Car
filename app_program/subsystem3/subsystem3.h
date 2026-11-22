/*
 *	subsystem3.h
 *	Subsystem 3 -- Consumer (statistics).  Student ID: <student ID>.
 *
 *	Public interface of subsystem 3.  Other subsystems include it as
 *	"../subsystem3/subsystem3.h".
 */

#ifndef SUBSYSTEM3_H
#define SUBSYSTEM3_H

#include <stdint.h>
#include <stdbool.h>
#include "../app.h"

#define S3_PRI_MAIN 16

typedef struct ir_sensor_t {
  const uint8_t gpio;
  const bool is_digital;
  uint16_t reading;
} IR;

typedef struct installed_ir_t {
  IR front_left;
  IR front_right;
  IR barcode;
} Installed_IR;

extern Installed_IR installed_ir_sensors;

typedef struct ir_snapshot_t {
  uint16_t reading;
  uint32_t timestamp;
} IR_Snapshot;


/* Add IR reading type and getter here once interface is ready */

#define IR_SAMPLE_REQUESTED 1u

extern ID ir_event_flag_id;
extern ID barcode_edge_flag_id;
void barcode_pin_changed(uint32_t gpio, uint32_t edges);
void barcode_reading_task(INT start_code, void *task_context);
extern ID ir_snapshot_lock_id;

void ir_bench_task(INT start_code, void *task_context);
ER ir_get_snapshot(uint8_t gpio, IR_Snapshot *snapshot_out);

#endif /* SUBSYSTEM3_H */
