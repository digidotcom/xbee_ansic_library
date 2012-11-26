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

#include <stdio.h>

#include "xbee/platform.h"
#include "../unittest.h"

struct {
	char		*values;
	size_t	length;
	int		test_char;
	int		expected;
} test[] = {
	{ "\0\0\0\0\0X",					5, '\0', 0 },
	{ "\0\0\0\0\0X",					6, '\0', 1 },
	{ "AAAA",							4, 'A', 0 },
	{ "AAAA",							5, 'A', -1 },
	{ "AAAA",							4, 'B', -1 },
	{ "\xAA\xAA\xAA",					3, 0xAA, 0 },
	{ "\xAA\xAA\xAA",					3, 0xBB, -1 },
	{ "\xAA\xAA\xAA",					0, 0xBB, 0 },
	{ "\xAA\xAA\xAA",					4, 0xAA, -1 },
	{ "\x55\x55\x55",					3, 0x55, 0 },
	{ "\x55\x55\x55",					3, 0x66, -1 },
	{ "\x55\x55\x55",					0, 0x66, 0 },
	{ "\x55\x55\x55",					4, 0x55, -1 },
};

void t_memcheck( void)
{
	char error[80];
	int i, retval;
	bool_t result;

	for (i = 0; i < _TABLE_ENTRIES( test); ++i)
	{
		sprintf( error, "entry %u, testing for 0x%02X, expected %d", i,
				test[i].test_char, test[i].expected);
		retval = memcheck( test[i].values, test[i].test_char, test[i].length);
		switch (test[i].expected)
		{
			case -1:	result = (retval < 0);	break;
			case 0:	result = (retval == 0);	break;
			case +1:	result = (retval > 0);	break;
			default:	result = FALSE;			break;
		}
		test_bool( result, error);
	}

	test_bool( memcheck( "\xFF\xFF\xFF", 0xFF, 3) == 0, "0xFF check failed");
}

int main( int argc, char *argv[])
{
	int failures = 0;

	failures += DO_TEST( t_memcheck);

	return test_exit( failures);
}
