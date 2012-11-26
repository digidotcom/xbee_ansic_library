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

/** @file unittest.h
 */

/// Global set to 1 for verbose, detailed output, or 0 for no output.
extern int test_verbose;

/**
	@brief Run test \p testname, using "testname:" as the prefix for all test
			results, and printing a summary of the results.

	A typical unit test consists of multiple test functions and a main() to
	call them: @code
#include "unittest.h"

void t_sometest( void)
{
	int a, b, c;

	b = 3; c = 11;
	a = b * c;
	test_compare( a, 33, NULL, "integer multiply broken");
}

int main( int argc, char *argv[])
{
	int failures = 0;

	test_verbose = 0;

	failures += DO_TEST( t_sometest);

	return test_exit( failures);
}
@endcode

	@param[in]	testname		Test function to run.  The test function doesn't
					take any parameters and doesn't have a return value.  The test
					function reports the test results via test_fail(), test_warn(),
					test_info(), test_bool(), test_int() and test_compare().

	@return	Number of failures for a test.

	@sa	test_fail, test_warn, test_info, test_bool, test_int, test_compare
*/
#define DO_TEST(testname)	do_test( #testname, testname)
int do_test( const char *testname, void (*testfunc)());

/**
	@brief Called before a set of tests to reset counters and register the
				test name (used as a prefix on all output during the test).

	When done, call test_footer() to print the test results.  Or, use the
	DO_TEST() macro which automates calling test_header() and test_footer().

	@param[in]	name	name of test

	@sa		DO_TEST, test_footer
*/
void test_header( const char *name);

/**
	@brief	Print the test results and return the number of failures.

	@return	Number of failures encountered since calling test_header().

	@sa		DO_TEST, test_header
*/
int test_footer( void);

/**
	@brief	If test_verbose is not zero, prints a "warning" to STDOUT.

	Warning appears as "<testname>: WARN (<message>)".  Does not affect the
	test or failure count for the current test.

	@param[in]	message	warning message to print

	@sa		test_fail, test_info, test_bool, test_compare
*/
void test_warn( const char *message);

/**
	@brief	If test_verbose is not zero, prints an "info" message to STDOUT.

	Message appears as "<testname>: INFO (<message>)".  Does not affect the
	test or failure count for the current test.

	@param[in]	message	info message to print

	@sa		test_fail, test_warn, test_bool, test_compare
*/
void test_info( const char *str);

/**
	@brief	Log a test that has failed.

	If test_verbose is not zero, print the failure to STDOUT as
	" failure #x/test #y: <error>".

	@param[in]	error		Error message for test (should be unique so failure
								point can be located in unit test).

	@return		always returns -1

	@sa			test_pass, test_info, test_warn, test_bool, test_compare
*/
#define test_fail( error)		test_bool( 0, error)

/**
	@brief	Log a test that has passed.

	@return		always returns 0

	@sa			test_fail, test_info, test_warn, test_bool, test_compare
*/
#define test_pass()				test_bool( 1, "")

/**
	@brief	Increment the test counter and failure counter (if \p success
				is zero) for the current test.

	If \p success is zero, increment the failure counter and if the global
	\c test_verbose is not zero, print the failure to STDOUT as
	" failure #x/test #y: <error>".

	@param[in]	success	A non-zero value for success, zero for failure.
								Typically the result of a conditional (e.g.,
								var == 0, var > 12, etc.).

	@param[in]	error		Error message for test (should be unique so failure
								point can be located in unit test).

	@retval	0		test passed (\p success != 0)
	@retval	-1		test failed (\p success == 0)

	@sa	test_fail, test_info, test_warn, test_compare, test_address,
			test_string
*/
int test_bool( int success, const char *error);

/**
	@brief	Increment the test counter and failure counter (if two longs
				differ) for the current test.

	If \p result is not equal to \p expected, increment the failure counter
	and if the global \c test_verbose is not zero, print the failure to STDOUT
	as " failure #x/test #y: <error> (<result> != <expected>)".

	@param[in]	result	result to test

	@param[in]	expected	expected value for result

	@param[in]	fmt		A valid format string for a long value (e.g., 0x%06lx,
								%lu, %ld, etc.).  Defaults to %ld if \p fmt is NULL.

	@param[in]	error		Error message for test (should be unique so failure
								point can be located in unit test).

	@retval	0		test passed (\p success != 0)
	@retval	-1		test failed (\p success == 0)

	@sa	test_fail, test_info, test_warn, test_bool, test_address, test_string
*/
int test_compare( long result, long expected,
													const char *fmt, const char *error);

/**
	@brief	Increment the test counter and failure counter (if two pointers
				differ) for the current test.

	If \p result is not equal to \p expected, increment the failure counter
	and if the global \c test_verbose is not zero, print the failure to STDOUT
	as " failure #x/test #y: <error> (<result> != <expected>)".

	Note that this function simply casts the addresses to long and
	passes them to test_compare() using "0x%06lx" as the format.

	@param[in]	result	result to test

	@param[in]	expected	expected value for result

	@param[in]	error		Error message for test (should be unique so failure
								point can be located in unit test).

	@retval	0		test passed (\p success != 0)
	@retval	-1		test failed (\p success == 0)

	@sa	test_fail, test_info, test_warn, test_bool, test_compare, test_string
*/
int test_address( const void *result, const void *expected, const char *error);

/**
	@brief	Increment the test counter and failure counter (if two strings
				differ) for the current test.

	If \p result differs from \p expected, increment the failure counter
	and if the global \c test_verbose is not zero, print the failure to STDOUT
	as " failure #x/test #y: <error> ([<result>] != [<expected>])".

	Note that this function simply casts the addresses to long and
	passes them to test_compare() using "0x%06lx" as the format.

	@param[in]	result	result to test

	@param[in]	expected	expected value for result

	@param[in]	error		Error message for test (should be unique so failure
								point can be located in unit test).

	@retval	0		test passed (\p success != 0)
	@retval	-1		test failed (\p success == 0)

	@sa	test_fail, test_info, test_warn, test_bool, test_compare, test_address
*/
int test_string( const char *result, const char *expected, const char *error);

/**
	@brief	Print a summary of tests if test_verbose is set.

	@param[in]	failures		Number of failures during all rounds of testing.

	@retval	0		all tests passed (\p failures == 0)
	@retval	-1		one or more tests failed (\p failures != 0)

	@sa		test_header, test_footer
*/
int test_exit( int failures);
