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
#include "zigbee/zcl_client.h"
#include "zigbee/zcl_commissioning.h"
#include "zigbee/zcl_types.h"
#include "zigbee/zdo.h"
#include "xbee/device.h"
#include "xbee/wpan.h"

#include "_commission_client.h"
#include "_atinter.h"
#include "parse_serial_args.h"

uint16_t profile_id = 0x0105;				// ZBA

typedef struct zami_net_config_t {
	zcl64_t		extended_panid;
	uint32_t		channel_mask;
	uint8_t		startup_control;
	uint8_t		network_key_le[16];
	uint8_t		preconfig_link_key_le[16];
} zami_net_config_t;

zami_net_config_t zami_net_config;

const zcl_attribute_base_t zami_net_config_attributes[] =
{
	{	ZCL_COMM_ATTR_EXTENDED_PANID,
		ZCL_ATTRIB_FLAG_NONE,
		ZCL_TYPE_IEEE_ADDR,
		&zami_net_config.extended_panid
	},

	{	ZCL_COMM_ATTR_CHANNEL_MASK,
		ZCL_ATTRIB_FLAG_NONE,
		ZCL_TYPE_BITMAP_32BIT,
		&zami_net_config.channel_mask
	},

	{	ZCL_COMM_ATTR_STARTUP_CONTROL,
		ZCL_ATTRIB_FLAG_NONE,
		ZCL_TYPE_ENUM_8BIT,
		&zami_net_config.startup_control
	},

	{	ZCL_COMM_ATTR_NETWORK_KEY,
		ZCL_ATTRIB_FLAG_RAW,
		ZCL_TYPE_SECURITY_KEY,
		&zami_net_config.network_key_le
	},

	{	ZCL_COMM_ATTR_PRECONFIG_LINK_KEY,
		ZCL_ATTRIB_FLAG_RAW,
		ZCL_TYPE_SECURITY_KEY,
		&zami_net_config.preconfig_link_key_le
	},

	{ ZCL_ATTRIBUTE_END_OF_LIST }
};

const zcl_comm_startup_param_t network_comm =
{
	0xFFFE,											// short_address
	ZCL_COMM_GLOBAL_EPID,						// extended_panid
	0xFFFF,											// panid (0xFFFF = not joined)
	UINT32_C(0x7FFF) << 11,						// channel_mask
	ZCL_COMM_STARTUP_FROM_SCRATCH,			// startup_control
/*
	{ { 0, 0, 0, 0, 0, 0, 0, 0 } },			// trust_ctr_addr
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
														// trust_ctr_master_key
*/
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
														// network_key
	FALSE,											// use_insecure_join
	{	'Z', 'i', 'g', 'B', 'e', 'e',
		'A', 'l', 'l', 'i', 'a', 'n', 'c', 'e', '0', '9' },
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
zcl_comm_startup_param_t network_deploy =
{
	0xFFFE,											// short_address
	ZCL64_INIT( 0, 0x0123),						// extended_panid
	0xFFFF,											// panid (0xFFFF = not joined)
	UINT32_C(0x7FFF) << 11,						// channel_mask
	ZCL_COMM_STARTUP_JOINED,					// startup_control
/*
	{ { 0, 0, 0, 0, 0, 0, 0, 0 } },			// trust_ctr_addr
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
														// trust_ctr_master_key
*/
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3 },
														// network_key
	FALSE,											// use_insecure_join
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'M', 'S', 'E', 'D', 'C', 'L' },
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
	puts( "Commissioning Client Help:");
	puts( "\tprofile 0xXXXX - set the profile ID for ZCL endpoint");
	puts( "\tfind           - find potential targets with PXBee OTA cluster");
	puts( "\ttarget         - show list of known targets");
	puts( "\ttarget <index> - set target to entry <index> from target list");
	puts( "\tsave           - tell target to save changes and restart network");
	puts( "\tcancel         - tell target to drop changes and restart network");
	puts( "\tdefault        - tell target to revert to default SAS");
	puts( "\tdeploy r       - switch target to Deployed SAS as router");
	puts( "\tdeploy c       - switch target to Deployed SAS as coordinator");
	puts( "\tcomm           - switch target to Commissioning SAS");
	puts( "");
	puts( "\tquit           - quit program");
}

xbee_dev_t my_xbee;

int find_devices( void)
{
	static uint16_t clusters[] =
				{ ZCL_CLUST_COMMISSIONING, WPAN_CLUSTER_END_OF_LIST };

	// find devices with Commissioning Cluster
	return zdo_send_match_desc( &my_xbee.wpan_dev, clusters,
													profile_id, xbee_found, NULL);
}

int zdo_device_annce_handler( const wpan_envelope_t FAR *envelope,
		void FAR *context)
{
	addr64 addr_be;
	char buffer[ADDR64_STRING_LENGTH];
	const zdo_device_annce_t FAR *annce;

	// Standard wpan_aps_handler_fn callback, but we don't use the context param.
	XBEE_UNUSED_PARAMETER( context);

	if (envelope == NULL)
	{
		return -EINVAL;
	}

	annce = envelope->payload;
	memcpy_letobe( &addr_be, &annce->ieee_address_le, 8);
	printf( "Device Announce %" PRIsFAR " (0x%04x) cap 0x%02x\n",
		addr64_format( buffer, &addr_be), le16toh( annce->network_addr_le),
		annce->capability);

	// discover the commissioning cluster and hand off to xbee_found?

	return 0;
}

const wpan_cluster_table_entry_t zdo_clusters[] = {
		ZDO_DEVICE_ANNCE_CLUSTER( zdo_device_annce_handler, NULL),
		{ WPAN_CLUSTER_END_OF_LIST }
};

int restart_target( const target_t *target, bool_t save_changes)
{
	wpan_envelope_t					envelope;
	zcl_comm_restart_device_cmd_t	command;

	if (target == NULL)
	{
		puts( "You must select a target first");
		return -EINVAL;
	}

	memset( &command, 0, sizeof command);
	command.options = save_changes	? ZCL_COMM_RESTART_OPT_MODE_SAVE_CHANGES
												: ZCL_COMM_RESTART_OPT_MODE_NO_CHANGES;
	command.delay = 3;
	command.jitter = 100;

	printf( "%s: sending restart with 3000 to 8000ms delay\n", __FUNCTION__);

	wpan_envelope_create( &envelope, &my_xbee.wpan_dev, &target->ieee,
		WPAN_NET_ADDR_UNDEFINED);
	envelope.dest_endpoint = target->endpoint;

	return zcl_comm_restart_device( &envelope, &command);
}

int default_target( const target_t *target)
{
	wpan_envelope_t					envelope;

	if (target == NULL)
	{
		puts( "You must select a target first");
		return -EINVAL;
	}

	wpan_envelope_create( &envelope, &my_xbee.wpan_dev, &target->ieee,
		WPAN_NET_ADDR_UNDEFINED);
	envelope.dest_endpoint = target->endpoint;

	return zcl_comm_reset_parameters( &envelope, NULL);
}

int set_pan( const target_t *target, const zcl_comm_startup_param_t *sas)
{
	wpan_envelope_t					envelope;

	if (target == NULL)
	{
		puts( "You must select a target first");
		return -EINVAL;
	}

	wpan_envelope_create( &envelope, &my_xbee.wpan_dev, &target->ieee,
		WPAN_NET_ADDR_UNDEFINED);
	envelope.dest_endpoint = target->endpoint;
	envelope.source_endpoint = SAMPLE_COMMISION_ENDPOINT;
	envelope.profile_id = profile_id;
	envelope.cluster_id = ZCL_CLUST_COMMISSIONING;

	zami_net_config.extended_panid = sas->extended_panid;
	zami_net_config.channel_mask = sas->channel_mask;
	zami_net_config.startup_control = sas->startup_control;
	memcpy( zami_net_config.network_key_le, sas->network_key_le, 16);
	memcpy( zami_net_config.preconfig_link_key_le, sas->preconfig_link_key_le, 16);

	return zcl_send_write_attributes( &envelope, zami_net_config_attributes);
}

int main( int argc, char *argv[])
{
   char cmdstr[80];
	int status, i;
	xbee_serial_t XBEE_SERPORT;
	target_t *target = NULL;

	memset( target_list, 0, sizeof target_list);

	parse_serial_arguments( argc, argv, &XBEE_SERPORT);

	// initialize the serial and device layer for this XBee device
	if (xbee_dev_init( &my_xbee, &XBEE_SERPORT, NULL, NULL))
	{
		printf( "Failed to initialize device.\n");
		return 0;
	}

	// replace ZDO cluster table with one that intercepts Device Annce messages
	sample_endpoints.zdo.cluster_table = zdo_clusters;

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

	// set Profile ID for our Basic Client Cluster endpoint
	sample_endpoints.zcl.profile_id = profile_id;

   print_help();

   puts( "searching for Commissioning Servers");
	find_devices();

	while (1)
   {
      while (xbee_readline( cmdstr, sizeof cmdstr) == -EAGAIN)
      {
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
			profile_id = strtoul( &cmdstr[8], NULL, 16);
			printf( "Profile ID set to 0x%04x\n", profile_id);
			sample_endpoints.zcl.profile_id = profile_id;
		}
		else if (! strcmpi( cmdstr, "find"))
		{
			find_devices();
		}
		else if (! strcmpi( cmdstr, "target"))
		{
			puts( " #: --IEEE Address--- Ver. --------Application Name--------"
				" ---Date Code----");
			for (i = 0; i < target_index; ++i)
			{
				print_target( i);
			}
			puts( "End of List");
		}
		else if (! strncmpi( cmdstr, "target ", 7))
		{
			i = (int) strtoul( &cmdstr[7], NULL, 10);
			if (target_index == 0)
			{
				printf( "error, no targets in list, starting search now...\n");
				find_devices();
			}
			else if (i < 0 || i >= target_index)
			{
				printf( "error, index %d is invalid (must be 0 to %u)\n", i,
					target_index - 1);
			}
			else
			{
				target = &target_list[i];
				puts( "set target to:");
				print_target( i);
			}
		}
		else if (! strcmpi( cmdstr, "save"))
		{
			restart_target( target, TRUE);
		}
		else if (! strcmpi( cmdstr, "cancel"))
		{
			restart_target( target, FALSE);
		}
		else if (! strcmpi( cmdstr, "default"))
		{
			default_target( target);
		}
		else if (! strcmpi( cmdstr, "deploy"))
		{
			set_pan( target, &network_deploy);
		}
		else if (! strncmpi( cmdstr, "deploy ", 7))
		{
			if (cmdstr[7] == 'r')
			{
				puts( "deploy as router");
				network_deploy.startup_control = ZCL_COMM_STARTUP_JOINED;
			}
			else if (cmdstr[7] == 'c')
			{
				puts( "deploy as coordinator");
				network_deploy.startup_control = ZCL_COMM_STARTUP_COORDINATOR;
			}
			set_pan( target, &network_deploy);
		}
		else if (! strcmpi( cmdstr, "comm"))
		{
			set_pan( target, &network_comm);
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

// Since we're not using a dynamic frame dispatch table, we need to define
// it here.
#include "xbee/atcmd.h"			// for XBEE_FRAME_HANDLE_LOCAL_AT
#include "xbee/wpan.h"			// for XBEE_FRAME_HANDLE_RX_EXPLICIT
const xbee_dispatch_table_entry_t xbee_frame_handlers[] =
{
	XBEE_FRAME_HANDLE_LOCAL_AT,
	XBEE_FRAME_HANDLE_RX_EXPLICIT,
	XBEE_FRAME_MODEM_STATUS_DEBUG,
	XBEE_FRAME_TABLE_END
};

