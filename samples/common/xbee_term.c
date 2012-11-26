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
	Code for a stand-alone terminal emulator.  Look in
	samples/common/_xbee_term.c for xbee_term() and its support functions,
	and in samples/<platform>/xbee_term_<platform>.c for platform-specific
	console support functions.
*/
#include <stdio.h>
#include <stdlib.h>

#include "xbee/serial.h"

#include "_xbee_term.h"
#include "parse_serial_args.h"

int main( int argc, char *argv[])
{
	int retval;
	xbee_serial_t serport;

	parse_serial_arguments( argc, argv, &serport);

	retval = xbee_ser_open( &serport, serport.baudrate);
	if (retval != 0)
	{
		fprintf( stderr, "Error %d opening serial port\n", retval);
		return EXIT_FAILURE;
	}

	xbee_term( &serport);

	xbee_ser_close( &serport);

	return 0;
}

