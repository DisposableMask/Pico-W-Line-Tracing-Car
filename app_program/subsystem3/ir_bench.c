#include "subsystem3.h"
#include <sys/machine.h>

#define IR_LEFT_PIN  27u
#define IR_RIGHT_PIN 0u

static void ir_bench_task(INT stacd, void *exinf)
{
    (void)stacd;
    (void)exinf;

    out_w(GPIO_OE_CLR, (1u << IR_LEFT_PIN) | (1u << IR_RIGHT_PIN));
    out_w(GPIO_CTRL(IR_LEFT_PIN), GPIO_CTRL_FUNCSEL_SIO);
    out_w(GPIO_CTRL(IR_RIGHT_PIN), GPIO_CTRL_FUNCSEL_SIO);
    set_w(GPIO(IR_LEFT_PIN), GPIO_IE);
    set_w(GPIO(IR_RIGHT_PIN), GPIO_IE);

    for (;;) {
        UW levels = in_w(GPIO_IN);
        tm_printf((const UB *)"left=%u right=%u\n",
                  (UINT)((levels >> IR_LEFT_PIN) & 1u),
                  (UINT)((levels >> IR_RIGHT_PIN) & 1u));
        tk_dly_tsk(10);
    }
}

ER ir_bench_start(void)
{
    ID tid = app_start_task(ir_bench_task, S3_PRI_MAIN, 0, "ir_bench");
    return tid < E_OK ? tid : E_OK;
}
