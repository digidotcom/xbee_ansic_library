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
	Interactive Commissioning Cluster client, used for testing.

	Supports "AT Interactive" commands too.
*****************************************************************************/

#include <stdio.h>

#include "xbee/platform.h"
#include "xbee/atcmd.h"
#include "zigbee/zdo.h"
#include "zigbee/zcl.h"
#include "zigbee/zcl_client.h"
#include "zigbee/zcl_commissioning.h"
#include "wpan/aps.h"

#include "_commission_client.h"

#include "zigbee/zcl_basic.h"
#define ZCL_APP_VERSION			0x12
#define ZCL_MANUFACTURER_NAME	"Digi International"
#define ZCL_MODEL_IDENTIFIER	"ZCL Commissioning Client"
#define ZCL_DATE_CODE			"20110331 dev"
#define ZCL_POWER_SOURCE		ZCL_BASIC_PS_SINGLE_PHASE
#include "zigbee/zcl_basic_attributes.h"

/// Used to track ZDO transactions in order to match responses to requests
/// (#ZDO_MATCH_DESC_RESP).
wpan_ep_state_t zdo_ep_state = { 0 };

/// Tracks state of ZCL endpoint.
wpan_ep_state_t zcl_ep_state = { 0 };

basic_value_t basic_value;

/*
	It would be interesting to explore a way to store offsets instead of
	addresses in this structure, and then copy it to the stack while converting
	offsets to an actual address (either in an array, or a temp object on the
	stack).
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

	i = find_target( &zcl->envelope->ieee_address);
	if (i >= 0)
	{
		// could ignore duplicates by checking (target_list[i].basic.app_ver > 0)

		memset( &basic_value, 0, sizeof basic_value);

		response = zcl_process_read_attr_response( zcl, &basic_attr.app_ver);

		target_list[i].basic = basic_value;
		print_target( i);
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

int commissioning_client( const wpan_envelope_t FAR *envelope,
	void FAR *context)
{
	const char *cmd = NULL;
	zcl_command_t	zcl;
	int err;

	puts( "frame for commissioning client:");

	err = zcl_command_build( &zcl, envelope, context);
	if (err)
	{
		printf( "\tzcl_command_build() returned %d for %u-byte payload\n",
			err, envelope->length);
		hex_dump( envelope->payload, envelope->length, HEX_DUMP_FLAG_TAB);
	}
	else
	{
		if (ZCL_CMD_MATCH( &zcl.frame_control,
													GENERAL, SERVER_TO_CLIENT, CLUSTER))
		{
			switch (zcl.command)
			{
				case ZCL_COMM_CMD_RESTART_DEVICE_RESP:
					cmd = "Restart Device Response";
					break;

				case ZCL_COMM_CMD_SAVE_STARTUP_PARAM_RESP:
					cmd = "Save Startup Param Response";
					break;

				case ZCL_COMM_CMD_RESTORE_STARTUP_PARAM_RESP:
					cmd = "Restore Startup Param Response";
					break;

				case ZCL_COMM_CMD_RESET_STARTUP_PARAM_RESP:
					cmd = "Reset Startup Param Response";
					break;
			}

			// pretty-print properly formatted commands that we know
			if (cmd != NULL && zcl.length == 1)
			{
				printf( "\t%s: Status = %s\n", cmd,
					zcl_status_text( *(const uint8_t FAR *)zcl.zcl_payload));

				// ignore this response
				return zcl_default_response( &zcl, ZCL_STATUS_SUCCESS);
			}
		}

		// if not already parsed, dump the raw command
		zcl_command_dump( &zcl);
	}

	// let the ZCL General Command handler process all unhandled frames
	return zcl_general_command( envelope, context);
}

const wpan_cluster_table_entry_t zcl_cluster_table[] =
{
	{	ZCL_CLUST_BASIC,
		&basic_client,
		zcl_basic_attribute_tree,
		WPAN_CLUST_FLAG_INOUT
	},

	{	ZCL_CLUST_COMMISSIONING,
		&commissioning_client,
		NULL,
		WPAN_CLUST_FLAG_CLIENT
	},

	WPAN_CLUST_ENTRY_LIST_END
};

struct _endpoints sample_endpoints = {
	ZDO_ENDPOINT(zdo_ep_state),

	{	SAMPLE_COMMISION_ENDPOINT,		// endpoint
		0,										// profile ID (filled in later)
		NULL,									// endpoint handler
		&zcl_ep_state,						// ep_state
		0x0000,								// device ID
		0x00,									// version
		zcl_cluster_table,				// clusters
	},

	WPAN_ENDPOINT_END_OF_LIST
};

// This should be the ZDO Match Descriptor response to our search for
// an XBee with the Commissioning cluster.  Assume the Basic cluster
// is on the same endpoint (for now).  Correct solution is to generate
// another ZDO request with a unicast to find the Basic Cluster on
// that target.
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
	const PACKED_STRUCT {
		uint8_t									transaction;
		zdo_match_desc_rsp_header_t		header;
		uint8_t									endpoints[80];
	} FAR *match_response;

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

		// TODO: from here, we should unicast a Basic Cluster discover to the
		// target to find the correct endpoint.  For now we assume it's the
		// same endpoint as the Commissioning Cluster.
		match_response = envelope->payload;
		target_list[i].endpoint =
					reply_envelope.dest_endpoint = match_response->endpoints[0];

		reply_envelope.source_endpoint = client_read.ep->endpoint;
		reply_envelope.profile_id = client_read.ep->profile_id;
		reply_envelope.cluster_id = client_read.cluster_id;
	   reply_envelope.options = WPAN_CLUST_FLAG_NONE;

	   zcl_client_read_attributes( &reply_envelope, &client_read);
	}

	// just need to find one
	return WPAN_CONVERSATION_CONTINUE;
}

