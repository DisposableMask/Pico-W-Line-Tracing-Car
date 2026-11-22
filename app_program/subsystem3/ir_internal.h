#ifndef IR_INTERNAL_H
#define IR_INTERNAL_H

#include "subsystem3.h"

extern const ir_sensor_config_t ir_left, ir_right;
extern bool gpio_ready;
void ir_bench_task(INT stacd, void *exinf);

#endif
