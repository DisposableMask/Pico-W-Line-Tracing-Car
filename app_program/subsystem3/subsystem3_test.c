/*
 *	subsystem3_test.c
 *	On-target tests for subsystem 3.  Built only with: make TEST=1
 *	Record unit results in testing/subsystem3/TEST_LOG.md and the
 *	INT- result in testing/integration/TEST_LOG.md.
 */

#include "subsystem3.h"
#include "../robot_hal.h"
#include "../test_report.h"

#if APP_TEST
EXPORT void subsystem3_test(void)
{
  tk_dly_tsk(5000);
  /* Add IR threshold and hysteresis tests here */
  tm_printf((const UB *)"c::subsystem3_test()");

  tk_dly_tsk(5000);
	TEST_INIT();

	/* Replace with real checks of this subsystem's behaviour */
	TEST_CHECK("S3-U01", tk_get_tid() > 0, "S3: subsystem3_test()");

  tm_printf((const UB *)"[Barcode test] Move tape onto and off the barcode sensor.\n");

  // Wait until the callback has reported one rising and one falling edge.
  UINT received_edges;
  const UINT both_edges = HAL_GPIO_EDGE_RISE | HAL_GPIO_EDGE_FALL;
  const ER edge_result = tk_wai_flg(barcode_edge_flag_id, both_edges,
                                    TWF_ANDW | TWF_BITCLR, &received_edges, 10000);
  TEST_CHECK("S3-U02", edge_result == E_OK, "Barcode GPIO reported both edges");

	TEST_SUMMARY("subsystem3");
}
#endif /* APP_TEST */
