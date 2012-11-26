/*
 * Copyright (c) 2007-2012 Digi International Inc.,
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
#include "_atinter.h"
#include "parse_serial_args.h"

xbee_dev_t my_xbee;

/*
	main

	Initiate communication with the XBee module, then accept AT commands from
	STDIO, pass them to the XBee module and print the result.
*/
int main( int argc, char *argv[])
{
   char cmdstr[80];
	int status;
	xbee_serial_t XBEE_SERPORT;

	parse_serial_arguments( argc, argv, &XBEE_SERPORT);

	// initialize the serial and device layer for this XBee device
	if (xbee_dev_init( &my_xbee, &XBEE_SERPORT, NULL, NULL))
	{
		printf( "Failed to initialize device.\n");
		return 0;
	}

	// Initialize the AT Command layer for this XBee device and have the
	// driver query it for basic information (hardware version, firmware version,
	// serial number, IEEE address, etc.)
	xbee_cmd_init_device( &my_xbee);
	printf( "Waiting for driver to query the XBee device...\n");
	do {
		xbee_dev_tick( &my_xbee);
		status = xbee_cmd_query_status( &my_xbee);
	} while (status == -EBUSY);
	if (status)
	{
		printf( "Error %d waiting for query to complete.\n", status);
	}

	// report on the settings
	xbee_dev_dump_settings( &my_xbee, XBEE_DEV_DUMP_FLAG_DEFAULT);

   printATCmds( &my_xbee);

   while (1)
   {
      while (xbee_readline( cmdstr, sizeof cmdstr) == -EAGAIN)
      {
      	xbee_dev_tick( &my_xbee);
      }

		if (! strncmpi( cmdstr, "menu", 4))
      {
      	printATCmds( &my_xbee);
      }
      else if (! strcmpi( cmdstr, "quit"))
      {
			return 0;
		}
      else
      {
			process_command( &my_xbee, cmdstr);
	   }
   }
}

// Since we're not using a dynamic frame dispatch table, we need to define
// it here.
#include "xbee/atcmd.h"			// for XBEE_FRAME_HANDLE_LOCAL_AT
#include "xbee/device.h"		// for XBEE_FRAME_HANDLE_TX_STATUS
const xbee_dispatch_table_entry_t xbee_frame_handlers[] =
{
	XBEE_FRAME_HANDLE_LOCAL_AT,
	XBEE_FRAME_MODEM_STATUS_DEBUG,
	XBEE_FRAME_TABLE_END
};

