#include "ir_internal.h"
#include <config_tm.h>
#include <sys/machine.h>


ER ir_get_snapshot(ir_snapshot_t *out)
{
    if (out == NULL) {
        return E_PAR;
    }
    *out = (ir_snapshot_t){0};
#if !TM_CONSOLE_UART
    if (gpio_ready) {
        UW levels = in_w(GPIO_IN);
        out->left_on_black = ((levels & (1u << ir_left.gpio)) != 0u) == ir_left.black_is_high;
        out->right_on_black = ((levels & (1u << ir_right.gpio)) != 0u) == ir_right.black_is_high;
        out->valid = true;
    }
#endif
    return E_OK;
}

#if !TM_CONSOLE_UART
void ir_bench_task(INT stacd, void *exinf)
{
    (void)stacd;
    (void)exinf;

    for (;;) {
        ir_snapshot_t sample;
        if (ir_get_snapshot(&sample) == E_OK && sample.valid) {
            tm_printf((const UB *)"[IR] left_black=%u right_black=%u\n",
                      (UINT)sample.left_on_black, (UINT)sample.right_on_black);
        }
        tk_dly_tsk(200);
    }
}
#endif
