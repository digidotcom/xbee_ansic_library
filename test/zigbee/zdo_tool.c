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
#include <string.h>

#include "xbee/platform.h"
#include "xbee/wpan.h"
#include "wpan/aps.h"
#include "zigbee/zdo.h"

#include "../../samples/common/_atinter.h"
#include "../../samples/common/parse_serial_args.h"

xbee_dev_t my_xbee;

/// Used to track ZDO transactions in order to match responses to requests
/// (#ZDO_MATCH_DESC_RESP).
wpan_ep_state_t zdo_ep_state = { 0 };
struct {
	wpan_endpoint_table_entry_t		zdo;
	uint8_t									end_of_list;
} sample_endpoints = {
	ZDO_ENDPOINT(zdo_ep_state),
	WPAN_ENDPOINT_END_OF_LIST
};

void print_help( void)
{
	puts( "ZDO Tool Help:");
	puts( "\tieee HHHHHHHH-HHHHHHHH - send IEEE_addr request");
	puts( "\tnet 0xHHHH             - send NWK_addr request");
	puts( "");
	puts( "\tquit           - quit program");
}

/*
	main

	Initiate communication with the XBee module, then accept AT commands from
	STDIO, pass them to the XBee module and print the result.
*/
int main( int argc, char *argv[])
{
   char cmdstr[80];
   char buffer[ADDR64_STRING_LENGTH];
	int status;
	xbee_serial_t XBEE_SERPORT;
	addr64 return_ieee_be = *ZDO_IEEE_ADDR_PENDING;
	uint16_t return_net_addr = ZDO_NET_ADDR_PENDING;
	int err;

	parse_serial_arguments( argc, argv, &XBEE_SERPORT);

	// initialize the serial and device layer for this XBee device
	if (xbee_dev_init( &my_xbee, &XBEE_SERPORT, NULL, NULL))
	{
		printf( "Failed to initialize device.\n");
		return 0;
	}

	// Initialize the WPAN layer of the XBee device driver.  This layer enables
	// endpoints and clusters, and is required for all ZigBee layers.
	xbee_wpan_init( &my_xbee, &sample_endpoints.zdo);

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

	// have XBee handle ZDO responses -- on SE device, need to have it respond
	// to match descriptors looking for the Key Establishment cluster.
	xbee_cmd_simple( &my_xbee, "AO", 1);

   print_help();

   while (1)
   {
      while (xbee_readline( cmdstr, sizeof cmdstr) == -EAGAIN)
      {
      	wpan_tick( &my_xbee.wpan_dev);
      	if (! addr64_equal( &return_ieee_be, ZDO_IEEE_ADDR_PENDING))
      	{
				if (addr64_equal( &return_ieee_be, ZDO_IEEE_ADDR_TIMEOUT))
				{
					puts( "IEEE_addr request timed out");
				}
				else if (addr64_is_zero( &return_ieee_be))
				{
					printf( "Error retrieving IEEE_addr");
				}
				else
				{
					printf( "IEEE address is %" PRIsFAR "\n",
						addr64_format( buffer, &return_ieee_be));
				}
				return_ieee_be = *ZDO_IEEE_ADDR_PENDING;
      	}
      	if (return_net_addr != ZDO_NET_ADDR_PENDING)
      	{
      		if (return_net_addr == ZDO_NET_ADDR_TIMEOUT)
      		{
					puts( "NWK_addr request timed out");
      		}
      		else if (return_net_addr == ZDO_NET_ADDR_ERROR)
      		{
					printf( "Error retrieving NWK_addr");
      		}
      		else
      		{
      			printf( "Network address is 0x%04X\n", return_net_addr);
      		}
      		return_net_addr = ZDO_NET_ADDR_PENDING;
      	}
      }

		if (! strcmpi( cmdstr, "help") || ! strcmp( cmdstr, "?"))
		{
			print_help();
		}
      else if (! strcmpi( cmdstr, "quit"))
      {
			return 0;
		}
		else if (! strncmpi( cmdstr, "ieee ", 5))
		{
			addr64 ieee_be;

			if (addr64_parse( &ieee_be, &cmdstr[5]))
			{
				printf( "couldn't parse '%s'\n", &cmdstr[5]);
			}
			else
			{
				err = zdo_send_nwk_addr_req( &my_xbee.wpan_dev, &ieee_be,
					&return_net_addr);
				printf( "sent NWK_addr request for %" PRIsFAR " (err=%d)\n",
					addr64_format( buffer, &ieee_be), err);
			}
		}
		else if (! strncmpi( cmdstr, "net ", 4))
		{
			uint16_t net_addr;
			char *tail;

			net_addr = strtoul( &cmdstr[4], &tail, 0);
			if (tail == NULL)
			{
				printf( "couldn't parse '%s'\n", &cmdstr[4]);
			}
			else
			{
				err = zdo_send_ieee_addr_req( &my_xbee.wpan_dev, net_addr,
					&return_ieee_be);
				printf( "sent IEEE_addr request for 0x%04X (err=%d)\n", net_addr,
					err);
			}
		}
      else if (! strncmpi( cmdstr, "AT", 2))
      {
			process_command( &my_xbee, cmdstr);
	   }
   }
}

#include "xbee/atcmd.h"			// for XBEE_FRAME_HANDLE_LOCAL_AT
#include "xbee/device.h"		// for XBEE_FRAME_HANDLE_TX_STATUS
#include "xbee/wpan.h"			// for XBEE_FRAME_HANDLE_RX_EXPLICIT
const xbee_dispatch_table_entry_t xbee_frame_handlers[] =
{
	XBEE_FRAME_HANDLE_LOCAL_AT,
	XBEE_FRAME_HANDLE_RX_EXPLICIT,
	XBEE_FRAME_MODEM_STATUS_DEBUG,
	XBEE_FRAME_TABLE_END
};

