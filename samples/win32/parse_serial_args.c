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
	Common code used by Win32 samples to extract serial port settings from
	the command-line arguments passed into the program.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xbee/serial.h"
#include "parse_serial_args.h"

/*
	Parse the command-line arguments, looking for "COMx" to determine the
	serial port to use, and a bare number (assumed to be the baud rate).

	@param[in]	argc		argument count
	@param[in]	argv		array of \a argc arguments
	@param[out]	serial	serial port settings
*/
void parse_serial_arguments( int argc, char *argv[], xbee_serial_t *serial)
{
	int i;
	uint32_t baud;
	char buffer[80];

	memset( serial, 0, sizeof *serial);

	// default baud rate
	serial->baudrate = 115200;

	for (i = 1; i < argc; ++i)
	{
		if (strncmp( argv[i], "COM", 3) == 0)
		{
			serial->comport = (int) strtoul( argv[i] + 3, NULL, 10);
		}
		if ( (baud = (uint32_t) strtoul( argv[i], NULL, 0)) > 0)
		{
			serial->baudrate = baud;
		}
	}

	while (serial->comport == 0)
	{
		printf( "Connect to which COMPORT? ");
		fgets( buffer, sizeof buffer, stdin);
		serial->comport = (int) strtoul( buffer, NULL, 10);
	}
}

