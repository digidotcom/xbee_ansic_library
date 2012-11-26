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
	List all COM ports available via the inefficient method of trying to
	read the configuration of each one.  Defaults to checking COM1 to COM255;
	pass a number up to 4096 on the command line to check all ports up
	to that number.
*/

#include <stdio.h>
#include <windows.h>
#include <winbase.h>

#define MAX_COMPORT 4096

BOOL COM_exists( int port)
{
	char buffer[8];
	COMMCONFIG CommConfig;
	DWORD size;

	if (! (1 <= port && port <= MAX_COMPORT))
	{
		return FALSE;
	}

	snprintf( buffer, sizeof buffer, "COM%d", port);
	size = sizeof CommConfig;

	// COM port exists if GetDefaultCommConfig returns TRUE
	// or changes <size> to indicate COMMCONFIG buffer too small.
	return (GetDefaultCommConfig( buffer, &CommConfig, &size)
													|| size > sizeof CommConfig);
}

int main( int argc, char *argv[])
{
	int i, test_end;
	unsigned long temp;

	test_end = 255;
	if (argc > 1)
	{
		temp = strtoul( argv[1], NULL, 10);
		if (temp >= 1 && temp <= MAX_COMPORT)
		{
			test_end = (int) temp;
		}
	}

	printf( "Checking COM1 through COM%d:\n", test_end);

	for (i = 1; i <= test_end; ++i)
	{
		if (COM_exists( i))
		{
			printf( "COM%d exists\n", i);
		}
	}

	return 0;
}
