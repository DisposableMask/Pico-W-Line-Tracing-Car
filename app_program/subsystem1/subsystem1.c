/*
 *	subsystem1.c
 *	Subsystem 1 -- LED heartbeat: creates its objects and starts its task.
 *
 *	APIs shown: tk_cre_cyc, tk_sta_cyc
 */

#include "subsystem1.h"

EXPORT ER subsystem1_init(void)
{
  tm_printf((const UB *)"c::subsystem1_init()");
	return E_OK;
}

EXPORT ER subsystem1_start(void)
{

  tm_printf((const UB *)"c::subsystem1_start()");
	return E_OK;
}
