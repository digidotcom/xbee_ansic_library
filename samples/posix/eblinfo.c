/*
 * Copyright (c) 2012 Digi International Inc.,
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
	Displays information stored in the headers of XBee firmware images
	(.EBL files) used for XBee S2, S2B and S2C models.
 */
#include <errno.h>
#include <stdio.h>

#include "xbee/ebl_file.h"

int ebl_dump( const char *filename)
{
	FILE					*f;
	ebl_file_header_t	header;
	
	f = fopen( filename, "rb");
	if (f == NULL)
	{
		puts( "couldn't open file");
		return -EIO;
	}
	
	if (fread( &header, 1, sizeof header, f) != sizeof header)
	{
		puts( "read came up short");
	}
	else
	{
		ebl_header_dump( &header, EBL_HEADER_DUMP_EVERYTHING);
	}
	
	fclose( f);

	return 0;
}

int main( int argc, char *argv[])
{
	int i;
	
	for (i = 1; i < argc; ++i)
	{
		printf( "\n%s:\n", argv[i]);
		ebl_dump( argv[i]);
	}
	
	return 0;
}
