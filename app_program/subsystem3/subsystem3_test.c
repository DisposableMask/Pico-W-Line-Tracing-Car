#include "subsystem3.h"
#include "../test_report.h"

#if APP_TEST
EXPORT void subsystem3_test(void)
{
    TEST_INIT();
    ir_snapshot_t sample = {0};

    TEST_CHECK("S3-U01", ir_steering_decision(&sample) == IR_STEER_STOP,
               "invalid snapshot stops");
    sample.valid = true;
    TEST_CHECK("S3-U02", ir_steering_decision(&sample) == IR_STEER_STRAIGHT,
               "both white goes straight");
    sample.left_on_black = true;
    TEST_CHECK("S3-U03", ir_steering_decision(&sample) == IR_STEER_LEFT,
               "left black steers left");
    sample.right_on_black = true;
    TEST_CHECK("S3-U04", ir_steering_decision(&sample) == IR_STEER_STRAIGHT,
               "both black goes straight");
    sample.left_on_black = false;
    TEST_CHECK("S3-U05", ir_steering_decision(&sample) == IR_STEER_RIGHT,
               "right black steers right");
    TEST_SUMMARY("subsystem3");
}
#endif
