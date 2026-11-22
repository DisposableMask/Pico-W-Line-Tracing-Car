#include "subsystem3.h"

ER ir_init(void);
ER ir_start(void);

EXPORT ER subsystem3_init(void)
{
    return ir_init();
}

EXPORT ER subsystem3_start(void)
{
    return ir_start();
}
