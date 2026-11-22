#include "ir_internal.h"
#include <config_tm.h>

#define S3_PRI_MAIN 16

#if !TM_CONSOLE_UART
ID event_id, mutex_id, cyclic_id;
#endif

EXPORT ER subsystem3_init(void)
{
#if !TM_CONSOLE_UART
    const T_CMTX mutex = { .mtxatr = TA_INHERIT };
    const T_CFLG flag = {0};
    /* Keep polling during worker bring-up. The GPIO IRQ replaces this timer. */
    const T_CCYC cyclic = { .cycatr = TA_HLNG, .cychdr = ir_tick, .cyctim = 10 };

    mutex_id = tk_cre_mtx(&mutex);
    if (mutex_id < E_OK) {
        return mutex_id;
    }
    event_id = tk_cre_flg(&flag);
    if (event_id < E_OK) {
        (void)tk_del_mtx(mutex_id);
        mutex_id = 0;
        return event_id;
    }
    cyclic_id = tk_cre_cyc(&cyclic);
    if (cyclic_id < E_OK) {
        (void)tk_del_flg(event_id);
        (void)tk_del_mtx(mutex_id);
        mutex_id = 0;
        return cyclic_id;
    }
#endif
    return E_OK;
}

EXPORT ER subsystem3_start(void)
{
#if TM_CONSOLE_UART
    return E_OK;
#else
    if (cyclic_id <= 0) {
        return E_OBJ;
    }
    ID tid = app_start_task(ir_worker, S3_PRI_MAIN, 0, "ir");
    return tid < E_OK ? tid : E_OK;
#endif
}
