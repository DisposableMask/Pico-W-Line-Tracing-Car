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
  IR sensor;
  uint32_t timestamp;
} IR_Snapshot;

// Helper to compare struct sensors
static inline bool is_ir_equal(const IR A, const IR B) {
  return (A.gpio == B.gpio) && (A.is_digital == B.is_digital);
}


/* Add IR reading type and getter here once interface is ready */
ER ir_bench_start(void);

#endif /* SUBSYSTEM3_H */
