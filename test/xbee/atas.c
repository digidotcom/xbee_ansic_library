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

#include "../../samples/common/parse_serial_args.h"
#include "../../samples/common/_atinter.h"
#include "xbee/platform.h"
#include "xbee/byteorder.h"
#include "xbee/device.h"
#include "xbee/atcmd.h"
#include "wpan/types.h"

xbee_dev_t my_xbee;

typedef PACKED_STRUCT {
	uint8_t	as_type;
	uint8_t	channel;
	uint16_t	pan_be;
	addr64	extended_pan_be;
	uint8_t	allow_join;
	uint8_t	stack_profile;
	uint8_t	lqi;
	int8_t	rssi;
} xbee_atas_response_t;


int xbee_disc_atas_response( xbee_dev_t *xbee, const void FAR *raw,
	uint16_t length, void FAR *context)
{
	static const xbee_at_cmd_t as = { { 'A', 'S' } };
	const xbee_frame_local_at_resp_t FAR *resp = raw;
	const xbee_atas_response_t FAR *atas = (const void FAR *)resp->value;
	char buffer[ADDR64_STRING_LENGTH];

	if (resp->header.command.w == as.w)
	{
		if (XBEE_AT_RESP_STATUS( resp->header.status) == XBEE_AT_RESP_SUCCESS)
		{
			// this is a successful ATAS response
			printf( "CH%2u PAN:0x%04X %" PRIsFAR " J%u SP%u LQI:%3u RSSI:%d\n",
				atas->channel, be16toh( atas->pan_be),
				addr64_format( buffer, &atas->extended_pan_be),
				atas->allow_join, atas->stack_profile,
				atas->lqi, atas->rssi);
		}
		else
		{
			printf( "ATAS status 0x%02X\n", resp->header.status);
			hex_dump( resp->value,
				length - offsetof( xbee_frame_local_at_resp_t, value),
				HEX_DUMP_FLAG_OFFSET);
		}
	}

	return 0;
}

// Since we're not using a dynamic frame dispatch table, we need to define
// it here.
const xbee_dispatch_table_entry_t xbee_frame_handlers[] =
{
	XBEE_FRAME_HANDLE_LOCAL_AT,
	{ XBEE_FRAME_LOCAL_AT_RESPONSE, 0, xbee_disc_atas_response, NULL },
	XBEE_FRAME_TABLE_END
};

void print_menu( void)
{
	puts( "help                 This list of options.");
	puts( "as                   Initiate active scan.");
	puts( "");
}

void active_scan( void)
{
	puts( "Initiating active scan...");
	xbee_cmd_execute( &my_xbee, "AS", NULL, 0);
}

/*
	main

	Initiate communication with the XBee module, then accept AT commands from
	STDIO, pass them to the XBee module and print the result.
*/
int main( int argc, char *argv[])
{
   char cmdstr[80];
	int status;
	xbee_serial_t XBEE_SERPORT = { 115200, 0 };

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

	// Take responsibility for responding to certain ZDO requests.
	// This is necessary for other devices to discover our endpoints.
	xbee_cmd_simple( &my_xbee, "AO", 3);

	print_menu();
	active_scan();

   while (1)
   {
      while (xbee_readline( cmdstr, sizeof cmdstr) == -EAGAIN)
      {
      	xbee_dev_tick( &my_xbee);
      }

		if (! strcmpi( cmdstr, "help") || ! strcmp( cmdstr, "?"))
		{
			print_menu();
		}
		else if (! strcmpi( cmdstr, "as"))
      {
      	active_scan();
      }
      else if (! strcmpi( cmdstr, "quit"))
      {
			return 0;
		}
		else if (! strncmpi( cmdstr, "AT", 2))
		{
			process_command( &my_xbee, &cmdstr[2]);
		}
	   else
	   {
			printf( "unknown command: %s\n", cmdstr);
	   }
   }
}
