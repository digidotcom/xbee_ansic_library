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

/**
	@addtogroup zcl_time
	@{
	@file zcl_time.c
	Client and server code to implement the ZigBee Time Cluster.

	This is an attribute-only cluster, so it could use the General Command
	handler in its cluster definition, but we use a special client to
	intercept Read Attribute Responses so we don't take up an entry in the
	conversation table.
*/

/*** BeginHeader */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "xbee/platform.h"
#include "xbee/byteorder.h"
#include "zigbee/zcl_time.h"
#include "zigbee/zdo.h"

#ifndef __DC__
	#define _zcl_time_debug
#elif defined ZCL_TIME_DEBUG
	#define _zcl_time_debug		__debug
#else
	#define _zcl_time_debug		__nodebug
#endif
/*** EndHeader */

/*** BeginHeader zcl_time_time, zcl_time_timestatus */
extern zcl_utctime_t	zcl_time_time;
extern uint8_t			zcl_time_timestatus;
/*** EndHeader */
/// Current value of Time Attribute (#ZCL_TIME_ATTR_TIME), start value
/// doesn't matter since it is set by _zcl_time_time_get().
zcl_utctime_t	zcl_time_time;

/// Current value of TimeStatus Attribute (#ZCL_TIME_ATTR_TIME_STATUS)
#ifdef ZCL_ENABLE_TIME_SERVER
	uint8_t	zcl_time_timestatus = ZCL_TIME_STATUS_MASTER;
#else
	uint8_t	zcl_time_timestatus = 0;
#endif

/*** BeginHeader _zcl_time_time_set, _zcl_time_time_get */
int _zcl_time_time_set( const zcl_attribute_full_t FAR *attribute,
	zcl_attribute_write_rec_t *rec);
uint_fast8_t _zcl_time_time_get( const zcl_attribute_full_t FAR *attribute);
/*** EndHeader */
/// difference between xbee_seconds_timer() and actual calendar time
int32_t zcl_time_skew = 0;

/**
	@internal
	@brief
	Function registered to #ZCL_TIME_ATTR_TIME attribute of Time Cluster and
	called to modify the attribute.  Also called internally to update the
	zcl_time_skew global.

	@param[in]		attribute	ignored; assumed to point to zcl_time_time
	@param[in,out]	rec			if NULL, function was called internally to update
										zcl_time_skew only

	See zcl_attribute_write_fn() for calling convention.
*/
_zcl_time_debug
int _zcl_time_time_set( const zcl_attribute_full_t FAR *attribute,
	zcl_attribute_write_rec_t *rec)
{
	int16_t bytes_read = 0;

	// decode using standard method
	if (rec)
	{
		// if this device is a MASTER, Time is read-only
		if (zcl_time_timestatus & ZCL_TIME_STATUS_MASTER)
		{
			rec->status = ZCL_STATUS_READ_ONLY;
			rec->buffer += 4;
			return 4;
		}

		bytes_read = zcl_decode_attribute( &attribute->base, rec);
		if (! (rec->flags & ZCL_ATTR_WRITE_FLAG_ASSIGN))
		{
			return bytes_read;
		}
	}

	zcl_time_skew = zcl_time_time -
							(xbee_seconds_timer() - ZCL_TIME_EPOCH_DELTA);

	#ifdef ZCL_TIME_VERBOSE
		printf( "%s: setting time to 0x%" PRIx32 "; skew is %" PRId32 " sec\n",
			__FUNCTION__, zcl_time_time, zcl_time_skew);
	#endif

	return bytes_read;
}

/**
	@internal
	@brief
	Function registered to #ZCL_TIME_ATTR_TIME attribute of Time Cluster and
	called to refresh the Time attribute (global zcl_time_time).

	See zcl_attribute_update_fn() for calling convention.
*/
_zcl_time_debug
uint_fast8_t _zcl_time_time_get( const zcl_attribute_full_t FAR *attribute)
{
	// zcl_attribute_update_fn API, but 'attribute' parameter's value is always
	// a pointer to 'zcl_time_time' variable.
	XBEE_UNUSED_PARAMETER( attribute);

#if ZCL_TIME_EPOCH_DELTA > 0
	if (! zcl_time_skew && xbee_seconds_timer() < ZCL_TIME_EPOCH_DELTA)
	{
		// we don't really know what time it is...
		zcl_time_time = ZCL_UTCTIME_INVALID;
	}
	else
#endif
	{
		zcl_time_time = zcl_time_skew +
								(xbee_seconds_timer() - ZCL_TIME_EPOCH_DELTA);
	}

	#ifdef ZCL_TIME_VERBOSE
		printf( "%s: read clock & updated zcl_time_time to 0x%" PRIx32 "\n",
			__FUNCTION__, zcl_time_time);
	#endif

	return ZCL_STATUS_SUCCESS;
}

/*** BeginHeader zcl_time_attribute_tree */
/*** EndHeader */
/**
	@internal
	@brief
	Special code for setting bitfield of TimeStatus attribute.

	See ZCL Spec section 3.12.2.2.2 for rules on setting values in this field.

	Only used if device has a Time Server.
*/
_zcl_time_debug
int _zcl_time_timestatus_set( const zcl_attribute_full_t FAR *attribute,
	zcl_attribute_write_rec_t *rec)
{
	uint8_t time_status;

	// zcl_attribute_write_fn API, but 'attribute' parameter's value is always
	// a pointer to 'time_status' variable.
	XBEE_UNUSED_PARAMETER( attribute);

	if (rec->flags & ZCL_ATTR_WRITE_FLAG_ASSIGN)
	{
		// Master and MasterZoneDst bits are not settable.
		// If Master is set to 1, Synchronized bit is always set to 0.

		time_status = *rec->buffer;
		// can only set the Synchronized bit if Master bit is not set
		if (!(zcl_time_timestatus & ZCL_TIME_STATUS_MASTER))
		{
			if (time_status & ZCL_TIME_STATUS_SYNCHRONIZED)
			{
				// set Synchronized bit
				zcl_time_timestatus |= ZCL_TIME_STATUS_SYNCHRONIZED;
			}
			else
			{
				// clear Synchronized bit
				zcl_time_timestatus &= ~ZCL_TIME_STATUS_SYNCHRONIZED;
			}
		}
	}

	return 1;
}

/** Global attribute list used in #ZCL_CLUST_ENTRY_TIME_SERVER and
	for adding a Time Cluster Server to an endpoint.  The Time Cluster
	Client does not have any attributes.
*/
const struct {
	zcl_attribute_full_t			time;
	zcl_attribute_full_t			time_status;
	uint16_t							end_of_list;
} zcl_time_attr =
{
//	  ID, Flags, Type, Address to data, min, max, read, write
	{ { ZCL_TIME_ATTR_TIME,
		ZCL_ATTRIB_FLAG_FULL,
		ZCL_TYPE_TIME_UTCTIME,
		&zcl_time_time },
		{ 0 }, { 0 },
		&_zcl_time_time_get, &_zcl_time_time_set },
	{ { ZCL_TIME_ATTR_TIME_STATUS,
		ZCL_ATTRIB_FLAG_FULL,
		ZCL_TYPE_BITMAP_8BIT,
		&zcl_time_timestatus },
		{ 0 }, { 0 },
		NULL, &_zcl_time_timestatus_set },
	ZCL_ATTRIBUTE_END_OF_LIST
};

const zcl_attribute_tree_t zcl_time_attribute_tree[] =
		{ { ZCL_MFG_NONE, &zcl_time_attr.time.base, NULL } };

/*** BeginHeader zcl_time_now */
/*** EndHeader */
/**
	@brief
	Returns the current date/time, using the ZCL epoch of January 1, 2000.

	Assumes that device has connected to a time server and updated its clock
	accordingly.  Returns ZCL_UTCTIME_INVALID if the device has not synchronized
	its clock.

	Do not use this value for tracking elapsed time -- use xbee_seconds_timer()
	or xbee_millisecond_timer() instead.  The value may jump forward (or even
	backward) when the device decides to synchronize with a time server.

	@retval	ZCL_UTCTIME_INVALID		Clock not synchronized to a time source.
	@retval	0-0xFFFFFFFE	Number of elapsed seconds since midnight UTC on
									January 1, 2000.

	@sa xbee_seconds_timer, xbee_millisecond_timer
*/
zcl_utctime_t zcl_time_now( void)
{
	// Confirm that we are a master or have synchronized with one.
	if (zcl_time_timestatus
		& (ZCL_TIME_STATUS_MASTER | ZCL_TIME_STATUS_SYNCHRONIZED))
	{
		_zcl_time_time_get( NULL);
		return zcl_time_time;
	}
	else
	{
		return ZCL_UTCTIME_INVALID;
	}
}

/*** BeginHeader zcl_time_client */
/*** EndHeader */

/**
	@brief
	Handle Read Attribute Responses to requests sent as part of the
	zcl_time_find_servers() process.

	This function expects to receive
	"read attribute response" packets ONLY for reads of Time and TimeStatus.

	If responding device is a master or is synchronized with one, use it's Time
	value to update a "skew" global used to track the offset between system time
	(which may just be "seconds of uptime") and calendar time.

	@param[in]	envelope	envelope from received message
	@param[in]	context	pointer to attribute list for cluster
								(typically passed in via endpoint dispatcher)

	@retval	0	Command was processed and default response (with either success
					or error status) was sent.

	@retval	!0	Error sending default response; time may or may not have been
					set.

*/
// TODO: make use of zcl_process_read_attr_response() in zcl_client.c to parse
// Read Attributes Response and populate a temporary attribute list with the
// values.
_zcl_time_debug
int zcl_time_client( const wpan_envelope_t FAR *envelope, void FAR *context)
{
	zcl_command_t	zcl;

	// We're only expecting Read Attribute Responses.
	// Make sure frame is server-to-client, not manufacturer-specific and
	// a profile (not cluster) command.
	if (zcl_command_build( &zcl, envelope, context) == 0 &&
		zcl.command == ZCL_CMD_READ_ATTRIB_RESP &&
		ZCL_CMD_MATCH( &zcl.frame_control, GENERAL, SERVER_TO_CLIENT, PROFILE))
	{
		const zcl_header_t			FAR *header = envelope->payload;
		union {
			const uint8_t	FAR *u8;
			const uint16_t	FAR *u16_le;
			const uint32_t	FAR *u32_le;
		} walk;									// used to walk the payload
		uint8_t		FAR *payload_end;
		uint16_t			attribute_id;
		uint8_t			attribute_type;
		zcl_utctime_t	time = 0;
		uint8_t			time_status = 0;
		uint8_t			response = ZCL_STATUS_SUCCESS;

		#ifdef ZCL_TIME_VERBOSE
			printf( "%s: %d-byte payload to time client\n", __FUNCTION__,
				envelope->length);
			hex_dump( envelope->payload, envelope->length, HEX_DUMP_FLAG_TAB);
		#endif

		payload_end = ((uint8_t FAR *)envelope->payload) + envelope->length;
		walk.u8 = header->type.std.common.payload;
		while (response == ZCL_STATUS_SUCCESS && walk.u8 < payload_end)
		{
			attribute_id = le16toh( *walk.u16_le++);
			if (*walk.u8++ == ZCL_STATUS_SUCCESS)
			{
				attribute_type = *walk.u8++;
				if (attribute_id == ZCL_TIME_ATTR_TIME)
				{
					if (attribute_type != ZCL_TYPE_TIME_UTCTIME)
					{
						response = ZCL_STATUS_INVALID_DATA_TYPE;
					}
					else
					{
						time = le32toh( *walk.u32_le++);
					}
				}
				else if (attribute_id == ZCL_TIME_ATTR_TIME_STATUS)
				{
					if (attribute_type != ZCL_TYPE_BITMAP_8BIT)
					{
						response = ZCL_STATUS_INVALID_DATA_TYPE;
					}
					else
					{
						time_status = *walk.u8++;
					}
				}
				else
				{
					// unexpected attribute in response
					response = ZCL_STATUS_UNSUPPORTED_ATTRIBUTE;
				}
			}
		}

		// didn't get a valid time in response
		if (response == ZCL_STATUS_SUCCESS && ! time)
		{
			#ifdef ZCL_TIME_VERBOSE
				printf( "%s: response did not contain a valid time\n",
					__FUNCTION__);
			#endif
			response = ZCL_STATUS_FAILURE;
		}

		if (response == ZCL_STATUS_SUCCESS)
		{
			#ifdef ZCL_TIME_VERBOSE
				printf( "%s: the time is %" PRIu32 "\n", __FUNCTION__, time);
			#endif

			// only set the clock if the response has a valid time
			if (time != ZCL_UTCTIME_INVALID && (time_status &
				(ZCL_TIME_STATUS_MASTER | ZCL_TIME_STATUS_SYNCHRONIZED)) != 0)
			{
				zcl_time_time = time;
				_zcl_time_time_set( NULL, NULL);

				// set Synchronized bit of our TimeStatus
				zcl_time_timestatus |= ZCL_TIME_STATUS_SYNCHRONIZED;
			}
		}

		return zcl_default_response( &zcl, response);
	}

	// command not handled by this function, try general command handler
	return zcl_general_command( envelope, context);
}

/*** BeginHeader zcl_time_find_servers */
/*** EndHeader */

#include "zigbee/zcl_client.h"
/**
	@brief
	Find Time Servers on the network, query them for the current time and then
	synchronize this device's clock to that time.

	@note	This function uses a static buffer to hold context information for
			the ZDO responder that generates the ZCL Read Attributes request.
			Wait at least 60 seconds between calls to allow for earlier requests
			to time out.

	@note This function will only work correctly if the Time Cluster Client
			in your endpoint table is using the zcl_time_client() function as
			its callback handler.  If you use the ZCL_CLUST_ENTRY_TIME_CLIENT
			or ZCL_CLUST_ENTRY_TIME_BOTH macro, the client cluster is set up
			correctly.

	@param[in]	dev	device to send query on
	@param[in]	profile_id	profile ID to match in endpoint table or
									#WPAN_APS_PROFILE_ANY to use the first endpoint
									with a Time Cluster Client

	@retval	0	Successfully issued ZDO Match Descriptor Request to find Time
					Cluster Servers on the network.  No guarantee that we'll get
					a response.
	@retval	!0	Some sort of error occurred in generating or sending the
					ZDO Match Descriptor Request.
	@retval	-EINVAL	Couldn't find a Time Cluster Client with \p profile_id
							in the endpoint table of \p dev.
*/
int zcl_time_find_servers( wpan_dev_t *dev, uint16_t profile_id)
{
	// Might as well keep this const -- fewer bytes to have a const table than
	// to have code to build it in RAM.  If not const, still needs const table
	// to use for initializing a list in RAM.
	static const uint16_t clusters[] =
										{ ZCL_CLUST_TIME, WPAN_CLUSTER_END_OF_LIST };

	// These structures are static since they must persist after this function
	// ends.  They're used as the context for a callback, and the function that
	// generages a ZCL Read Attributes Request from the ZDO Match Descriptor
	// response needs the contents of this structure.
	static const uint16_t attributes[] =
		{
			ZCL_TIME_ATTR_TIME,
			ZCL_TIME_ATTR_TIME_STATUS,
			ZCL_ATTRIBUTE_END_OF_LIST
		};
	// This structure can't be const, since we fill in the endpoint from the
	// device's endpoint table.
	static zcl_client_read_t client_read =
		{
			NULL,					// fill in with endpoint passed to function
			ZCL_MFG_NONE,		// part of ZCL, not a manufacturer-specific cluster
			ZCL_CLUST_TIME,	// cluster ID
			attributes			// attributes to request
		};

	// Find a Time Cluster Client with the correct profile ID
	client_read.ep = wpan_endpoint_of_cluster( dev, profile_id,
				ZCL_CLUST_TIME, WPAN_CLUST_FLAG_CLIENT);
	if (client_read.ep == NULL)
	{
		return -EINVAL;
	}

	return zcl_find_and_read_attributes( dev, clusters, &client_read);
}
