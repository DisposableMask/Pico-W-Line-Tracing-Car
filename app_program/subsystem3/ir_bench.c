#include "subsystem3.h"
#include <config_tm.h>
#include <sys/machine.h>

#define S3_PRI_MAIN 16

#if !TM_CONSOLE_UART
/* Confirm polarity with the installed sensors and tape. */
static const ir_sensor_config_t left = { 27, false };
static const ir_sensor_config_t right = { 0, false };
static bool gpio_ready;
#endif

ER ir_get_snapshot(ir_snapshot_t *out)
{
    if (out == NULL) {
        return E_PAR;
    }
    *out = (ir_snapshot_t){0};
#if !TM_CONSOLE_UART
    if (gpio_ready) {
        UW levels = in_w(GPIO_IN);
        out->left_on_black = ((levels & (1u << left.gpio)) != 0u) == left.black_is_high;
        out->right_on_black = ((levels & (1u << right.gpio)) != 0u) == right.black_is_high;
        out->valid = true;
    }
#endif
    return E_OK;
}

#if !TM_CONSOLE_UART
static void ir_bench_task(INT stacd, void *exinf)
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

EXPORT ER subsystem3_init(void)
{
#if !TM_CONSOLE_UART
    ER err = gpio_set_pin(left.gpio, GPIO_MODE_IN);
    if (err == E_OK) {
        err = gpio_set_pin(right.gpio, GPIO_MODE_IN);
    }
    gpio_ready = err == E_OK;
    return err;
#else
    return E_OK;
#endif
}

EXPORT ER subsystem3_start(void)
{
#if !TM_CONSOLE_UART
    if (!gpio_ready) {
        return E_OBJ;
    }
    ID tid = app_start_task(ir_bench_task, S3_PRI_MAIN, 0, "ir_bench");
    return tid < E_OK ? tid : E_OK;
#else
    return E_OK;
#endif
}
