/*
 *	subsystem3_test.c
 *	On-target tests for subsystem 3.  Built only with: make TEST=1
 *	Record unit results in testing/subsystem3/TEST_LOG.md and the
 *	INT- result in testing/integration/TEST_LOG.md.
 */

#include "subsystem3.h"
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

  tk_dly_tsk(5000);
  tm_printf((const UB *)"[IR test] Move tape under one sensor now.\n");
  tk_dly_tsk(5000);

  // Ask the worker to read both pins again after the tape moves.
  TEST_CHECK("S3-U02", sample_request_result == E_OK,
              "IR sample request sent");

	TEST_SUMMARY("subsystem3");
}
#endif /* APP_TEST */
