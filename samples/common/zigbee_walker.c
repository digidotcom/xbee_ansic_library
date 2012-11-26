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
	This sample demonstrates the use of ZDO and ZCL discovery to dump a list
	of all ZCL attributes (with their values) of all clusters of all endpoints
	of a device.
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "xbee/platform.h"
#include "xbee/device.h"
#include "xbee/atcmd.h"
#include "xbee/wpan.h"

#include "_zigbee_walker.h"

#include "parse_serial_args.h"

/*
	TODO

	Need a ZCL endpoint that we can change around as needed.  It should have
	a single cluster, based on the current cluster we're testing (client to
	server or server to client, correct profile ID).

*/

xbee_dev_t my_xbee;
addr64 target = { { 0 } };

void parse_args( int argc, char *argv[])
{
	int i;

	for (i = 1; i < argc; ++i)
	{
		if (strncmp( argv[i], "--mac=", 6) == 0)
		{
			if (addr64_parse( &target, &argv[i][6]))
			{
				fprintf( stderr, "ERROR: couldn't parse MAC %s\n", &argv[i][6]);
				exit( EXIT_FAILURE);
			}
		}
	}

	if (addr64_is_zero( &target))
	{
		fprintf( stderr, "ERROR: must pass address of target on command-line\n");
		fprintf( stderr, "\t(--mac=01:23:45:67:89:AB:CD:EF)\n");
		exit( EXIT_FAILURE);
	}
}

int main( int argc, char *argv[])
{
	int status;
	xbee_serial_t XBEE_SERPORT;

	// set serial port
	parse_serial_arguments( argc, argv, &XBEE_SERPORT);

	// parse args for this program
	parse_args( argc, argv);

	// initialize the serial and device layer for this XBee device
	if (xbee_dev_init( &my_xbee, &XBEE_SERPORT, NULL, NULL))
	{
		printf( "Failed to initialize XBee device.\n");
		return -1;
	}

	// Initialize the WPAN layer of the XBee device driver.  This layer enables
	// endpoints and clusters, and is required for all ZigBee layers.
	xbee_wpan_init( &my_xbee, &sample_endpoints.zdo);

	// Initialize the AT Command layer for this XBee device and have the
	// driver query it for basic information (hardware version, firmware version,
	// serial number, IEEE address, etc.)
	xbee_cmd_init_device( &my_xbee);
	do {
		xbee_dev_tick( &my_xbee);
		status = xbee_cmd_query_status( &my_xbee);
	} while (status == -EBUSY);
	if (status)
	{
		printf( "Error %d waiting for query to complete.\n", status);
		return -1;
	}

	walker_init( &my_xbee.wpan_dev, &target, WALKER_FLAGS_NONE);
	while (1)
   {
		wpan_tick( &my_xbee.wpan_dev);
		switch (walker_tick())
		{
			case WALKER_DONE:
				return 0;
			case WALKER_ERROR:
				return -1;
			default:
				break;		// continue processing
		}
   }
}

