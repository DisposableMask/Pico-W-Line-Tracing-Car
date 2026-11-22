#ifndef SUBSYSTEM3_H
#define SUBSYSTEM3_H

#include <stdbool.h>
#include <stdint.h>
#include "../app.h"

typedef struct {
    uint8_t gpio;
    bool black_is_high;
} ir_sensor_config_t;

typedef struct {
    bool left_on_black;
    bool right_on_black;
    bool valid;
    uint32_t last_change_ms;
} ir_snapshot_t;

typedef enum {
    IR_STEER_STOP,
    IR_STEER_STRAIGHT,
    IR_STEER_LEFT,
    IR_STEER_RIGHT
} ir_steering_t;

ER ir_get_snapshot(ir_snapshot_t *out);
ir_steering_t ir_steering_decision(const ir_snapshot_t *snapshot);

#endif
