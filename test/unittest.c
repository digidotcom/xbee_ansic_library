/*
 * Copyright (c) 2010-2012 Digi International Inc.,
 * All rights not expressly granted are reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
 * =======================================================================
 */

/*
	unittest.c

	See unittest.h for usage summary and Doxygen-style documentation.
*/

#include <string.h>
#include <stdio.h>

#include "unittest.h"

int test_verbose = 1;

int do_test( const char *testname, void (*testfunc)( void))
{
	test_header( testname);
	testfunc();
	return test_footer();
}

typedef struct {
	int			failures;
	int			tests;
	const char	*testname;
} test_result_t;
test_result_t _test_results;

void test_header( const char *name)
{
	_test_results.failures = _test_results.tests = 0;
	_test_results.testname = name;
	if (test_verbose)
	{
		printf( "\n%s: START\n", _test_results.testname);
	}
}

int test_footer( void)
{
	if (test_verbose)
	{
		if (_test_results.failures)
		{
	      printf( "%s: FAILED (%d of %d tests)\n",
	      	_test_results.testname, _test_results.failures,
	      	_test_results.tests);
		}
		else
		{
	      printf( "%s: PASSED (%d tests)\n", _test_results.testname,
	      	_test_results.tests);
		}
	}

	return _test_results.failures;
}

void test_warn( const char *message)
{
	if (test_verbose)
	{
		printf( "%s: %s (%s)\n", _test_results.testname, "WARN", message);
	}
}

void test_info( const char *message)
{
	if (test_verbose)
	{
		printf( "%s: %s (%s)\n", _test_results.testname, "INFO", message);
	}
}

int test_bool( int success, const char *error)
{
	_test_results.tests++;
	if (success)
	{
		return 0;
	}

   _test_results.failures++;
	if (test_verbose)
	{
      printf( " failure #%d/test #%d: %s\n", _test_results.failures,
      	_test_results.tests, error);
   }

   return -1;
}

int test_compare( long result, long expected,
														const char *fmt, const char *error)
{
	char format_str[80];

	_test_results.tests++;
	if (result == expected)
	{
		return 0;
	}

   _test_results.failures++;
	if (test_verbose)
	{
		if (! fmt)
		{
			fmt = "%ld";
		}

   	sprintf( format_str, " failure #%d/test #%d: %%s (%s != %s)\n",
			_test_results.failures, _test_results.tests, fmt, fmt);
      printf( format_str, error, result, expected);
   }

   return -1;
}

int test_address( const void *result, const void *expected, const char *error)
{
	return test_compare( (long) result, (long) expected, "0x%06lx", error);
}

int test_string( const char *result, const char *expected, const char *error)
{
	_test_results.tests++;
	if (result == NULL)
	{
		if (expected == NULL)
		{
			return 0;					// if both are NULL, the test passes
		}
	}
	else if (expected != NULL && strcmp( result, expected) == 0)
	{
		return 0;
	}

   _test_results.failures++;
	if (test_verbose)
	{
      printf( " failure #%d/test #%d: %s ([%s] != [%s])\n",
      	_test_results.failures, _test_results.tests, error,
      	(result == NULL) ? "(null)" : result,
      	(expected == NULL) ? "(null)" : expected);
   }

   return -1;
}

int test_exit( int failures)
{
	if (test_verbose)
	{
		putchar( '\n');
		puts( failures ? "FAILED" : "PASSED");
	}
	return failures ? -1 : 0;
}
