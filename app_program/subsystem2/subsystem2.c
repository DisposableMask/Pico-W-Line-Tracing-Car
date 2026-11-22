/*
 *	subsystem2.c
 *	Subsystem 2 -- Producer: creates its objects and starts its task.
 *
 *	APIs shown: tk_cre_sem, tk_cre_mpf
 */

#include "subsystem2.h"

EXPORT ER subsystem2_init(void)
{
  tm_printf((const UB *)"c::subsystem2_init()");
	return E_OK;
}

EXPORT ER subsystem2_start(void)
{
  tm_printf((const UB *)"c::subsystem2_start()");
	return E_OK;
}
