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

/***************************************************************************
	Interactive OTA client, used for testing.

	Supports "AT Interactive" commands and a time client too.

	Used to send firmware updates OTA (over the air) to Programmable XBee
	devices.
*****************************************************************************/

#include <stdio.h>

#include "xbee/platform.h"
#include "xbee/atcmd.h"
#include "zigbee/zdo.h"
#include "zigbee/zcl.h"
#include "zigbee/zcl_client.h"
#include "wpan/aps.h"

#include "wpan/aps.h"
#include "xbee/ota_client.h"

#include "_pxbee_ota_update.h"

#define ZCL_APP_VERSION			0x12
#define ZCL_MANUFACTURER_NAME	"Digi International"
#define ZCL_MODEL_IDENTIFIER	"OTA Client"
#define ZCL_DATE_CODE			"20110525 dev"
#define ZCL_POWER_SOURCE		ZCL_BASIC_PS_SINGLE_PHASE
#include "zigbee/zcl_basic_attributes.h"

/// Used to track ZDO transactions in order to match responses to requests
/// (#ZDO_MATCH_DESC_RESP).
wpan_ep_state_t zdo_ep_state = { 0 };

/// Tracks state of ZCL endpoint.
wpan_ep_state_t zcl_ep_state = { 0 };

basic_value_t basic_value;

/*
	Structure used to hold values read from Basic Cluster on potential targets.
*/
const struct {
	zcl_attribute_base_t		app_ver;
	zcl_attribute_base_t		stack_ver;
	zcl_attribute_full_t		model_id;
	zcl_attribute_full_t		datecode;
} basic_attr = {
	{	ZCL_BASIC_ATTR_APP_VERSION,
		ZCL_ATTRIB_FLAG_NONE,
		ZCL_TYPE_UNSIGNED_8BIT,
		&basic_value.app_ver,
	},
	{	ZCL_BASIC_ATTR_STACK_VERSION,
		ZCL_ATTRIB_FLAG_NONE,
		ZCL_TYPE_UNSIGNED_8BIT,
		&basic_value.stack_ver,
	},
	{
		{	ZCL_BASIC_ATTR_MODEL_IDENTIFIER,
			ZCL_ATTRIB_FLAG_FULL,
			ZCL_TYPE_STRING_CHAR,
			basic_value.model_id,
		},
		{ 0 }, { sizeof(basic_value.model_id) - 1 },
		NULL, NULL
	},
	{
		{	ZCL_BASIC_ATTR_DATE_CODE,
			ZCL_ATTRIB_FLAG_FULL,
			ZCL_TYPE_STRING_CHAR,
			basic_value.datecode,
		},
		{ 0 }, { sizeof(basic_value.datecode) - 1 },
		NULL, NULL
	},
};

target_t target_list[TARGET_COUNT];
int target_index = 0;

int find_target( const addr64 *ieee)
{
	uint8_t i;

	for (i = 0; i < target_index; ++i)
	{
		if (addr64_equal( &target_list[i].ieee, ieee))
		{
			// found in list
			return i;
		}
	}

	return -1;
}

void print_target( int index)
{
	target_t *t;

	t = &target_list[index];
	if (! addr64_is_zero( &t->ieee))
	{
		printf( "%2u: %08" PRIx32 "-%08" PRIx32 " 0x%02x %-32s %-16s\n", index,
			be32toh( t->ieee.l[0]), be32toh( t->ieee.l[1]),
			t->basic.app_ver, t->basic.model_id, t->basic.datecode);
	}
}

// parse the response to reading the other device's Basic Attributes
int basic_parse( zcl_command_t *zcl)
{
	uint8_t			response = ZCL_STATUS_SUCCESS;
	int				i;

/*
	find the device in our target_t list
	use a list of offsets into target_t, along with attr ID, type and length limits
	copy the attribute values from the response, ignore non-SUCCESS status

*/
	i = find_target( &zcl->envelope->ieee_address);
	if (i >= 0)
	{
		if (target_list[i].basic.app_ver > 0)
		{
			//puts( "ignoring duplicate basic response");
		}
		else
		{
			memset( &basic_value, 0, sizeof basic_value);

			response = zcl_process_read_attr_response( zcl, &basic_attr.app_ver);

			target_list[i].basic = basic_value;
			print_target( i);
		}
	}

	return zcl_default_response( zcl, response);
}

int basic_client( const wpan_envelope_t FAR *envelope,
	void FAR *context)
{
	zcl_command_t	zcl;

	if (zcl_command_build( &zcl, envelope, context) == 0 &&
		zcl.command == ZCL_CMD_READ_ATTRIB_RESP &&
		ZCL_CMD_MATCH( &zcl.frame_control, GENERAL, SERVER_TO_CLIENT, PROFILE))
	{
		// response to our read basic attr
		return basic_parse( &zcl);
	}

	return zcl_general_command( envelope, context);
}

const wpan_cluster_table_entry_t zcl_cluster_table[] =
{
	{	ZCL_CLUST_BASIC,
		&basic_client,
		zcl_basic_attribute_tree,
		WPAN_CLUST_FLAG_INOUT
	},

	WPAN_CLUST_ENTRY_LIST_END
};

// Since we're not using a dynamic frame dispatch table, we need to define
// it here.
#include "xbee/atcmd.h"			// for XBEE_FRAME_HANDLE_LOCAL_AT
#include "xbee/wpan.h"			// for XBEE_FRAME_HANDLE_RX_EXPLICIT
const xbee_dispatch_table_entry_t xbee_frame_handlers[] =
{
	XBEE_FRAME_HANDLE_LOCAL_AT,
	XBEE_FRAME_HANDLE_RX_EXPLICIT,
	XBEE_FRAME_TABLE_END
};

// !!! temporarily a global, move to some state/context later
xbee_ota_t xbee_ota;

int xbee_ota_error( const wpan_envelope_t	FAR *envelope,
	void FAR *context)
{
	// This cluster callback has a NULL context in the cluster table, so
	// there's no need to reference it here.
	XBEE_UNUSED_PARAMETER( context);

	// display any error messages sent to the OTA update client
	puts( "Error from target:");
	printf( "%.*s\n", envelope->length, (char *) envelope->payload);

	return 0;
}

wpan_cluster_table_entry_t digi_data_clusters[] =
{
	// Initialization code may set the flags for this entry to use encryption
	XBEE_OTA_DATA_CLIENT_CLUST_ENTRY( &xbee_ota, WPAN_CLUST_FLAG_NONE),

	// cluster for sending command to kick off OTA update
	XBEE_OTA_CMD_CLIENT_CLUST_ENTRY( xbee_ota_error, NULL,
																		WPAN_CLUST_FLAG_NONE),

	WPAN_CLUST_ENTRY_LIST_END
};

// note that you can't use a modified version of this structure in your
// program, since the client_read function refers to it
struct _endpoints sample_endpoints = {
	ZDO_ENDPOINT(zdo_ep_state),

	{	1,										// endpoint
		0,										// profile ID (filled in later)
		NULL,									// endpoint handler
		&zcl_ep_state,						// ep_state
		0x0000,								// device ID
		0x00,									// version
		zcl_cluster_table,				// clusters
	},

	// Endpoint/cluster for transparent serial and OTA command cluster
	{	WPAN_ENDPOINT_DIGI_DATA,		// endpoint
		WPAN_PROFILE_DIGI,				// profile ID
		NULL,									// endpoint handler
		NULL,									// ep_state
		0x0000,								// device ID
		0x00,									// version
		digi_data_clusters				// clusters
	},

	WPAN_ENDPOINT_END_OF_LIST
};

int xbee_ota_find_devices( wpan_dev_t *dev, wpan_response_fn callback,
	const void FAR *context)
{
	static uint16_t clusters[] =
				{ DIGI_CLUST_PROG_XBEE_OTA_UPD, WPAN_CLUSTER_END_OF_LIST };

	// find devices with OTA
	return zdo_send_match_desc( dev, clusters, WPAN_PROFILE_DIGI,
																		callback, context);
}

// This should be the ZDO Match Descriptor response to our search for
// an XBee with the OTA update cluster.
int xbee_found( wpan_conversation_t FAR *conversation,
	const wpan_envelope_t FAR *envelope)
{
	int i;

	const uint16_t attributes[] = {
		ZCL_BASIC_ATTR_APP_VERSION,
		ZCL_BASIC_ATTR_STACK_VERSION,
		ZCL_BASIC_ATTR_MODEL_IDENTIFIER,
		ZCL_BASIC_ATTR_DATE_CODE,
		ZCL_ATTRIBUTE_END_OF_LIST
	};
	const zcl_client_read_t client_read = {
		&sample_endpoints.zcl,
		ZCL_MFG_NONE,
		ZCL_CLUST_BASIC,
		attributes
	};
	wpan_envelope_t reply_envelope;

	// We don't need to reference the 'converstation' parameter in this
	// wpan_response_fn callback.
	XBEE_UNUSED_PARAMETER( conversation);

	if (! envelope)		// conversation timed out
	{
		if (target_index == 0)
		{
			printf( "%s: no targets found\n", __FUNCTION__);
		}
		return 0;
	}

	if (envelope->cluster_id != ZDO_MATCH_DESC_RSP)
	{
		printf( "%s: cluster is not a ZDO response?\n", __FUNCTION__);
		return -EINVAL;
	}

	i = find_target( &envelope->ieee_address);
	if (i < 0 && target_index < TARGET_COUNT)
	{
		i = target_index;
		// new target found -- add to list
		target_list[target_index].ieee = envelope->ieee_address;
		++target_index;

	}

	if (i >= 0)
	{
		// send Read Attributes to Basic Cluster
		wpan_envelope_reply( &reply_envelope, envelope);

		reply_envelope.source_endpoint = client_read.ep->endpoint;
		reply_envelope.profile_id = client_read.ep->profile_id;
		reply_envelope.cluster_id = client_read.cluster_id;

		// use broadcast endpoint
		reply_envelope.dest_endpoint = 0xFF;
	   reply_envelope.options = current_profile->flags;

	   zcl_client_read_attributes( &reply_envelope, &client_read);
	}

	// just need to find one
	return WPAN_CONVERSATION_CONTINUE;
}
