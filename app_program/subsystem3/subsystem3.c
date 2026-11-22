#include "ir_internal.h"
#include <config_tm.h>

#define S3_PRI_MAIN 16

#if !TM_CONSOLE_UART
ID ir_event, ir_mutex;
#endif

EXPORT ER subsystem3_init(void)
{
#if !TM_CONSOLE_UART
    const T_CFLG flag = {0};
    const T_CMTX mutex = { .mtxatr = TA_INHERIT };

    ir_mutex = tk_cre_mtx(&mutex);
    if (ir_mutex < E_OK) {
        return ir_mutex;
    }
    ir_event = tk_cre_flg(&flag);
    if (ir_event < E_OK) {
        (void)tk_del_mtx(ir_mutex);
        ir_mutex = 0;
        return ir_event;
    }
#endif
    return E_OK;
}

EXPORT ER subsystem3_start(void)
{
#if TM_CONSOLE_UART
    tm_printf((const UB *)"[IR] disabled: right sensor GP0 is used by UART0\n");
    return E_OK;
#else
    if (ir_event <= 0) {
        return E_OBJ;
    }
    ID tid = app_start_task(ir_task, S3_PRI_MAIN, TP_PRC1, "ir");
    return tid < E_OK ? tid : E_OK;
#endif
}
