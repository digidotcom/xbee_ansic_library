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

#include <stddef.h>
#include <stdio.h>

#include "xbee/platform.h"
#include "xbee/atcmd.h"
#include "xbee/time.h"
#include "zigbee/zdo.h"
#include "zigbee/zcl.h"
#include "zigbee/zcl_client.h"
#include "wpan/aps.h"

#include "_zigbee_walker.h"

#define ZCL_APP_VERSION			0x01
#define ZCL_MANUFACTURER_NAME	"Digi International"
#define ZCL_MODEL_IDENTIFIER	"ZDO/ZCL Walker"
#define ZCL_DATE_CODE			"20120101"
#define ZCL_POWER_SOURCE		ZCL_BASIC_PS_SINGLE_PHASE
#include "zigbee/zcl_basic_attributes.h"


#define WALKER_ZCL_TIMEOUT 15

//#define WALKER_DEBUG

/* Initially tried doing this without a global for the state, but ran into
	trouble with the ZCL handler -- it doesn't have a link back to the caller's
	walker_state_t.  Additionally, the ZCL handler can only handle one walk
	at a time and we're using lots of other globals, so just roll with it.

	Then I changed the ZCL code to use conversations, which would allow me
	to pass a pointer to the client's walker object as the context.  But,
	we'd run into failures with more than one walker at a time due to the
	global endpoint and cluster table that's modified while walking.
*/
walker_state_t	walker;

/// Used to track ZDO transactions in order to match responses to requests
/// (#ZDO_MATCH_DESC_RESP).
wpan_ep_state_t zdo_ep_state = { 0 };

/// Tracks state of ZCL endpoint.
wpan_ep_state_t zcl_ep_state = { 0 };

const wpan_cluster_table_entry_t basic_server = ZCL_CLUST_ENTRY_BASIC_SERVER;

// We'll actually modify this cluster table on an as-needed basis to
// complement the ZCL cluster of the target that we're reading.
wpan_cluster_table_entry_t zcl_cluster_table[] =
{
	{ 0, &zcl_general_command, zcl_attributes_none, WPAN_CLUST_FLAG_INOUT },

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
//	XBEE_FRAME_MODEM_STATUS_DEBUG,
	XBEE_FRAME_TABLE_END
};

struct _endpoints sample_endpoints = {
	ZDO_ENDPOINT(zdo_ep_state),

	{	SAMPLE_ENDPOINT,					// endpoint
		0,										// profile ID (filled in later)
		NULL,									// endpoint handler
		&zcl_ep_state,						// ep_state
		0x0000,								// device ID
		0x00,									// version
		zcl_cluster_table,				// clusters
	},

	WPAN_ENDPOINT_END_OF_LIST
};

// eventually use command-line parameter to select UTC vs. local time
bool_t walker_use_gmtime = TRUE;
char *format_time( char *dest, zcl_utctime_t utctime)
{
	struct tm tm;
	time_t t;

	if (utctime == ZCL_UTCTIME_INVALID)
	{
		strcpy( dest, "[Invalid Timestamp]");
	}
	else
	{
		if (walker_use_gmtime)
		{
			xbee_gmtime( &tm, utctime);
			sprintf( dest, "%04u-%02u-%02u %02u:%02u:%02uZ",
				tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
				tm.tm_hour, tm.tm_min, tm.tm_sec);
		}
		else
		{
			t = utctime + ZCL_TIME_EPOCH_DELTA;
			tm = *localtime( &t);
			sprintf( dest, "%04u-%02u-%02u %02u:%02u:%02u",
				tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
				tm.tm_hour, tm.tm_min, tm.tm_sec);
		}
	}

	return dest;
}

int walker_init( wpan_dev_t *dev, addr64 *target, uint16_t flags)
{
	char addr64_buffer[ADDR64_STRING_LENGTH];

	if (dev == NULL || target == NULL)
	{
		return -EINVAL;
	}

	memset( &walker, 0, sizeof walker);
	walker.dev = dev;
	walker.target.ieee = *target;
	walker.target.network = WPAN_NET_ADDR_UNDEFINED;
	walker.flags = flags & WALKER_USER_FLAG_MASK;
	walker.status = WALKER_INIT;
	walker.start = time( NULL);

	puts( "Digi International -- ZDO/ZCL Walker v" WALKER_VER_STR);
	printf( "Started at %s", ctime( &walker.start));
	printf( "Performing discovery on %" PRIsFAR ".\n",
		addr64_format( addr64_buffer, target));

	return 0;
}

int walker_active_ep_resp( wpan_conversation_t FAR *conversation,
	const wpan_envelope_t FAR *envelope)
{
	const PACKED_STRUCT {
		uint8_t								transaction;
		zdo_active_ep_rsp_header_t		header;
		uint8_t								endpoints[255];
	} FAR *response;
	const uint8_t FAR *ep;
	uint_fast8_t i;

	// standard wpan_response_fn, but we don't need to look at the conversation
	XBEE_UNUSED_PARAMETER( conversation);

	if (envelope == NULL)
	{
		printf( "Error: timeout waiting for active EP response");
		walker.status = WALKER_ERROR;
	}
	else
	{
		response = envelope->payload;

		ep = response->endpoints;
		i = response->header.ep_count;
		#ifdef WALKER_DEBUG
			printf( "got active EP response with %u endpoints\n", i);
			hex_dump( ep, i, HEX_DUMP_FLAG_TAB);
		#endif

		for (i = response->header.ep_count; i; ++ep, --i)
		{
			WALKER_SET_EP( walker, *ep);
		}
		walker.status = WALKER_SIMPLE_DESC_REQ;
	}

	return WPAN_CONVERSATION_END;
}

void walker_print_cluster( uint8_t index)
{
	printf( "  %s cluster 0x%04X\n",
		(index < walker.cluster.out_index) ? " input/server" : "output/client",
		walker.cluster.list[index]);
}

void walker_dump_cluster_list( void)
{
	uint_fast8_t i, count;

	count = walker.cluster.count;
	for (i = 0; i < count; ++i)
	{
		walker_print_cluster( i);
	}
}

// todo: need a LOT of error checking in here to make sure we don't walk
// off the end of the response, or over the cluster list.
int walker_simple_desc_resp( wpan_conversation_t FAR *conversation,
	const wpan_envelope_t FAR *envelope)
{
	const PACKED_STRUCT {
		uint8_t								transaction;
		zdo_simple_desc_resp_header_t	header;
		zdo_simple_desc_header_t		descriptor;
		uint8_t								clusters[255];
	} FAR *response;

	ptrdiff_t overparse;

	uint16_t FAR *cluster_le;
	uint16_t *cluster;
	uint_fast8_t count;
	uint_fast8_t i;

	// standard wpan_response_fn, but we don't need to look at the conversation
	XBEE_UNUSED_PARAMETER( conversation);

	if (envelope == NULL)
	{
		printf( "Error: timeout waiting for Simple Desc response");
		walker.status = WALKER_ERROR;
	}
	else
	{
		response = envelope->payload;

		walker.profile_id = le16toh( response->descriptor.profile_id_le);
		printf( "Endpoint 0x%02X  Profile 0x%04X  Device 0x%04X  Ver 0x%02X\n",
			walker.current_endpoint, walker.profile_id,
			le16toh( response->descriptor.device_id_le),
			response->descriptor.device_version);

		// reset the cluster and attribute information
		memset( &walker.cluster, 0, sizeof walker.cluster);
		memset( &walker.attribute, 0, sizeof walker.attribute);

		cluster = walker.cluster.list;
		cluster_le = (uint16_t *)response->clusters;
		for (i = 0; i < 2; ++i)
		{
			count = *(uint8_t *)cluster_le;
			// point past the count to the cluster list
			cluster_le = (uint16_t *)((uint8_t *)cluster_le + 1);

			if (i == 0)
			{
				// the output clusters will start after the input clusters
				walker.cluster.out_index = count;
			}

			walker.cluster.count += count;
			if (walker.cluster.count > WALKER_MAX_CLUST)
			{
				printf( "ERROR: Can't process Simple Desc with >%u clusters\n",
					WALKER_MAX_CLUST);

				walker.status = WALKER_ERROR;
				return WPAN_CONVERSATION_END;
			}

			// copy clusters to the list, converting to host byte order
			while (count--)
			{
				*cluster++ = le16toh( *cluster_le++);
			}
		}

		#ifdef WALKER_DEBUG
			hex_dump( response, envelope->length, HEX_DUMP_FLAG_TAB);
		#endif

		// At this point, cluster_le should point to just past the payload
		overparse = (uint8_t FAR *)cluster_le -
			((uint8_t FAR *)envelope->payload + envelope->length);
		if (overparse)
		{
			printf( "error parsing simple descriptor, overparse = %d\n",
				(int) overparse);

			walker.status = WALKER_ERROR;
		}
		else
		{
			walker.status = WALKER_DISCOVER_ATTR_REQ;
		}
	}

	return WPAN_CONVERSATION_END;
}

enum walker_status_t walker_send_simple_desc_req( void)
{
	wpan_envelope_t envelope;

	while (++walker.current_endpoint != WPAN_ENDPOINT_BROADCAST)
	{
		if (WALKER_EP_IS_SET( walker, walker.current_endpoint))
		{
			#ifdef WALKER_DEBUG
				printf( "sending Simple Desc request for ep 0x%02X\n",
					walker.current_endpoint);
			#endif
			wpan_envelope_create( &envelope, walker.dev, &walker.target.ieee,
				walker.target.network);
			if (zdo_simple_desc_request( &envelope, walker.target.network,
				walker.current_endpoint, walker_simple_desc_resp, NULL) == 0)
			{
				return WALKER_WAIT;
			}

			puts( "failed to send Simple Desc Req");
			return WALKER_ERROR;
		}
	}

	return WALKER_DONE;
}

int walker_process_discover_attr_resp( wpan_conversation_t FAR *conversation,
	const wpan_envelope_t FAR *envelope)
{
	zcl_command_t										zcl;

	// standard wpan_response_fn, but we don't need to look at the conversation
	XBEE_UNUSED_PARAMETER( conversation);

	if (envelope == NULL)
	{
		// timeout on discover, now what?
		puts( "Error: Timeout waiting for discover attributes response");
		walker.status = WALKER_ERROR;
	}
	else if (zcl_command_build( &zcl, envelope, NULL) == 0)
	{
		const zcl_discover_attrib_resp_t		FAR	*resp;
		const uint8_t 								FAR	*end;
		const zcl_rec_attrib_report_t			FAR	*attr;

		if (zcl.command == ZCL_CMD_DISCOVER_ATTRIB_RESP)
		{
			// reset the attribute information
			memset( &walker.attribute, 0, sizeof walker.attribute);
			resp = zcl.zcl_payload;
			attr = resp->attrib;
			walker.attribute.discovery_complete = resp->discovery_complete;
			if (walker.attribute.discovery_complete == 0 && zcl.length == 1)
			{
				puts( "ERROR: Discover Attribute Resp has 0 attributes, "
					"but not marked complete.");
				walker.attribute.discovery_complete = 1;
			}
			else
			{
				end = (const uint8_t FAR *)zcl.zcl_payload + zcl.length;
				while ((const uint8_t FAR *)attr < end)
				{
					walker.attribute.list[walker.attribute.count++] = *attr;
					++attr;
				}
			}
		}
		else
		{
			if (zcl.command == ZCL_CMD_DEFAULT_RESP)
			{
				// must not be any attributes (Ember responds with FAILURE)
			}
			else
			{
				printf( "unexepected response 0x%02X to Read Attributes Request\n",
					zcl.command);
				wpan_envelope_dump( envelope);
			}
			walker.attribute.discovery_complete = 1;
		}

		// read first attribute
		walker.status = WALKER_READ_ATTR_REQ;
	}

	return WPAN_CONVERSATION_END;
}

void walker_zcl_init( wpan_envelope_t *envelope,
	zcl_header_nomfg_t *zcl_header, uint16_t zcl_length)
{
	wpan_envelope_create( envelope, walker.dev, &walker.target.ieee,
		walker.target.network);

	envelope->payload = zcl_header;
	envelope->length = zcl_length;
	envelope->profile_id = walker.profile_id;
	envelope->source_endpoint = SAMPLE_ENDPOINT;
	envelope->dest_endpoint = walker.current_endpoint;
	envelope->cluster_id = walker.cluster.list[walker.cluster.index];

	zcl_header->frame_control = ZCL_FRAME_CLIENT_TO_SERVER
											| ZCL_FRAME_TYPE_PROFILE
											| ZCL_FRAME_GENERAL
											| ZCL_FRAME_DISABLE_DEF_RESP;
	// toggle direction bit if this is SERVER_TO_CLIENT
	if (walker.cluster.index >= walker.cluster.out_index)
	{
		zcl_header->frame_control ^= ZCL_FRAME_DIRECTION;
	}
}

// TODO: need a blacklist (or whitelist?) of profile IDs that it's OK to
// send ZCL commands to.  For example, we should not send ZCL commands to
// the Digi Profile.
enum walker_status_t walker_send_discover_attr_req( void)
{
	wpan_envelope_t envelope;
	PACKED_STRUCT {
		zcl_header_nomfg_t		header;
		zcl_discover_attrib_t	req;
	} zcl;
	int trans;

	if (walker.profile_id == WPAN_PROFILE_DIGI)
	{
		// Digi Profile is not ZCL, so skip ZCL discovery and go to next ep
		puts( "  Not a ZCL endpoint; skipping attribute discovery");
		walker_dump_cluster_list();

		return WALKER_SIMPLE_DESC_REQ;
	}

	if (walker.attribute.discovery_complete)
	{
		if (walker.attribute.count == 0)
		{
			puts( "    no attributes");
		}

		memset( &walker.attribute, 0, sizeof walker.attribute);

		// move on to the next cluster
		if (++walker.cluster.index == walker.cluster.count)
		{
			// move on to the next endpoint
			return WALKER_SIMPLE_DESC_REQ;
		}
	}

	walker_zcl_init( &envelope, &zcl.header, sizeof zcl);
	zcl.header.command = ZCL_CMD_DISCOVER_ATTRIB;

	// modify our endpoint and cluster table to match the target
	sample_endpoints.zcl.profile_id = envelope.profile_id;
	zcl_cluster_table[0].cluster_id = envelope.cluster_id;

	trans = wpan_conversation_register( &zcl_ep_state,
		walker_process_discover_attr_resp, NULL, WALKER_ZCL_TIMEOUT);

	if (trans < 0)
	{
		return walker.status;
	}

	zcl.header.sequence = (uint8_t) trans;

	if (walker.attribute.count == 0)
	{
		// this is the first request, so start at 0 and print header
		zcl.req.start_attrib_id_le = 0x0000;
		walker_print_cluster( walker.cluster.index);
	}
	else
	{
		// this is a followup request, so take last attribute from previous
		// response and add 1 to it to get the starting attribute ID
		zcl.req.start_attrib_id_le = htole16( 1 +
				le16toh( walker.attribute.list[walker.attribute.count - 1].id_le));
	}
	zcl.req.max_return_count = 20;

	if (wpan_envelope_send( &envelope) != 0)
	{
		return walker.status;
	}

	return WALKER_WAIT;
}

void walker_print_attribute_header( const zcl_rec_attrib_report_t *attr)
{
	printf( "      attr 0x%04X, type 0x%02X (%s) ",
		le16toh( attr->id_le), attr->type, zcl_type_name( attr->type));
}

int walker_process_read_attr_resp( wpan_conversation_t FAR *conversation,
	const wpan_envelope_t FAR *envelope)
{
	zcl_rec_attrib_report_t				FAR	*attr;		// attribute requested
	zcl_command_t									zcl;
	const zcl_rec_read_attrib_resp_t	FAR	*resp;

	attr = conversation->context;
	walker_print_attribute_header( attr);

	if (envelope == NULL)
	{
		puts( " Read Timeout");
	}
	else if (zcl_command_build( &zcl, envelope, NULL) == 0)
	{
		resp = zcl.zcl_payload;

		// make sure the response is for the correct ID
		if (resp->id_le != attr->id_le)
		{
			printf( "\nERROR: Read Attr Resp has wrong ID (0x%04x)\n",
				le16toh( resp->id_le));
		}
		else if (resp->status != ZCL_STATUS_SUCCESS)
		{
			printf( " Read Error: %s\n", zcl_status_text( resp->status));
		}
		else
		{
			zcl_print_attribute_value( resp->type, resp->value,
				zcl.length - offsetof( zcl_rec_read_attrib_resp_t, value));

			// the response should match the ID and TYPE of the one we requested
			if (resp->type != attr->type)
			{
				printf( "WARNING: Read Attr Resp had different type (0x%02x)",
					resp->type);
			}
		}
	}

	// read next attribute
	walker.status = WALKER_READ_ATTR_REQ;

	return WPAN_CONVERSATION_END;
}

int walker_send_read_attr( zcl_rec_attrib_report_t *attr)
{
	wpan_envelope_t envelope;
	PACKED_STRUCT {
		zcl_header_nomfg_t		header;
		uint16_t						attr_id_le;
	} zcl;
	int trans;
	int retval;

	walker_zcl_init( &envelope, &zcl.header, sizeof zcl);
	zcl.header.command = ZCL_CMD_READ_ATTRIB;

	// Endpoint's profile_id and cluster's ID still set from Discover Attributes

	trans = wpan_conversation_register( &zcl_ep_state,
		walker_process_read_attr_resp, attr, WALKER_ZCL_TIMEOUT);

	if (trans < 0)
	{
		return trans;
	}

	zcl.header.sequence = (uint8_t) trans;
	zcl.attr_id_le = attr->id_le;

	retval = wpan_envelope_send( &envelope);

	if (retval != 0)
	{
		// TODO: find a way to delete the conversation here!
	}

	return retval;
}

enum walker_status_t walker_read_attribute_req( void)
{
	zcl_rec_attrib_report_t *attr;
	enum walker_status_t new_state = WALKER_READ_ATTR_REQ;

	if (walker.attribute.index == walker.attribute.count)
	{
		// done walking this collection, go get more
		return WALKER_DISCOVER_ATTR_REQ;
	}

	attr = &walker.attribute.list[walker.attribute.index];
	if (1)				// option to read each attribute
	{
		if (walker_send_read_attr( attr) != 0)
		{
			// send failed, don't advance state machine
			return WALKER_READ_ATTR_REQ;
		}
		new_state = WALKER_WAIT;
	}
	else
	{
		walker_print_attribute_header( attr);
		// append a newline to the attribute id/type since there
		// isn't any additional information to display
		puts( "");
	}

	++walker.attribute.index;
	return new_state;
}

enum walker_status_t walker_tick( void)
{
	enum walker_status_t oldstatus;
	wpan_envelope_t envelope;

	oldstatus = walker.status;

	switch (walker.status)
	{
		case WALKER_INIT:
			if (walker.target.network == WPAN_NET_ADDR_UNDEFINED)
			{
				// need to get network address before sending Active EP req
				walker.status = WALKER_NWK_ADDR_REQ;
			}
			else
			{
				// send ZDO Active Endpoint Request
				walker.status = WALKER_ACTIVE_EP_REQ;
			}
			break;

		case WALKER_NWK_ADDR_REQ:
			// Use NWK_addr request to get network address
			if (zdo_send_nwk_addr_req( walker.dev, &walker.target.ieee,
															&walker.target.network) == 0)
			{
				walker.status = WALKER_NWK_ADDR_RSP;
			}
			break;

		case WALKER_NWK_ADDR_RSP:
			// waiting for response to network addr request
			if (walker.target.network != ZDO_NET_ADDR_PENDING)
			{
				#ifdef WALKER_DEBUG
					printf( "got address of 0x%04X\n", walker.target.network);
				#endif
				if (walker.target.network == ZDO_NET_ADDR_TIMEOUT)
				{
					puts( "Timeout waiting for Network Address Response.");
					walker.status = WALKER_ERROR;
				}
				else if (walker.target.network == ZDO_NET_ADDR_ERROR)
				{
					puts( "Error waiting for Network Address Response.");
					walker.status = WALKER_ERROR;
				}
				else
				{
					walker.status = WALKER_ACTIVE_EP_REQ;
				}
			}
			break;

		case WALKER_ACTIVE_EP_REQ:
			// send an active endpoints request
			wpan_envelope_create( &envelope, walker.dev, &walker.target.ieee,
				walker.target.network);
			if (zdo_send_descriptor_req( &envelope, ZDO_ACTIVE_EP_REQ,
				walker.target.network, walker_active_ep_resp, NULL) == 0)
			{
				walker.status = WALKER_WAIT;
			}
			break;

		case WALKER_SIMPLE_DESC_REQ:
			walker.status = walker_send_simple_desc_req();
			break;

		case WALKER_DISCOVER_ATTR_REQ:
			walker.status = walker_send_discover_attr_req();
			break;

		case WALKER_READ_ATTR_REQ:
			walker.status = walker_read_attribute_req();
			break;

		case WALKER_WAIT:
		case WALKER_DONE:
		case WALKER_ERROR:
			break;
	}

	if (oldstatus != walker.status)
	{
		#ifdef WALKER_DEBUG
			printf( "walker status changed from %u to %u\n", oldstatus,
				walker.status);
		#endif
		if (walker.status == WALKER_DONE)
		{
			walker.stop = time( NULL);
			printf( "Completed at %s", ctime( &walker.stop));
			printf( "(%" PRIu32 " seconds elapsed)\n",
				(uint32_t) (walker.stop - walker.start));
		}
	}

	return walker.status;
}

