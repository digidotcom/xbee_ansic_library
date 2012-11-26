#include <stdio.h>
#include "xbee/platform.h"
#include "../unittest.h"

#define run_test( a, op, b) \
	test_bool( XBEE_TIMER_COMPARE( a, op, b), #a " " #op " " #b)
void test( void)
{
	run_test( 0xFFFF, <, 0);
	run_test( 0, <, 1);
	run_test( 1, >, 0);

	run_test( 0, <, 0x7FFF);
	run_test( 0x7FFF, >, 0);

	// don't test the boundary case of a difference of 0x8000 between the two
	// values, since 0 > 0x8000 will be false, but 0x8000 < 0 will be true.

	run_test( 0, >, 0x8001);
	run_test( 0x8001, <, 0);

	run_test( 100, ==, 100);

	// test 0x7FFF elapsed (no rollover)
	run_test( 1234, <, 1234 + 0x7FFF);
	run_test( 1234 + 0x7FFF, >, 1234);

	// text 0x8001 elasped (rollover)
	run_test( 1234, >, 1234 + 0x8001);
	run_test( 1234 + 0x8001, <, 1234);
}

int main( int argc, char *argv[])
{
	int failures = 0;

	failures += DO_TEST( test);

	return test_exit( failures);
}
