/*
 * Copyright (c) 2011-2012 Digi International Inc.,
 * All rights not expressly granted are reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
 * =======================================================================
 */

/**
	@addtogroup xbee_discovery
	@{
	@file xbee_discovery.c

	Code related to "Node Discovery" (the ATND command, 0x95 frames).  See full
	documentation in xbee/discovery.h.
*/

/*** BeginHeader */
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "xbee/platform.h"
#include "xbee/atcmd.h"
#include "xbee/byteorder.h"
#include "xbee/discovery.h"
/*** EndHeader */

/*** BeginHeader xbee_disc_nd_parse */
/*** EndHeader */
// documented in xbee/discovery.h
// @todo make use of the source_length parameter and return -EBADMSG if
// it isn't long enough (ensure it's long enough to include the device_type
// field)
int xbee_disc_nd_parse( xbee_node_id_t FAR *parsed, const void FAR *source,
		int source_length)
{
	size_t ni_len;
	const xbee_node_id1_t FAR *id1;
	const xbee_node_id2_t FAR *id2;

	if (parsed == NULL || source == NULL)
	{
		return -EINVAL;
	}

	id1 = source;
	ni_len = strlen( id1->node_info);
	if (ni_len > XBEE_DISC_MAX_NODEID_LEN)
	{
		return -EBADMSG;
	}

	// xbee_node_id2_t structure follows null terminator of id1->node_info
	id2 = (const xbee_node_id2_t FAR *) &id1->node_info[ni_len + 1];
	parsed->ieee_addr_be = id1->ieee_addr_be;
	parsed->network_addr = be16toh( id1->network_addr_be);
	parsed->parent_addr = be16toh( id2->parent_addr_be);
	parsed->device_type = id2->device_type;
	_f_memcpy( parsed->node_info, id1->node_info, ni_len);
	// null terminate node_info and clear any other unused bytes
	_f_memset( parsed->node_info + ni_len, 0, sizeof parsed->node_info - ni_len);

	return 0;
}


/*** BeginHeader xbee_disc_device_type_str */
/*** EndHeader */
// documented in xbee/discovery.h
const char *xbee_disc_device_type_str( uint8_t device_type)
{
	static const char *_xbee_disc_device_type[] =
		{ "Coord", "Router", "EndDev", "???" };
	if (device_type > XBEE_ND_DEVICE_TYPE_ENDDEV)
	{
		device_type = XBEE_ND_DEVICE_TYPE_ENDDEV + 1;
	}

	return _xbee_disc_device_type[device_type];
}


/*** BeginHeader xbee_disc_node_id_dump */
/*** EndHeader */
// documented in xbee/discovery.h
void xbee_disc_node_id_dump( const xbee_node_id_t FAR *ni)
{
	if (ni != NULL)
	{
		printf( "Address:%08" PRIx32 "-%08" PRIx32 " 0x%04x  "
			"PARENT:0x%04x  %6s  NI:[%" PRIsFAR "]\n",
			be32toh( ni->ieee_addr_be.l[0]), be32toh( ni->ieee_addr_be.l[1]),
			ni->network_addr, ni->parent_addr,
			xbee_disc_device_type_str( ni->device_type), ni->node_info);

	}
}


/*** BeginHeader xbee_disc_add_node_id_handler */
/*** EndHeader */
// documented in xbee/discovery.h
int xbee_disc_add_node_id_handler( xbee_dev_t *xbee, xbee_disc_node_id_fn fn)
{
	if (xbee == NULL || fn == NULL)
	{
		return -EINVAL;
	}

	if (xbee->node_id_handler != NULL)
	{
		return -ENOSPC;
	}

	xbee->node_id_handler = fn;
	return 0;
}


/*** BeginHeader xbee_disc_remove_node_id_handler */
/*** EndHeader */
// documented in xbee/discovery.h
int xbee_disc_remove_node_id_handler( xbee_dev_t *xbee,
		xbee_disc_node_id_fn fn)
{
	if (xbee == NULL || fn == NULL)
	{
		return -EINVAL;
	}

	if (xbee->node_id_handler != fn)
	{
		return -ENOENT;
	}

	xbee->node_id_handler = NULL;
	return 0;
}


/*** BeginHeader _xbee_disc_parse_and_pass */
int _xbee_disc_parse_and_pass( xbee_dev_t *xbee, const void FAR *node_data,
		int length);
/*** EndHeader */
/**
	@internal @brief
	Common function used to parse Node ID messages from various sources, and
	then pass them on to registered receivers.

	@param[in]	xbee			XBee device that received the message
	@param[in]	node_data	pointer to raw Node ID message
	@param[in]	length		length of Node ID message

	@retval	0				Node ID message parsed and passed on to any
								registered receivers
	@retval	-EINVAL		invalid parameter passed to function
	@retval	-EBADMSG		error parsing Node ID message

	@sa xbee_disc_nd_parse, xbee_disc_add_node_id_handler,
		xbee_disc_remove_node_id_handler
*/
int _xbee_disc_parse_and_pass( xbee_dev_t *xbee, const void FAR *node_data,
		int length)
{
	int retval;
	xbee_node_id_t node_id;

	retval = xbee_disc_nd_parse( &node_id, node_data, length);
	if (retval == 0 && xbee != NULL && xbee->node_id_handler != NULL)
	{
		xbee->node_id_handler( xbee, &node_id);
	}

	return retval;
}


/*** BeginHeader xbee_disc_nodeid_cluster_handler */
/*** EndHeader */
// documented in xbee/discovery.h
int xbee_disc_nodeid_cluster_handler( const wpan_envelope_t FAR *envelope,
	void FAR *context)
{
	XBEE_UNUSED_PARAMETER( context);

	if (envelope == NULL)
	{
		return -EINVAL;
	}

	// Safe to cast the wpan_dev_t back to xbee_dev_t, since it's the first
	// member of the xbee_dev_t structure.
	return _xbee_disc_parse_and_pass( (xbee_dev_t *) envelope->dev,
			envelope->payload, envelope->length);
}


/*** BeginHeader xbee_disc_atnd_response_handler */
/*** EndHeader */
// documented in xbee/discovery.h
int xbee_disc_atnd_response_handler( xbee_dev_t *xbee, const void FAR *raw,
		uint16_t length, void FAR *context)
{
	static const xbee_at_cmd_t nd = {{'N', 'D'}};
	const xbee_frame_local_at_resp_t FAR *resp = raw;

	XBEE_UNUSED_PARAMETER( context);

	if (resp == NULL)
	{
		return -EINVAL;
	}

	if (resp->header.command.w == nd.w)
	{
		if (XBEE_AT_RESP_STATUS( resp->header.status) == XBEE_AT_RESP_SUCCESS)
		{
			// this is a successful ATND response, parse the response
			// and add the discovered node to our node table
			return _xbee_disc_parse_and_pass( xbee, resp->value,
					length - offsetof( xbee_frame_local_at_resp_t, value));
		}
		else if (XBEE_AT_RESP_STATUS( resp->header.status) == XBEE_AT_RESP_ERROR)
		{
			// this is an unsuccessful ATND response, pass a NULL node_id to
			// indicate that the NI request timed out.
			if (xbee != NULL && xbee->node_id_handler != NULL) 
			{
				xbee->node_id_handler( xbee, NULL);
			}
		}
	}

	return 0;
}


/*** BeginHeader xbee_disc_nodeid_frame_handler */
/*** EndHeader */
// documented in xbee/discovery.h
int xbee_disc_nodeid_frame_handler( xbee_dev_t *xbee, const void FAR *raw,
		uint16_t length, void FAR *context)
{
	const xbee_frame_node_id_t FAR *frame = raw;

	XBEE_UNUSED_PARAMETER( context);

	if (frame == NULL)
	{
		return -EINVAL;
	}

	return _xbee_disc_parse_and_pass( xbee, &frame->node_data,
			length - offsetof( xbee_frame_node_id_t, node_data));
}


/*** BeginHeader xbee_disc_discover_nodes */
/*** EndHeader */
// documented in xbee/discovery.h
int xbee_disc_discover_nodes( xbee_dev_t *xbee, const char *identifier)
{
	size_t ni_len;

	ni_len = (identifier == NULL) ? 0 : strlen( identifier);
	if (ni_len > XBEE_DISC_MAX_NODEID_LEN)
	{
		return -EINVAL;
	}

	// We don't have to check (xbee == NULL) since xbee_cmd_execute() does so.
	return xbee_cmd_execute( xbee, "ND", identifier, (uint8_t) ni_len);
}
