/*
 *	subsystem4.c
 *	Subsystem 4 -- Monitor: creates its objects and starts its task.
 *
 *	APIs shown: tk_cre_flg, tk_cre_mbf, tk_cre_alm
 */

#include "subsystem4.h"

EXPORT ER subsystem4_init(void)
{
  tm_printf((const UB *)"c::subsystem4_init()");
	return E_OK;
}

EXPORT ER subsystem4_start(void)
{
  tm_printf((const UB *)"c::subsystem4_start()");
	return E_OK;
}
