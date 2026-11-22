/*
 *	subsystem3.c
 *	Subsystem 3 -- Consumer: creates its objects and starts its task.
 *
 *	APIs shown: tk_cre_mbx, tk_cre_mtx
 */

#include "subsystem3.h"


EXPORT ER subsystem3_init(void)
{
  /* Set up IR driver and shared reading here */

  tm_printf((const UB *)"c::subsystem3_init()");
	return E_OK;
}

EXPORT ER subsystem3_start(void)
{
  // Temporary Success Log
  tm_printf((const UB *)"c::subsystem3_start()");

  return ir_bench_start();
}
