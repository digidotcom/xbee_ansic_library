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
/*
 * Sample program demonstrating the General Purpose Memory API, used to access
 * flash storage on some XBee modules (868LP, 900HP, Wi-Fi).  This space is
 * available for host use, and is also used to stage firmware images before
 * installation.
 */
#include <stdio.h>
#include "../common/_atinter.h"
#include "parse_serial_args.h"

#include "xbee/byteorder.h"
#include "xbee/gpm.h"
#include "xbee/wpan.h"

// function receiving data on transparent serial cluster when ATAO != 0
int gpm_response( const wpan_envelope_t FAR *envelope, void FAR *context)
{
	const xbee_gpm_frame_t FAR *frame = envelope->payload;

	switch (frame->header.response.cmd_id)
	{
		case XBEE_GPM_CMD_PLATFORM_INFO_RESP:
		{
			uint16_t blocks = be16toh( frame->header.response.block_num_be);
			uint16_t bytes = be16toh( frame->header.response.start_index_be);

			printf( "Platform: status 0x%02X, %u blocks, %u bytes/block = %"
					PRIu32 " bytes\n", frame->header.response.status,
					blocks, bytes, (uint32_t) blocks * bytes);
		}
		break;

		case XBEE_GPM_CMD_READ_RESP:
		{
			uint16_t block = be16toh( frame->header.response.block_num_be);
			uint16_t offset = be16toh( frame->header.response.start_index_be);
			uint16_t bytes = be16toh( frame->header.response.byte_count_be);

			printf( "Read %u bytes from offset %u of block %u:\n",
					bytes, offset, block);
			hex_dump( frame->data, bytes, HEX_DUMP_FLAG_OFFSET);
		}
		break;

		default:
			printf( "%u-byte GPM response:\n", envelope->length);
			hex_dump( envelope->payload, envelope->length, HEX_DUMP_FLAG_OFFSET);
			break;
	}

	return 0;
}
/////// endpoint table

// must be sorted by cluster ID
const wpan_cluster_table_entry_t digi_device_clusters[] =
{
	{ DIGI_CLUST_MEMORY_ACCESS, gpm_response, NULL,
		WPAN_CLUST_FLAG_INOUT | WPAN_CLUST_FLAG_NOT_ZCL },

	WPAN_CLUST_ENTRY_LIST_END
};

const wpan_endpoint_table_entry_t sample_endpoints[] = {
	{	WPAN_ENDPOINT_DIGI_DEVICE,		// endpoint
		WPAN_PROFILE_DIGI,				// profile ID
		NULL,									// endpoint handler
		NULL,									// ep_state
		0x0000,								// device ID
		0x00,									// version
		digi_device_clusters				// clusters
	},

	{ WPAN_ENDPOINT_END_OF_LIST }
};

////////// end of endpoint table

int parse_uint16( uint16_t *parsed, const char *text, int param_count)
{
	int index = 0;
	char *tail;
	unsigned long ul;

	if (text == NULL || parsed == NULL)
	{
		return -EINVAL;
	}

	for (;;)
	{
		ul = strtoul( text, &tail, 0);
		if (tail == text)
		{
			break;
		}
		if (index == param_count)
		{
			return -E2BIG;
		}
		parsed[index++] = (uint16_t) ul;
		text = tail;
	}

	return index;
}
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
	wpan_envelope_t envelope_self;

	parse_serial_arguments( argc, argv, &XBEE_SERPORT);

	// initialize the serial and device layer for this XBee device
	if (xbee_dev_init( &my_xbee, &XBEE_SERPORT, NULL, NULL))
	{
		printf( "Failed to initialize device.\n");
		return 0;
	}

	// Initialize the WPAN layer of the XBee device driver.  This layer enables
	// endpoints and clusters, and is required for all ZigBee layers.
	xbee_wpan_init( &my_xbee, sample_endpoints);

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

	xbee_gpm_envelope_local( &envelope_self, &my_xbee.wpan_dev);

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
      else if (! strcmpi( cmdstr, "info"))
      {
      	printf( "Sending platform info request (result %d)\n",
      			xbee_gpm_get_flash_info( &envelope_self));
      }
      else if (! strncmpi( cmdstr, "read", 4))
      {
      	uint16_t params[3];

      	if (parse_uint16( params, &cmdstr[5], 3) == 3)
      	{
      		printf( "Read %u bytes from offset %u of block %u (result %d)\n",
      				params[2], params[1], params[0],
      				xbee_gpm_read( &envelope_self,
      						params[0], params[1], params[2]));
      	}
      	else
      	{
      		printf( "Couldn't parse three values from [%s]\n", &cmdstr[5]);
      	}
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
	XBEE_FRAME_HANDLE_RX_EXPLICIT,
	XBEE_FRAME_TABLE_END
};

