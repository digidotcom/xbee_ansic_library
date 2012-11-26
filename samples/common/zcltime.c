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
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

// Win32 epoch is 1/1/1970, add 30 years, plus 7 leap days (1972, 1976, 1980,
// 1984, 1988, 1992, 1996) to get to ZigBee epoch of 1/1/2000.
#define ZCL_TIME_EPOCH_DELTA	(((uint32_t)30 * 365 + 7) * 24 * 60 * 60)

int main( int argc, char *argv[])
{
	time_t rawtime;
	char *tail;
	struct tm *timeinfo;

	if (argc != 2)
	{
		printf( "usage: %s <time>\n", argv[0]);
		printf( "\tWhere <time> is a ZCL UTCTime value in hex (0x prefix)\n");
		printf( "\tor decimal (no prefix).\n");

		return 0;
	}

	rawtime = (time_t) strtoul( argv[1], &tail, 0);
	if (! tail)
	{
		printf( "Error trying to convert '%s'.\n", argv[1]);
		return -1;
	}

	rawtime += ZCL_TIME_EPOCH_DELTA;
	timeinfo = gmtime( &rawtime);
	printf( "  ZCL = '%s'\n  UTC = %s", argv[1], asctime( timeinfo));
	timeinfo = localtime( &rawtime);
	printf( "local = %s", asctime( timeinfo));

	return 0;
}
