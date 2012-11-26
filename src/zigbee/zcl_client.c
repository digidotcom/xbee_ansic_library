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
	@addtogroup zcl_client
	@{
	@file zcl_client.c

	Code to support ZCL client clusters.
*/

/*** BeginHeader */
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>

#include "xbee/platform.h"
#include "xbee/time.h"
#include "zigbee/zdo.h"
#include "zigbee/zcl.h"
#include "zigbee/zcl_client.h"

#ifndef __DC__
	#define zcl_client_debug
#elif defined ZCL_CLIENT_DEBUG
	#define zcl_client_debug		__debug
#else
	#define zcl_client_debug		__nodebug
#endif
/*** EndHeader */

/*** BeginHeader zcl_process_read_attr_response */
/*** EndHeader */
/**
	@brief
	Process a Read Attributes Response and populate the attibute values into
	an attribute table as if it was a Write Attributes Request.

	Used in ZCL clients that want to read a lot of ZCL attributes.  The
	client has a mirrored copy of the attributes on the target, and this
	function is used to populate that copy using the Read Attributes Responses.

	@param[in]	zcl	ZCL command to process
	@param[in]	attr_list	start of the attribute list to use for storing
									attribute responses

	@return	ZCL status value to send in a default response
*/
zcl_client_debug
int zcl_process_read_attr_response( zcl_command_t *zcl,
	const zcl_attribute_base_t FAR *attr_table)
{
	const uint8_t					FAR *payload_end;
	zcl_attribute_write_rec_t write_rec;

	if (zcl == NULL || attr_table == NULL)
	{
		return ZCL_STATUS_FAILURE;
	}

	write_rec.buffer = zcl->zcl_payload;
	payload_end = write_rec.buffer + zcl->length;
	write_rec.status = ZCL_STATUS_SUCCESS;
	while (write_rec.status == ZCL_STATUS_SUCCESS
			&& write_rec.buffer < payload_end)
	{
		write_rec.flags = ZCL_ATTR_WRITE_FLAG_ASSIGN
								| ZCL_ATTR_WRITE_FLAG_READ_RESP;
		write_rec.buflen = (int16_t)(payload_end - write_rec.buffer);
		zcl_parse_attribute_record( attr_table, &write_rec);
	}

	return write_rec.status;
}


/*** BeginHeader zcl_client_read_attributes */
/*** EndHeader */
/**
	@brief
	Send a Read Attributes request for attributes listed in \p client_read.

	@param[in,out]	envelope		addressing information for sending a ZCL Read
										Attributes request; this function fills in the
										\c payload and \c length
	@param[in]		client_read	data structure used by ZCL clients to do
										ZDO discovery followed by ZCL attribute reads

	@retval	0		successfully sent request
	@retval	!0		error sending request
*/
zcl_client_debug
int zcl_client_read_attributes( wpan_envelope_t FAR *envelope,
	const zcl_client_read_t *client_read)
{
   PACKED_STRUCT {
		zcl_header_response_t	header;
		uint16_t						attrib_le[20];
	} zcl_req;
	uint8_t							*request_start;
	const uint16_t			FAR *attrib_src;
	uint16_t							*attrib_dst_le;
	uint_fast8_t					i;

   // Generate a Read Attributes request for attributes in list
	if (client_read->mfg_id == ZCL_MFG_NONE)
	{
		zcl_req.header.u.std.frame_control = ZCL_FRAME_TYPE_PROFILE |
			ZCL_FRAME_GENERAL | ZCL_FRAME_CLIENT_TO_SERVER;
		request_start = &zcl_req.header.u.std.frame_control;
	}
	else
	{
		zcl_req.header.u.mfg.mfg_code_le = htole16( client_read->mfg_id);
		zcl_req.header.u.mfg.frame_control = ZCL_FRAME_TYPE_PROFILE |
			ZCL_FRAME_MFG_SPECIFIC | ZCL_FRAME_CLIENT_TO_SERVER;
		request_start = &zcl_req.header.u.mfg.frame_control;
	}
	zcl_req.header.sequence = wpan_endpoint_next_trans( client_read->ep);
	zcl_req.header.command = ZCL_CMD_READ_ATTRIB;

  // copy attribute list into request
	attrib_src = client_read->attribute_list;
	attrib_dst_le = zcl_req.attrib_le;
	for (i = 20; i && *attrib_src != ZCL_ATTRIBUTE_END_OF_LIST; --i)
	{
		*attrib_dst_le = htole16( *attrib_src);
		++attrib_dst_le;
		++attrib_src;
	}

	envelope->payload = request_start;
	envelope->length = (uint8_t *)attrib_dst_le - request_start;

	#ifdef ZCL_CLIENT_VERBOSE
		printf( "%s: sending %d-byte ZCL request\n", __FUNCTION__,
			envelope->length);
		wpan_envelope_dump( envelope);
	#endif

	return wpan_envelope_send( envelope);
}

/*** BeginHeader zcl_find_and_read_attributes */
/*** EndHeader */
/**
	@internal

	Process responses to ZDO Match_Desc broadcast sent by zdo_send_match_desc()
	and generate ZCL Read Attributes request for the attributes in the
	related conversation's context (a pointer to a zcl_client_read_t object).

	The Read Attributes Responses are handled by the function registered to the
	cluster in the endpoint table.

	@param[in]	conversation	matching entry in converstation table (which
										contains a context pointer)
	@param[in]	envelope			envelope of response or NULL if conversation
										timed out

	@retval	0			send ZCL request for current time
	@retval	!0			error sending ZCL request for current time

	@todo support a flag in the client_read object for sending ZCL request
			to first endpoint in list (current behavior) or all endpoints
			(for example, to query ALL meter endpoints on ALL radios).

	@todo find a way to track whether frame came in from broadcast or unicast
			response -- if unicast, can return WPAN_CONVERSATION_END
*/
zcl_client_debug
int _zcl_send_read_from_zdo_match( wpan_conversation_t FAR *conversation,
	const wpan_envelope_t FAR *envelope)
{
	const PACKED_STRUCT {
		uint8_t								transaction_id;
		zdo_match_desc_rsp_header_t	header;
		uint8_t								endpoint;
	}									FAR *zdo_rsp;
	zcl_client_read_t						client_read;
	wpan_envelope_t						reply_envelope;		// for sending ZCL request
	const wpan_cluster_table_entry_t *cluster;
	int										err;

	// note that conversation is never NULL in wpan_response_fn callbacks

	if (envelope == NULL)
	{
		// conversation timed out, that's OK.
		return WPAN_CONVERSATION_END;
	}

	zdo_rsp = envelope->payload;
	client_read = *(zcl_client_read_t FAR *)conversation->context;

	#ifdef ZCL_CLIENT_VERBOSE
		printf( "%s: generating ZCL request from ZDO response\n",
			__FUNCTION__);
		hex_dump( envelope->payload, envelope->length, HEX_DUMP_FLAG_TAB);
	#endif

	// Make sure this is a Match Descriptor Response.
	if (envelope->cluster_id != ZDO_MATCH_DESC_RSP)
	{
		#ifdef ZCL_CLIENT_VERBOSE
			printf( "%s: ignoring response (cluster=0x%04x, not MatchDescRsp)\n",
				__FUNCTION__, envelope->cluster_id);
		#endif
		return WPAN_CONVERSATION_CONTINUE;
	}

	// Only respond to successful responses with a length > 0
	if (zdo_rsp->header.status != ZDO_STATUS_SUCCESS
		|| zdo_rsp->header.match_length == 0)
	{
		#ifdef ZCL_CLIENT_VERBOSE
			printf( "%s: ignoring response (status=0x%02x, length=%u)\n",
				__FUNCTION__, zdo_rsp->header.status,
				zdo_rsp->header.match_length);
		#endif
		return WPAN_CONVERSATION_CONTINUE;
	}

	// Reply on the same device that received the response.
	err = wpan_envelope_reply( &reply_envelope, envelope);
	if (err != 0)
	{
		#ifdef ZCL_CLIENT_VERBOSE
			printf( "%s: %s returned %d\n", __FUNCTION__, "wpan_envelope_reply",
				err);
		#endif
		return WPAN_CONVERSATION_CONTINUE;
	}

	// assert that network_address == zdo_rsp->header.network_addr_le?
	if (reply_envelope.network_address !=
										le16toh( zdo_rsp->header.network_addr_le))
	{
		#ifdef ZCL_CLIENT_VERBOSE
			printf( "%s: ERROR net addr mismatch (env=0x%04x, zdo=0x%04x)\n",
				__FUNCTION__, reply_envelope.network_address,
				le16toh( zdo_rsp->header.network_addr_le));
		#endif
		return WPAN_CONVERSATION_CONTINUE;
		// DEVNOTE: exit here, or send request anyway?
	}

	// change source endpoint from ZDO (0) to selected ZCL client
	reply_envelope.source_endpoint = client_read.ep->endpoint;
	reply_envelope.profile_id = client_read.ep->profile_id;
	reply_envelope.cluster_id = client_read.cluster_id;

	// change dest endpoint to the one specified in the ZDO response
	reply_envelope.dest_endpoint = zdo_rsp->endpoint;

	// look up the cluster on the endpoint to set envelope options (like
	// the encryption flag)
	cluster = wpan_cluster_match( client_read.cluster_id,
		WPAN_CLUST_FLAG_CLIENT, client_read.ep->cluster_table);
	if (cluster)
	{
		reply_envelope.options = cluster->flags;
	}

   zcl_client_read_attributes( &reply_envelope, &client_read);

	return WPAN_CONVERSATION_CONTINUE;
}

/**
	@brief
	Use ZDO Match Descriptor Requests to find devices with a given
	profile/cluster and then automatically send a ZCL Read Attributes request
	for some of that cluster's attributes.

	@param[in]	dev			device to use for time request

	@param[in]	clusters		pointer to list of server clusters to search for,
									must end with #WPAN_CLUSTER_END_OF_LIST

	@param[in]	cr				zcl_client_read record containing information on
									the request (endpoint, attributes, etc.); must
									be a static object (not an auto variable) since
									the ZDO responder will need to access it

	@retval	0	request sent
	@retval	!0	error sending request
*/
zcl_client_debug
int zcl_find_and_read_attributes( wpan_dev_t *dev, const uint16_t *clusters,
	const zcl_client_read_t FAR *cr)
{
	return zdo_send_match_desc( dev, clusters, cr->ep->profile_id,
		_zcl_send_read_from_zdo_match, cr);
}


/*** BeginHeader zdo_send_match_desc */
/*** EndHeader */

/**
	@brief
	Send a ZDO Match Descriptor request for a list of cluster IDs.

	@param[in]	dev			device to use for time request
	@param[in]	clusters		pointer to list of server clusters to search for,
									must end with #WPAN_CLUSTER_END_OF_LIST
	@param[in]	profile_id	profile ID associated with the cluster IDs (cannot
									be WPAN_APS_PROFILE_ANY)
	@param[in]	callback		function that will process the ZDO Match Descriptor
									responses; see wpan_response_fn for the callback's
									API
	@param[in]	context		context to pass to \p callback in the
									wpan_conversation_t structure

	@retval	0	request sent
	@retval	!0	error sending request

	@sa zcl_find_and_read_attributes()

	@todo update API to allow for unicast request in addition to broadcast
*/
zcl_client_debug
int zdo_send_match_desc( wpan_dev_t *dev, const uint16_t *clusters,
	uint16_t profile_id, wpan_response_fn callback, const void FAR *context)
{
	wpan_envelope_t	envelope = { 0 };
	uint8_t				buffer[50];
	int					frame_length;
	int					trans;
	int					retval;

	if (dev == NULL || clusters == NULL || callback == NULL)
	{
		return -EINVAL;
	}

	// build the envelope -- use broadcast to find all matching devices
	envelope.dev = dev;
	envelope.ieee_address = _WPAN_IEEE_ADDR_BROADCAST;
	envelope.network_address = WPAN_NET_ADDR_UNDEFINED;

	// Sending a ZDO Match Descriptor request
	envelope.cluster_id = ZDO_MATCH_DESC_REQ;

	// build the request (return error if not enough space)
	frame_length = zdo_match_desc_request( buffer, sizeof(buffer),
		WPAN_NET_ADDR_BCAST_NOT_ASLEEP, profile_id, clusters, NULL);
	if (frame_length < 0)
	{
		#ifdef ZCL_CLIENT_VERBOSE
			printf( "%s: error %d calling %s\n",
				__FUNCTION__, frame_length, "zdo_match_desc_request");
		#endif
		return frame_length;
	}

	// register the conversation to get a transaction id
	trans = wpan_conversation_register( zdo_endpoint_state( dev), callback,
		context, 60);
	if (trans < 0)
	{
		#ifdef ZCL_CLIENT_VERBOSE
			printf( "%s: error %d calling %s\n",
				__FUNCTION__, trans, "wpan_conversation_register");
		#endif
		return trans;
	}
	// Copy the transaction ID into the request and send it.  Should I add these
	// three parameters to the conversation_register function?  Lots of
	// parameters to pass around!
	*buffer = (uint8_t)trans;
	envelope.payload = buffer;
	envelope.length = frame_length;

	#ifdef ZCL_CLIENT_VERBOSE
		printf( "%s: sending %u-byte ZDO Match Desc request\n", __FUNCTION__,
			frame_length);
		wpan_envelope_dump( &envelope);
	#endif

	retval = wpan_envelope_send( &envelope);
	#ifdef ZCL_CLIENT_VERBOSE
		if (retval)
		{
			printf( "%s: error %d calling %s\n",
				__FUNCTION__, retval, "wpan_envelope_send");
		}
	#endif

	return retval;
}

/*** BeginHeader zcl_create_attribute_records */
/*** EndHeader */
/**
	@brief
	From a list of attributes, write ID (in little-endian byte order), type
	and value to a buffer as would be done in a Write Attributes Request.

	The attribute list should be an array of attribute records, ending with
	an attribute ID of ZCL_ATTRIBUTE_END_OF_LIST.

	@param[out]		buffer			buffer to write values to
	@param[in]		bufsize			size of \p buffer
	@param[in,out]	p_attr_list		pointer to the attribute list to encode in
											\p buffer; points to next attribute to
											encode if buffer is filled

	@return	number of bytes written to buffer
*/
#ifdef __XBEE_PLATFORM_HCS08
	#pragma MESSAGE DISABLE C5909		// Assignment in condition is OK
#endif
zcl_client_debug
int zcl_create_attribute_records( void FAR *buffer,
	uint8_t bufsize, const zcl_attribute_base_t FAR **p_attr_list)
{
	zcl_attrib_t FAR *encoded;
	uint8_t FAR *buffer_insert;
	const zcl_attribute_base_t FAR *attr;
	int result;

	if (p_attr_list == NULL || buffer == NULL ||
		(attr = *p_attr_list) == NULL)
	{
		// can't encode any attributes; 0 bytes written to buffer
		return 0;
	}

	buffer_insert = buffer;

	while (attr->id != ZCL_ATTRIBUTE_END_OF_LIST && bufsize >= 3)
	{
		encoded = (zcl_attrib_t FAR *) buffer_insert;
		encoded->id_le = htole16( attr->id);
		encoded->type = attr->type;
		result = zcl_encode_attribute_value( encoded->value, bufsize - 3, attr);
		if (result < 0)		// not enough room to encode value
		{
			break;
		}
		result += 3;			// account for 3 bytes used by ID and type
		bufsize -= result;
		buffer_insert += result;

		attr = zcl_attribute_get_next( attr);	// get next attribute to encode
	}

	// update *p_attr_list to point to the next attribute to encode, or
	// the end of the attribute list
	*p_attr_list = attr;

	return (int) (buffer_insert - (uint8_t FAR *)buffer);
}
#ifdef __XBEE_PLATFORM_HCS08
	#pragma MESSAGE DEFAULT C5909		// restore C5909 (Assignment in condition)
#endif

/*** BeginHeader zcl_send_write_attributes */
/*** EndHeader */
/**
	@internal
	@brief
	Send one or more Write Attributes Requests to a node on the network.

	@note This function API and is likely to change in a future release
			to include a Manufacturer ID and flags for sending server-to-client
			and "undivided" write requests.

	@param[in,out]	envelope	envelope to use for sending request; \c payload
									and \c length cleared on function exit
	@param[in]		attr_list	attributes with values to send a Write
										Attributes Request for

	@retval	-EINVAL	NULL parameter passed, or couldn't find source endpoint
							based on envelope
	@retval	<0			error trying to send request (for large writes, one or
							more requests may have been successful)
	@retval	0			request(s) sent
*/
// Consider adding mfg_id and flags field for direction and undivided?
// Should this function send a single request, and somehow return the next
// attribute to write (as zcl_create_attribute_records does)?
zcl_client_debug
int zcl_send_write_attributes( wpan_envelope_t *envelope,
	const zcl_attribute_base_t FAR *attr_list)
{
	const wpan_endpoint_table_entry_t *source_endpoint;
	PACKED_STRUCT request {
		zcl_header_nomfg_t	header;
		uint8_t					payload[80];
	} request;
	int bytecount;
	int retval = 0;

	// wpan_endpoint_of_envelope returns NULL for a NULL envelope
	source_endpoint = wpan_endpoint_of_envelope( envelope);
	if (source_endpoint == NULL || attr_list == NULL)
	{
		return -EINVAL;
	}

	envelope->payload = &request;
	request.header.frame_control = ZCL_FRAME_CLIENT_TO_SERVER
											| ZCL_FRAME_TYPE_PROFILE
											| ZCL_FRAME_GENERAL
											| ZCL_FRAME_DISABLE_DEF_RESP;
	request.header.command = ZCL_CMD_WRITE_ATTRIB;

	// it may take multiple packets to encode the write attributes records
	while (attr_list->id != ZCL_ATTRIBUTE_END_OF_LIST && retval == 0)
	{
		bytecount = zcl_create_attribute_records( &request.payload,
			sizeof(request.payload), &attr_list);

		if (bytecount <= 0)
		{
			// some sort of error, or attribute is too big to fit in request
			retval = bytecount;
			break;
		}

		request.header.sequence = wpan_endpoint_next_trans( source_endpoint);
		envelope->length = offsetof( struct request, payload) + bytecount;
		#ifdef ZCL_CLIENT_VERBOSE
			printf( "%s: sending Write Attributes Request\n", __FUNCTION__);
			wpan_envelope_dump( envelope);
		#endif
		retval = wpan_envelope_send( envelope);
	}

	envelope->payload = NULL;
	envelope->length = 0;

	return retval;
}

/*** BeginHeader zcl_format_utctime */
/*** EndHeader */
/**
	@internal
	@brief Default function for zcl_format_utctime function pointer.
*/
const char *_zcl_format_utctime( char dest[40], zcl_utctime_t utctime)
{
	struct tm tm;

	if (dest != NULL)
	{
		if (utctime == ZCL_UTCTIME_INVALID)
		{
			strcpy( dest, "[Invalid Timestamp]");
		}
		else
		{
			xbee_gmtime( &tm, utctime);
			sprintf( dest, "%04u-%02u-%02u %02u:%02u:%02u UTC",
				tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
				tm.tm_hour, tm.tm_min, tm.tm_sec);
		}
	}

	return dest;
}
/**
	@brief Pointer to function that converts a zcl_utctime_t value to a
			human-readable timestamp.

	Defaults to a function that formats timestamps as "YYYY-MM-DD HH:MM:SS UTC"
	or "[Invalid Timestamp] if \p utctime is 0xFFFFFFFF.  Can be overriden by
	the user program to a function that uses another format.  Strings must be
	limited to 40 characters.

	@param[out]		dest		buffer to hold the formatted string
	@param[in]		utctime	time value to format

	@returns Value of parameter \p dest.
*/
zcl_format_utctime_fn zcl_format_utctime = _zcl_format_utctime;

/*** BeginHeader zcl_print_array_value, zcl_print_struct_value,
						zcl_print_attribute_value */
/*** EndHeader */
/**
	@brief Print Read Attributes Response value of an array attribute.

	Helper function for zcl_print_attribute_value(), takes a pointer to the
	"value" field of a Read Attributes Response for an array type, formats
	it and prints to stdout.

@code Example:
of 3 UNSIGNED_8BIT
	[0] = 0x54 = 84
	[1] = 0x42 = 66
	[2] = 0x43 = 67
@endcode

	@param[in]	value				pointer to ZCL-encoded array value
	@param[in]	value_length	number of bytes at value

	@retval	-EINVAL	\p value is NULL or \value_length is too small
	@retval	>=3		number of bytes from \p value consumed by function

	@sa zcl_print_attribute_value, zcl_print_struct_value
*/
int zcl_print_array_value( const void *value, int value_length)
{
	int i, result, offset;
	const uint8_t *b = value;
	uint16_t count;
	uint8_t type;

	// needs to be at least 3 bytes, type and count
	if (value == NULL || value_length < 3)
	{
		return -EINVAL;
	}

	type = b[0];
	count = b[1] + ((uint16_t)b[2] << 8);

	if (count == 0xFFFF && value_length >= 3)
	{
		printf( "undefined of %s\n", zcl_type_name( type));
		return 3;
	}

	printf( "of %u %s\n", count, zcl_type_name( type));
	for (i = 0, offset = 3; i < count && offset < value_length; ++i)
	{
		printf( "\t[%2u] = ", i);
		result = zcl_print_attribute_value( type, &b[offset],
			value_length - offset);
		if (result < 0)
		{
			printf( "[error %d printing value]\n", result);
			return result;
		}
		offset += result;
	}

	return offset;
}

/**
	@brief Print Read Attributes Response value of a structure attribute.

	Helper function for zcl_print_attribute_value(), takes a pointer to the
	"value" field of a Read Attributes Response for a structure type, formats
	it and prints to stdout.

@code Example:
with 3 elements
	.element0 (TIME_UTCTIME) = 2012-01-01 00:00:00
	.element1 (UNSIGNED_8BIT) = 0xAA = 170
	.element2 (SIGNED_8BIT) = 0xAA = -86
@endcode

	@param[in]	value				pointer to ZCL-encoded structure value
	@param[in]	value_length	number of bytes at value

	@retval	-EINVAL	\p value is NULL or \value_length is too small
	@retval	>=2		number of bytes from \p value consumed by function

	@sa zcl_print_attribute_value, zcl_print_array_value
*/
int zcl_print_struct_value( const void *value, int value_length)
{
	int i, result, offset;
	const uint8_t *b = value;
	uint16_t count;
	uint8_t type;

	// needs to be at least 2 bytes, count
	if (value == NULL || value_length < 2)
	{
		return -EINVAL;
	}

	// Try to display the elements.  If we can't, then fall back on existing code
	// which will perform a hex dump.
	count = b[0] + ((uint16_t)b[1] << 8);
	if (count == 0xFFFF)
	{
		puts( "undefined");
		return 2;
	}

	printf( "with %u elements\n", count);

	for (i = 0, offset = 2; i < count && offset < value_length; ++i)
	{
		type = b[offset++];
		printf( "\t.element%u (%s) = ", i, zcl_type_name( type));
		result = zcl_print_attribute_value( type, &b[offset],
			value_length - offset);
		if (result < 0)
		{
			printf( "[error %d printing value]\n", result);
			return result;
		}
		offset += result;
	}

	return offset;
}

/**
	Formats an attribute value and prints it to stdout.

	@param[in]	type				ZCL datatype of attribute's value
	@param[in]	value				pointer to the ZCL-encoded value
	@param[in]	value_length	number of bytes at value

	@retval	-EINVAL	\p value is NULL or \value_length is too small
	@retval	>=2		number of bytes from \p value consumed by function

	@sa zcl_print_array_value, zcl_print_struct_value
*/
int zcl_print_attribute_value( uint8_t type, const void *value,
	int value_length)
{
	bool_t hexdump = FALSE;
	const uint8_t *b = value;
	uint32_t temp = 0;
	addr64 addr_be;
	int typesize, datasize;
	int i;
	char buffer[80];

	if (value == NULL)
	{
		return -EINVAL;
	}

	if (type == ZCL_TYPE_ARRAY || type == ZCL_TYPE_SET || type == ZCL_TYPE_BAG)
	{
		datasize = zcl_print_array_value( value, value_length);
	}
	else if (type == ZCL_TYPE_STRUCT)
	{
		datasize = zcl_print_struct_value( value, value_length);
	}
	else
	{
		datasize = 0;
	}

	if (datasize > 0)
	{
		// zcl_print_{array|struct}_value successfully parsed <datasize> bytes
		// from value, so return that count.
		return datasize;
	}
	else if (datasize < 0)
	{
		hexdump = TRUE;		// problem parsing structured type; dump it raw
	}

	datasize = value_length;			// default to consuming entire value_length
	typesize = zcl_sizeof_type( type);
	switch (typesize)
	{
		case ZCL_SIZE_SHORT:
			// 1-octet size prefix, length is that size +1 for the size byte
			datasize = b[0] + 1;
			break;

		case ZCL_SIZE_LONG:
			// 2-octet size prefix, length is that size +2 for the size bytes
			datasize = b[1] * 256 + b[0] + 2;
			break;

		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 16:
			datasize = typesize;
			break;

		case ZCL_SIZE_INVALID:
		default:
			hexdump = TRUE;
			break;
	}
	if (value_length < datasize)		// type calls for more data than present
	{
		hexdump = TRUE;
		datasize = value_length;
	}

	if (type == ZCL_TYPE_STRING_CHAR)
	{
		for (i = 1; i < datasize && isprint( b[i]); ++i);
		if (i == datasize)
		{
			if (datasize > 32)
			{
				// when printing string length, subtract length byte from datasize
				printf( "%u-byte string\n\t", datasize - 1);
			}
			printf( "\"%.*s\"\n", datasize - 1, &b[1]);
			return datasize;			// chars in string plus length byte
		}
	}

	if (! hexdump) switch (type)
	{
		case ZCL_TYPE_NO_DATA:
		case ZCL_TYPE_UNKNOWN:
			puts( "n/a");
			break;

		case ZCL_TYPE_LOGICAL_BOOLEAN:
			if (*b == ZCL_BOOL_FALSE)
			{
				puts( "FALSE");
			}
			else if (*b == ZCL_BOOL_TRUE)
			{
				puts( "TRUE");
			}
			else
			{
				printf( "Invalid (0x%02X)\n", *b);
			}
			break;

		case ZCL_TYPE_IEEE_ADDR:
			memcpy_letobe( &addr_be, value, 8);
			printf( "%s\n", addr64_format( buffer, &addr_be));
			break;

		case ZCL_TYPE_TIME_UTCTIME:
		case ZCL_TYPE_GENERAL_32BIT:
		case ZCL_TYPE_BITMAP_32BIT:
		case ZCL_TYPE_UNSIGNED_32BIT:
		case ZCL_TYPE_SIGNED_32BIT:
			temp += (uint32_t)b[3] << 24;
			// fall through to lower bytes
		case ZCL_TYPE_GENERAL_24BIT:
		case ZCL_TYPE_BITMAP_24BIT:
		case ZCL_TYPE_UNSIGNED_24BIT:
		case ZCL_TYPE_SIGNED_24BIT:
			temp += (uint32_t)b[2] << 16;
			// fall through to lower bytes
		case ZCL_TYPE_GENERAL_16BIT:
		case ZCL_TYPE_BITMAP_16BIT:
		case ZCL_TYPE_UNSIGNED_16BIT:
		case ZCL_TYPE_SIGNED_16BIT:
		case ZCL_TYPE_ENUM_16BIT:
		case ZCL_TYPE_ID_CLUSTER:
		case ZCL_TYPE_ID_ATTRIB:
			temp += (uint16_t)b[1] << 8;
			// fall through to lower bytes
		case ZCL_TYPE_GENERAL_8BIT:
		case ZCL_TYPE_BITMAP_8BIT:
		case ZCL_TYPE_UNSIGNED_8BIT:
		case ZCL_TYPE_SIGNED_8BIT:
		case ZCL_TYPE_ENUM_8BIT:
			temp += b[0];

			if (type == ZCL_TYPE_TIME_UTCTIME)
			{
				printf( "%s\n", zcl_format_utctime( buffer, temp));
			}
			else if ((type >= ZCL_TYPE_BITMAP_8BIT && type <= ZCL_TYPE_BITMAP_32BIT)
						|| type == ZCL_TYPE_ID_CLUSTER || type == ZCL_TYPE_ID_ATTRIB)
			{
				// these types are only ever displayed as hexadecimal
				printf( "0x%0*" PRIX32 "\n",
					typesize << 1, temp);
			}
			else if (zcl_type_info[type] & ZCL_T_SIGNED)
			{
				printf( "0x%0*" PRIX32 " = %" PRId32 "\n",
					typesize << 1, temp, temp);
			}
			else
			{
				printf( "0x%0*" PRIX32 " = %" PRIu32 "\n",
					typesize << 1, temp, temp);
			}
			break;

		default:
			hexdump = TRUE;
	}

	if (hexdump)
	{
		puts( " Value:");
		hex_dump( value, datasize, HEX_DUMP_FLAG_TAB);
	}

	return datasize;
}


