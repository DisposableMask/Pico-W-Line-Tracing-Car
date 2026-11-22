#include "ir_internal.h"
#include <config_tm.h>

#define S3_PRI_MAIN 16

#if !TM_CONSOLE_UART
/* Confirm polarity with the installed sensors and tape. */
const ir_sensor_config_t ir_left = { 27, false };
const ir_sensor_config_t ir_right = { 0, false };
bool gpio_ready;
#endif

EXPORT ER subsystem3_init(void)
{
#if !TM_CONSOLE_UART
    ER err = gpio_set_pin(ir_left.gpio, GPIO_MODE_IN);
    if (err == E_OK) {
        err = gpio_set_pin(ir_right.gpio, GPIO_MODE_IN);
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
