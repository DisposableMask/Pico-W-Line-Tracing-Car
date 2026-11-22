#include "ir_internal.h"
#include <config_tm.h>
#include <sys/machine.h>

#define IR_CHANGED 1u

#if !TM_CONSOLE_UART
/* Confirm polarity with the installed sensors and tape. */
static const ir_sensor_config_t left = { 27, false };
static const ir_sensor_config_t right = { 0, false };
static ir_snapshot_t latest;

void ir_tick(void *exinf)
{
    (void)exinf;
    (void)tk_set_flg(event_id, IR_CHANGED);
}

void ir_worker(INT stacd, void *exinf)
{
    (void)stacd;
    (void)exinf;

    ER err = gpio_set_pin(left.gpio, GPIO_MODE_IN);
    if (err == E_OK) {
        err = gpio_set_pin(right.gpio, GPIO_MODE_IN);
    }
    if (err == E_OK) {
        err = tk_set_flg(event_id, IR_CHANGED);
    }
    if (err == E_OK) {
        err = tk_sta_cyc(cyclic_id);
    }
    if (err != E_OK) {
        tm_printf((const UB *)"[IR] setup failed (%d)\n", (INT)err);
        tk_ext_tsk();
        return;
    }

    for (;;) {
        UINT flags;
        if (tk_wai_flg(event_id, IR_CHANGED, TWF_ORW | TWF_BITCLR,
                       &flags, TMO_FEVR) != E_OK) {
            continue;
        }
        UW levels = in_w(GPIO_IN);
        bool left_black = ((levels & (1u << left.gpio)) != 0u) == left.black_is_high;
        bool right_black = ((levels & (1u << right.gpio)) != 0u) == right.black_is_high;

        if (tk_loc_mtx(mutex_id, TMO_FEVR) != E_OK) {
            continue;
        }
        bool changed = !latest.valid || latest.left_on_black != left_black ||
                       latest.right_on_black != right_black;
        if (changed) {
            SYSTIM now;
            if (tk_get_otm(&now) == E_OK) {
                latest.last_change_ms = (uint32_t)now.lo;
            }
            latest.left_on_black = left_black;
            latest.right_on_black = right_black;
            latest.valid = true;
        }
        (void)tk_unl_mtx(mutex_id);
#if APP_TEST
        if (changed) {
            tm_printf((const UB *)"[IR] left_black=%u right_black=%u\n",
                      (UINT)left_black, (UINT)right_black);
        }
#endif
    }
}
#endif

ER ir_get_snapshot(ir_snapshot_t *out)
{
    if (out == NULL) {
        return E_PAR;
    }
    *out = (ir_snapshot_t){0};
#if TM_CONSOLE_UART
    return E_OK;
#else
    if (mutex_id <= 0) {
        return E_OBJ;
    }
    ER err = tk_loc_mtx(mutex_id, TMO_FEVR);
    if (err != E_OK) {
        return err;
    }
    *out = latest;
    err = tk_unl_mtx(mutex_id);
    if (err != E_OK) {
        out->valid = false;
    }
    return err;
#endif
}
