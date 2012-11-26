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
#include <time.h>

#include "xbee/platform.h"
#include "xbee/commissioning.h"
#include "zigbee/zcl_commissioning.h"
#include "zigbee/zcl_identify.h"
#include "xbee/device.h"
#include "xbee/wpan.h"

#include "_commission_server.h"
#include "_atinter.h"
#include "parse_serial_args.h"

uint16_t profile_id = 0x0105;		// ZBA

const zcl_comm_startup_param_t zcl_comm_default_sas =
{
	0xFFFE,											// short_address
	ZCL_COMM_GLOBAL_EPID,						// extended_panid
	0xFFFF,											// panid (0xFFFF = not joined)
	UINT32_C(0x7FFF) << 11,						// channel_mask
	0x02,												// startup_control
/*
	{ { 0, 0, 0, 0, 0, 0, 0, 0 } },			// trust_ctr_addr
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
														// trust_ctr_master_key
*/
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
														// network_key
	FALSE,											// use_insecure_join
//	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 'Z','i','g','B','e','e','A','l','l','i','a','n','c','e','0','9' },
														// preconfig_link_key
	0x00,												// network_key_seq_num
	ZCL_COMM_KEY_TYPE_STANDARD,				// network_key_type
	0x0000,											// network_mgr_addr

	ZCL_COMM_SCAN_ATTEMPTS_DEFAULT,			// scan_attempts
	ZCL_COMM_TIME_BETWEEN_SCANS_DEFAULT,	// time_between_scans
	ZCL_COMM_REJOIN_INTERVAL_DEFAULT,		// rejoin_interval
	ZCL_COMM_MAX_REJOIN_INTERVAL_DEFAULT,	// max_rejoin_interval

	{	FALSE,											// concentrator.flag
		ZCL_COMM_CONCENTRATOR_RADIUS_DEFAULT,	// concentrator.radius
		0x00												// concentrator.discovery_time
	},
};

void print_help( void)
{
	puts( "Commissioning Server Help:");
	puts( "\tprofile 0xXXXX - set the profile ID for ZCL endpoint");
	puts( "\tdefault        - switch to Commissioning SAS");
	puts( "");
	puts( "\tquit           - quit program");
}

xbee_dev_t my_xbee;

void parse_args( int argc, char *argv[])
{
	int i;

	for (i = 1; i < argc; ++i)
	{
		if (strncmp( argv[i], "--profile=", 10) == 0)
		{
			profile_id = (uint16_t) strtoul( &argv[i][10], NULL, 16);
		}
	}
}

int main( int argc, char *argv[])
{
   char cmdstr[80];
	int status;
	xbee_serial_t XBEE_SERPORT;

	// set serial port
	parse_serial_arguments( argc, argv, &XBEE_SERPORT);

	// parse args for this program
	parse_args( argc, argv);

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

	// set Profile ID for our Basic/Commissioning Server Cluster endpoint
	sample_endpoints.zcl.profile_id = profile_id;

   print_help();

	while (1)
   {
      while (xbee_readline( cmdstr, sizeof cmdstr) == -EAGAIN)
      {
			// tick function for ZCL Commissioning Server
			xbee_commissioning_tick( &my_xbee, &zcl_comm_state);

			// change associate flash rate when Identify is active
			xbee_zcl_identify( &my_xbee);

      	wpan_tick( &my_xbee.wpan_dev);
      }

		if (! strcmpi( cmdstr, "quit"))
      {
			return 0;
		}
		else if (! strcmpi( cmdstr, "help") || ! strcmp( cmdstr, "?"))
		{
			print_help();
		}
		else if (! strncmpi( cmdstr, "profile ", 8))
		{
			sample_endpoints.zcl.profile_id = strtoul( &cmdstr[8], NULL, 16);
			printf( "Profile ID set to 0x%04x\n",
														sample_endpoints.zcl.profile_id);
		}
		else if (! strcmpi( cmdstr, "default"))
      {
      	zcl_comm_startup_param_t sas = zcl_comm_default_sas;

      	puts( "switching to default SAS");
			xbee_commissioning_set( &my_xbee, &sas);
		}
		else if (! strncmpi( cmdstr, "AT", 2))
		{
			process_command( &my_xbee, &cmdstr[2]);
		}
	   else
	   {
	   	printf( "unknown command: '%s'\n", cmdstr);
	   }
   }
}

