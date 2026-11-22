#include "subsystem3.h"
#include <sys/machine.h>

#define IR_PIN  27u

static void ir_bench_task(INT stacd, void *exinf)
{
    (void)stacd;
    (void)exinf;

    out_w(GPIO_OE_CLR, (1u << IR_PIN));
    out_w(GPIO_CTRL(IR_PIN), GPIO_CTRL_FUNCSEL_SIO);
    set_w(GPIO(IR_PIN), GPIO_IE);

    for (;;) {
        UW levels = in_w(GPIO_IN);
        tm_printf((const UB *)"led value=%u\n",
                  (UINT)((levels >> IR_PIN) & 1u));
        tk_dly_tsk(200);
    }
}

ER ir_bench_start(void)
{
    ID tid = app_start_task(ir_bench_task, S3_PRI_MAIN, 0, "ir_bench");
    return tid < E_OK ? tid : E_OK;
}
