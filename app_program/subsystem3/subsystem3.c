#include "subsystem3.h"

ir_steering_t ir_steering_decision(const ir_snapshot_t *snapshot)
{
    if (snapshot == NULL || !snapshot->valid) {
        return IR_STEER_STOP;
    }
    if (snapshot->left_on_black == snapshot->right_on_black) {
        return IR_STEER_STRAIGHT;
    }
    return snapshot->left_on_black ? IR_STEER_LEFT : IR_STEER_RIGHT;
}
