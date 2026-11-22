/*
 *	subsystem4_test.c
 *	On-target tests for subsystem 4.  Built only with: make TEST=1
 *	Record unit results in testing/subsystem4/TEST_LOG.md and the
 *	INT- result in testing/integration/TEST_LOG.md.
 */

#include "subsystem4.h"
#include "../test_report.h"

#if APP_TEST
EXPORT void subsystem4_test(void)
{
	TEST_INIT();

	/* Replace with real checks of this subsystem's behaviour */
	TEST_CHECK("S4-U01", tk_get_tid() > 0, "S4: subsystem4_test()");

	TEST_SUMMARY("subsystem4");
}
#endif /* APP_TEST */
