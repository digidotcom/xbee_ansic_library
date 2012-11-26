
/*
	This program tests our support of 64-bit integers.  Use the
	win32/jsll_gen.c program to generate a series of test vectors (stored
	in jsll_vectors.h) using native 64-bit variables.

	Then, compile this test on platforms that must use the non-native
	structure.


*/

#include <stdio.h>
#include <string.h>

#include "xbee/platform.h"
#include "../unittest.h"

// include the pre-computed test vectors
#include "jsll_vectors.h"

#define BINARY_TEST(m, el)	\
	do { sprintf( errstr, "%s failed on record %u", #m, i);			\
	m( c, a, b); test_bool( JSLL_EQ( c, rec->el), errstr); } while (0)
#define SHIFT_TEST(m, el, bits)	\
	do { sprintf( errstr, "%s failed on record %u", #m, i);			\
	m( c, a, bits); test_bool( JSLL_EQ( c, rec->el), errstr); } while (0)
#define UNARY_TEST(m, el)	\
	do { sprintf( errstr, "%s failed on record %u", #m, i);			\
	m( c, a); test_bool( JSLL_EQ( c, rec->el), errstr); } while (0)
#define CP_TEST(m, op, el)	\
	do { sprintf( errstr, "%s(%s) failed on record %u", #m, #op, i);	\
	test_bool( m( a, op, b) == rec->el, errstr); } while (0)
#define STRING_TEST( m, el) \
	do { memset( buffer, 0, sizeof buffer); m( buffer, a);			\
		sprintf( errstr, "%s failed on record %u", #m, i);				\
		test_string( buffer, rec->el, errstr); } while (0)

void t_jslong_shift( void)
{
	const shifttest_t *test;
	const shiftrec_t *rec;
	char errstr[60];
	uint_fast8_t i, sh;
	JSUint64 a, c;

	test = shift_tests;
	for (i = 0; i < _TABLE_ENTRIES(shift_tests); ++test, ++i)
	{
		a = test->base;
		for (rec = test->shift, sh = 1; sh < 64; ++rec, ++sh)
		{
#ifdef __XBEE_PLATFORM_HCS08
			// reset watchdog timer
			{asm sta SRS;}
#endif
			SHIFT_TEST( JSLL_SHL, shl, sh);
			SHIFT_TEST( JSLL_SHR, shr, sh);
			SHIFT_TEST( JSLL_USHR, ushr, sh);
		}
	}
}

void t_jslong( void)
{
	int i;
	char errstr[60];
	char buffer[25];
	const testrec_t *rec;
	JSInt64 a, b, c;
	int32_t i32;
	uint32_t ui32;

	for (rec = tests, i = 0; i < _TABLE_ENTRIES(tests); ++rec, ++i)
	{
		a = rec->a;
		b = rec->b;

#ifdef __XBEE_PLATFORM_HCS08
		// reset watchdog timer
		{asm sta SRS;}
#endif

		BINARY_TEST( JSLL_ADD, add);
		BINARY_TEST( JSLL_SUB, sub);
		BINARY_TEST( JSLL_MUL, mul);
		BINARY_TEST( JSLL_AND, and);
		BINARY_TEST( JSLL_OR, or);
		BINARY_TEST( JSLL_XOR, xor);

		UNARY_TEST( JSLL_NOT, not);
		UNARY_TEST( JSLL_NEG, neg);

		SHIFT_TEST( JSLL_SHL, shl, SHIFT_BITS);
		SHIFT_TEST( JSLL_SHR, shr, SHIFT_BITS);
		SHIFT_TEST( JSLL_USHR, ushr, SHIFT_BITS);

		JSLL_L2I( i32, a);
		sprintf( errstr, "%s failed on record %u", "JSLL_L2I", i);
		if (! test_compare( i32, rec->l2i, "0x%08" PRIx32, errstr))
		{
			JSLL_I2L( c, i32);
			sprintf( errstr, "%s failed on record %u", "JSLL_I2L", i);
			test_bool( JSLL_EQ( c, rec->i2l), errstr);
		}

		JSLL_L2UI( ui32, a);
		sprintf( errstr, "%s failed on record %u", "JSLL_L2UI", i);
		if (! test_compare( ui32, rec->l2ui, "0x%08" PRIx32, errstr))
		{
			JSLL_UI2L( c, ui32);
			sprintf( errstr, "%s failed on record %u", "JSLL_UI2L", i);
			test_bool( JSLL_EQ( c, rec->ui2l), errstr);
		}

		STRING_TEST( JSLL_HEXSTR, hexstr);
		STRING_TEST( JSLL_DECSTR, decstr);
		STRING_TEST( JSLL_UDECSTR, udecstr);

		CP_TEST( JSLL_UCMP, >, gtu);
		CP_TEST( JSLL_UCMP, <, ltu);
		CP_TEST( JSLL_CMP, >, gt);
		CP_TEST( JSLL_CMP, <, lt);
	}
}

int t_all( void)
{
	int failures = 0;

	failures += DO_TEST( t_jslong);
	failures += DO_TEST( t_jslong_shift);

	return test_exit( failures);
}
