#include "ir_internal.h"
#include <sys/machine.h>
#include <config_tm.h>

#define IR_CHANGED 1u

#if !TM_CONSOLE_UART
#define IO_IRQ_BANK0 13u
#define IO_BANK0_INTR(group)       (IO_BANK0_BASE + 0xf0u + 4u * (group))
#define IO_BANK0_PROC0_INTE(group) (IO_BANK0_BASE + 0x100u + 4u * (group))
#define IO_BANK0_PROC0_INTS(group) (IO_BANK0_BASE + 0x120u + 4u * (group))
#define GPIO_EDGE_BOTH 0xcu

/* Verify comparator polarity on the chassis before enabling motors. */
static const ir_sensor_config_t sensors[] = {
    { .gpio = 27, .black_is_high = false }, /* left */
    { .gpio = 0,  .black_is_high = false }  /* right */
};

static ir_snapshot_t latest;

static UW edge_mask(UINT gpio)
{
    return GPIO_EDGE_BOTH << ((gpio % 8u) * 4u);
}

static void gpio_bank_handler(UINT intno)
{
    bool changed = false;
    (void)intno;

    for (UINT i = 0; i < sizeof(sensors) / sizeof(sensors[0]); ++i) {
        UINT gpio = sensors[i].gpio;
        UW pending = in_w(IO_BANK0_PROC0_INTS(gpio / 8u)) & edge_mask(gpio);
        if (pending != 0u) {
            out_w(IO_BANK0_INTR(gpio / 8u), pending);
            changed = true;
        }
    }
    if (changed) {
        (void)tk_set_flg(ir_event, IR_CHANGED);
    }
}

void ir_task(INT stacd, void *exinf)
{
    (void)stacd;
    (void)exinf;

    for (UINT i = 0; i < sizeof(sensors) / sizeof(sensors[0]); ++i) {
        ER err = gpio_set_pin(sensors[i].gpio, GPIO_MODE_IN);
        if (err < E_OK) {
            tm_printf((const UB *)"[IR] GPIO setup failed (%d)\n", (INT)err);
            tk_ext_tsk();
            return;
        }
    }

    const T_DINT irq = { .intatr = TA_HLNG, .inthdr = gpio_bank_handler };
    ER err = tk_def_int(IO_IRQ_BANK0, &irq);
    if (err < E_OK) {
        tm_printf((const UB *)"[IR] IRQ setup failed (%d)\n", (INT)err);
        tk_ext_tsk();
        return;
    }

    for (UINT i = 0; i < sizeof(sensors) / sizeof(sensors[0]); ++i) {
        UINT gpio = sensors[i].gpio;
        out_w(IO_BANK0_INTR(gpio / 8u), edge_mask(gpio));
        set_w(IO_BANK0_PROC0_INTE(gpio / 8u), edge_mask(gpio));
    }
    EnableInt(IO_IRQ_BANK0, 2);
    (void)tk_set_flg(ir_event, IR_CHANGED);

    for (;;) {
        UINT flags;
        if (tk_wai_flg(ir_event, IR_CHANGED, TWF_ORW | TWF_BITCLR,
                       &flags, TMO_FEVR) != E_OK) {
            continue;
        }

        UW levels = in_w(GPIO_IN);
        bool left = ((levels & (1u << sensors[0].gpio)) != 0u) == sensors[0].black_is_high;
        bool right = ((levels & (1u << sensors[1].gpio)) != 0u) == sensors[1].black_is_high;

        if (tk_loc_mtx(ir_mutex, TMO_FEVR) != E_OK) {
            continue;
        }
        bool changed = !latest.valid || left != latest.left_on_black ||
                       right != latest.right_on_black;
        if (changed) {
            SYSTIM now;
            uint32_t timestamp = latest.last_change_ms;
            if (tk_get_otm(&now) == E_OK) {
                timestamp = (uint32_t)now.lo;
            }
            latest = (ir_snapshot_t){ left, right, true, timestamp };
        }
        (void)tk_unl_mtx(ir_mutex);

#if APP_TEST
        if (changed) {
            tm_printf((const UB *)"[IR] left_black=%u right_black=%u\n",
                      (UINT)left, (UINT)right);
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
    if (ir_mutex <= 0) {
        return E_OBJ;
    }
    ER err = tk_loc_mtx(ir_mutex, TMO_FEVR);
    if (err != E_OK) {
        return err;
    }
    *out = latest;
    err = tk_unl_mtx(ir_mutex);
    if (err != E_OK) {
        out->valid = false;
    }
    return err;
#endif
}
