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
	@addtogroup zdo
	@{
	@file zigbee_zdo.c
	ZigBee Device Objects (ZDO) and ZigBee Device Profile (ZDP).

	Note that ZigBee firmware needs to have ATAO set to 3 (or "not 1")
	in order to receive the Simple Descriptor, Active Endpoints and
	Match Descriptor requests.  We assume that the Ember stack handles
	responses to those requests for our children devies, and will only
	pass requests up for our device to respond to.

	@todo Create a cluster table and have the ZDO_ENDPOINT() macro use
			it instead of having zdo_handler use a switch on cluster_id
			to call the handlers for each command.
*/


/*** BeginHeader */
#include <stddef.h>
#include <stdio.h>

#include "xbee/platform.h"
#include "wpan/aps.h"
#include "xbee/byteorder.h"
#include "zigbee/zdo.h"

#ifndef __DC__
	#define zigbee_zdo_debug
#elif defined ZIGBEE_ZDO_DEBUG
	#define zigbee_zdo_debug	__debug
#else
	#define zigbee_zdo_debug	__nodebug
#endif

// maximum number of endpoints; arbitrarily selected
#define ZDO_MAX_ENDPOINTS	20
/*** EndHeader */

/*** BeginHeader _zdo_endpoint_of */
const wpan_endpoint_table_entry_t *_zdo_endpoint_of( wpan_dev_t *dev);
/*** EndHeader */
/**
	@internal
	@brief Return the endpoint table entry for the ZDO endpoint.

	@param[in]	dev	search the endpoint table of this device

	@retval	NULL	couldn't find the ZDO endpoint for \c dev
	@retval	!NULL	pointer to the ZDO endpoint of \c dev
*/
zigbee_zdo_debug
const wpan_endpoint_table_entry_t *_zdo_endpoint_of( wpan_dev_t *dev)
{
	return wpan_endpoint_match( dev, WPAN_ENDPOINT_ZDO, WPAN_PROFILE_ZDO);
}

/*** BeginHeader zdo_endpoint_state */
/*** EndHeader */
/**
	@brief
	Returns the ZDO endpoint's state if a device has a ZDO endpoint.

	@param[in]	dev		device to query

	@retval	NULL		\c dev does not have a ZDO endpoint
	@retval	!NULL		address of wpan_ep_state variable used for state tracking
*/
zigbee_zdo_debug
wpan_ep_state_t FAR *zdo_endpoint_state( wpan_dev_t *dev)
{
	const wpan_endpoint_table_entry_t *ep;

	ep = _zdo_endpoint_of( dev);

	return ep ? ep->ep_state : NULL;
}


/*** BeginHeader _zdo_envelope_create */
const addr64 *_zdo_envelope_create( wpan_envelope_t *envelope, wpan_dev_t *dev,
	const addr64 *address);
/*** EndHeader */
/**
	@internal
	@brief helper function for creating ZDO request envelopes

	@param[out]	envelope			envelope to populate
	@param[in]	dev				device that will send this envelope
	@param[in]	address			send request to the given target, or NULL
										for a self-addressed request

	@return	either \p address or a pointer to an all-zero IEEE address

	@note return value may change to something more useful in a future release
*/
zigbee_zdo_debug
const addr64 *_zdo_envelope_create( wpan_envelope_t *envelope, wpan_dev_t *dev,
	const addr64 *address)
{
	if (envelope == NULL)
	{
		return address ? address : WPAN_IEEE_ADDR_ALL_ZEROS;
	}

	if (address)	// send request to the given target device
	{
		wpan_envelope_create( envelope, dev, address, WPAN_NET_ADDR_UNDEFINED);
	}
	else				// self-addressed request
	{
		wpan_envelope_create( envelope, dev, &dev->address.ieee,
			dev->address.network);
		address = WPAN_IEEE_ADDR_ALL_ZEROS;
	}

	// wpan_envelope_create leaves profile and endpoints set to 0.  Only need
	// to change them if the ZDO endpoint/profile is non-zero (which it isn't).
	#if WPAN_PROFILE_ZDO != 0
		envelope->profile_id = WPAN_PROFILE_ZDO;
	#endif
	#if WPAN_ENDPOINT_ZDO != 0
		envelope->source_endpoint = envelope->dest_endpoint = WPAN_ENDPOINT_ZDO;
	#endif

	return address;
}


/*** BeginHeader zdo_send_response */
/*** EndHeader */
/**
	@brief
	Send a response to a ZDO request.

	Automatically builds the response
	envelope and sets its cluster ID (to the request's cluster ID with the
	high bit set) before sending.

	@param[in]	request		envelope of original request
	@param[in]	response		frame to send in response
	@param[in]	length		length of \a response

	@retval	0	sent response
	@retval	!0	error sending response
*/
zigbee_zdo_debug
int zdo_send_response( const wpan_envelope_t FAR *request,
	const void FAR *response, uint16_t length)
{
	int retval;
	wpan_envelope_t reply_envelope;

	// address envelope of reply, handles NULL request
	retval = wpan_envelope_reply( &reply_envelope, request);
	if (retval != 0)
	{
		return retval;
	}

	#ifdef ZIGBEE_ZDO_VERBOSE
		printf( "%s: sending %u-byte response\n", __FUNCTION__, length);
		hex_dump( response, length, HEX_DUMP_FLAG_TAB);
	#endif
	reply_envelope.payload = response;
	reply_envelope.length = length;
	reply_envelope.cluster_id |= ZDO_CLUST_RESPONSE_MASK;

	return wpan_envelope_send( &reply_envelope);
}

/*** BeginHeader zdo_match_desc_request */
/*** EndHeader */
/**
	@internal	Writes to *buffer, an 8-bit cluster count followed by the 16-bit
					cluster IDs in little-endian byte order.

	@param[in,out]	buffer			location to store cluster list, updated to
											point to first byte after bytes written
	@param[in]		cluster_read	list of 16-bit cluster IDs, terminated by
											WPAN_CLUSTER_END_OF_LIST
	@param[in]		max_count		maximum number of clusters to write to *buffer

	@retval	-ENOSPC		can't fit list of clusters into buffer
	@retval	>=0			number of clusters written to buffer
*/
int _match_desc_cluster_list( uint8_t **buffer,
	const uint16_t *clust_read, int max_count)
{
	uint_fast8_t	written = 0;
	uint16_t			*clust_write_le;

	clust_write_le = (uint16_t *) (*buffer + 1);
	if (clust_read != NULL)
	{
		while (*clust_read != WPAN_CLUSTER_END_OF_LIST)
		{
			if (++written > max_count)
			{
				return -ENOSPC;			// not enough room for cluster ID
			}
			*clust_write_le++ = htole16( *clust_read++);
		}
	}
	**buffer = (uint8_t) written;				// store count at start of list
	*buffer = (uint8_t *) clust_write_le;	// point past end of clusters written

	return written;								// number of clusters written
}

/**
	@brief
	Generate a Match_Desc (Match Descriptor) request (ZigBee spec 2.4.3.1.7)
	to send on the network.  Note that the first byte of \a buffer is NOT set,
	and that the caller should set it to the next sequence number/transaction ID.

	@param[out]	buffer				Buffer to hold generated request.
	@param[in]	buflen				Size of \a buffer used to hold generated
											request.
	@param[in]	addr_of_interest	See ZDO documentation for NWKAddrOfInterest.
	@param[in]	profile_id			Profile ID to match, must be an actual
											profile ID (cannot be WPAN_APS_PROFILE_ANY).
	@param[in]	inClust				List of input clusters, ending with
											#WPAN_CLUSTER_END_OF_LIST.  Can use
											\c NULL if there aren't any input clusters.
	@param[in]	outClust				List of output clusters, ending with
											#WPAN_CLUSTER_END_OF_LIST.  Can use
											\c NULL if there aren't any output clusters.

	@retval	-ENOSPC	\a buffer isn't large enough to hold request; need 7
							bytes plus (2 * the number of clusters)
	@retval	>0	number of bytes written to \a buffer
*/
zigbee_zdo_debug
int zdo_match_desc_request( void *buffer, int16_t buflen,
	uint16_t addr_of_interest, uint16_t profile_id, const uint16_t *inClust,
	const uint16_t *outClust)
{
	PACKED_STRUCT {
		uint8_t		transaction;
		uint16_t		network_addr_le;
		uint16_t		profile_id_le;
	} *request;
	uint8_t			*clusters;
	int				retval;
	int				max_count;

	if (buffer == NULL)
	{
		return -EINVAL;
	}

	// space for response, input count, output count and at least 1 cluster
	if (buflen < (sizeof *request + 2 + sizeof(uint16_t)))
	{
		return -ENOSPC;
	}

	// Can fit a maximum of this many clusters in the response (deduct space for
	// header, count of input clusters, and count of output clusters).
	max_count = (buflen - sizeof *request - 2) / sizeof(uint16_t);

	request = buffer;
	request->network_addr_le = htole16( addr_of_interest);
	request->profile_id_le = htole16( profile_id);

	clusters = ((uint8_t *)buffer) + sizeof(*request);
	retval = _match_desc_cluster_list( &clusters, inClust, max_count);
	if (retval >= 0)
	{
		max_count -= retval;
		retval = _match_desc_cluster_list( &clusters, outClust, max_count);
	}
	if (retval < 0)
	{
		return retval;
	}

	return clusters - (uint8_t *)buffer;
}

/*** BeginHeader _zdo_match_desc_respond */
int _zdo_match_desc_respond( const wpan_envelope_t FAR *envelope);
/*** EndHeader */
#ifdef __XBEE_PLATFORM_HCS08
	#pragma MESSAGE DISABLE C5909		// Assignment in condition is OK
#endif
/**
	@internal
	@brief
	Respond to a Match Descriptor (Match_Desc) request (ZigBee spec 2.4.3.1.7).

	This ZDO request is used to find endpoints using a particular Profile ID
	and a cluster from a list of input and output clusters.

	@param[in]	envelope		envelope of request

	@retval	0	successfully sent response, or none of the endpoints matched
					the request
	@retval	-ENODATA	don't know network address of \p envelope->dev
	@retval	!0	error parsing request or sending response
*/
zigbee_zdo_debug
int _zdo_match_desc_respond( const wpan_envelope_t FAR *envelope)
{
	const wpan_endpoint_table_entry_t			*ep;
	const zdo_match_desc_req_t				FAR *req;
	uint16_t profile_id;

	// used to walk in/out clusters in request
	union {
		const uint8_t FAR *count;
		const uint16_t FAR *cluster_le;
	} p;
	uint8_t mask;
	uint_fast8_t type;
	uint_fast8_t count;
	bool_t search;

	// used to build response
	PACKED_STRUCT {
		uint8_t									transaction;
		zdo_match_desc_rsp_header_t		header;
		uint8_t									endpoints[ZDO_MAX_ENDPOINTS];
	} rsp;
	uint8_t *match_list;

	if (envelope == NULL)
	{
		return -EINVAL;
	}

	// Make sure we know our 16-bit network address before responding.
	if (envelope->dev->address.network == WPAN_NET_ADDR_UNDEFINED)
	{
		#ifdef ZIGBEE_ZDO_VERBOSE
			printf( "%s: ignoring request; don't know my address\n",
				__FUNCTION__);
		#endif

		return -ENODATA;
	}

	#ifdef ZIGBEE_ZDO_VERBOSE
		printf( "%s: %u-byte request:\n", __FUNCTION__, envelope->length);
		hex_dump( envelope->payload, envelope->length, HEX_DUMP_FLAG_TAB);
	#endif

	req = envelope->payload;
	match_list = rsp.endpoints;
	profile_id = le16toh( req->profile_id_le);

	// Loop through endpoints for this device, looking for matching profiles.
	ep = NULL;
	while ( (ep = wpan_endpoint_get_next( envelope->dev, ep)) != NULL)
	{
		if (ep->endpoint != WPAN_ENDPOINT_ZDO && ep->profile_id == profile_id)
		{
			search = TRUE;
			p.count = &req->num_in_clusters;
			// Search the list of input clusters and then output clusters in the
			// request, attempting to match each one to each of the clusters on
			// this endpoint.
			// loop twice, first with FLAG_INPUT, then with FLAG_OUTPUT
			for (mask = WPAN_CLUST_FLAG_INPUT, type = 2; type && search;
				mask = WPAN_CLUST_FLAG_OUTPUT, --type)
			{
				count = *p.count++;
				while (search && count--)
				{
					if (wpan_cluster_match( le16toh( *p.cluster_le++), mask,
						ep->cluster_table))
					{
						#ifdef ZIGBEE_ZDO_VERBOSE
							printf( "%s: endpoint 0x%02x matched\n", __FUNCTION__,
								ep->endpoint);
						#endif
						*match_list++ = ep->endpoint;
						search = FALSE;
					}
				}
			}
		}
	}

	rsp.header.match_length = (uint8_t) (match_list - rsp.endpoints);
	if (rsp.header.match_length)
	{
		rsp.transaction = req->transaction;
		rsp.header.status = ZDO_STATUS_SUCCESS;
		rsp.header.network_addr_le = htole16( envelope->dev->address.network);
		// Unicast to the originator of the MATCH_DESC_REQ command.
		return zdo_send_response( envelope, &rsp, match_list - (uint8_t *)&rsp);
	}

	#ifdef ZIGBEE_ZDO_VERBOSE
		printf( "%s: ignoring MATCH DESC, no matching endpoints\n", __FUNCTION__);
	#endif
	return 0;
}
#ifdef __XBEE_PLATFORM_HCS08
	#pragma MESSAGE DEFAULT C5909		// restore C5909 (Assignment in condition)
#endif


/*** BeginHeader zdo_simple_desc_request */
/*** EndHeader */
// documented in zigbee/zdo.h
zigbee_zdo_debug
int zdo_simple_desc_request( wpan_envelope_t *envelope,
	uint16_t addr_of_interest, uint_fast8_t endpoint,
	wpan_response_fn callback, const void FAR *context)
{
	int retval;
	zdo_simple_desc_req_t zdo;

	if (envelope == NULL || endpoint == 0 || endpoint == 255)
	{
		return -EINVAL;
	}

	retval = wpan_conversation_register( zdo_endpoint_state( envelope->dev),
		callback, context, ZDO_CONVERSATION_TIMEOUT);
	if (retval < 0)
	{
		return retval;
	}

	zdo.transaction = (uint8_t) retval;
	zdo.network_addr_le = htole16( addr_of_interest);
	zdo.endpoint = endpoint;

	// wpan_envelope_create leaves profile and endpoints set to 0.  Only need
	// to change them if the ZDO endpoint/profile is non-zero (which it isn't).
	#if WPAN_PROFILE_ZDO != 0
		envelope->profile_id = WPAN_PROFILE_ZDO;
	#endif
	#if WPAN_ENDPOINT_ZDO != 0
		envelope->source_endpoint = envelope->dest_endpoint = WPAN_ENDPOINT_ZDO;
	#endif

	envelope->cluster_id = ZDO_SIMPLE_DESC_REQ;
	envelope->payload = &zdo;
	envelope->length = sizeof zdo;

	retval = wpan_envelope_send( envelope);

	envelope->payload = NULL;
	envelope->length = 0;

	return retval;
}

/*** BeginHeader _zdo_simple_desc_respond */
int _zdo_simple_desc_respond( const wpan_envelope_t FAR *envelope);
/*** EndHeader */
/**
	@internal	Writes to *buffer, an 8-bit cluster count followed by the 16-bit
					IDs for entries in a cluster table matching a given mask.

	@param[in,out]	buffer			location to store cluster list, updated to
											point to first byte after bytes written
	@param[in]		max_count		maximum number of clusters to write to *buffer
	@param[in]		mask				mask used to restrict entries
	@param[in]		entry				list of clusters, terminated by an entry with
											and ID of WPAN_CLUSTER_END_OF_LIST

	@retval	-ENOSPC		can't fit list of clusters into buffer
	@retval	>=0			number of clusters written to buffer
*/
int _simple_desc_cluster_list( uint8_t **buffer, int max_count,
	uint8_t mask, const wpan_cluster_table_entry_t *entry)
{
	int count = 0;
	uint16_t *cluster_le;

	// start writing clusters after <count> byte in buffer
	cluster_le = (uint16_t *)(*buffer + 1);
	if (entry != NULL)
	{
		for ( ; entry->cluster_id != WPAN_CLUSTER_END_OF_LIST; ++entry)
		{
			if (entry->flags & mask)
			{
				if (++count > max_count)
				{
					return -ENOSPC;			// not enough room to store the cluster
				}
				*cluster_le++ = htole16( entry->cluster_id);
			}
		}
	}
	**buffer = (uint8_t) count;			// count in first byte of buffer
	*buffer = (uint8_t *) cluster_le;	// point past entries written to buffer

	return count;								// entries written to buffer
}

/**
	@internal
	@brief
	Respond to a Simple Descriptor (Simple_Desc) request
	(ZigBee spec 2.4.3.1.5).

	This ZDO request is used to get information about a single endpoint.

	@param[in]	envelope		envelope of request

	@retval	0	successfully sent response
	@retval	-ENODATA	don't know network address of \p envelope->dev
	@retval	!0	error parsing request or sending response
*/
zigbee_zdo_debug
int _zdo_simple_desc_respond( const wpan_envelope_t FAR *envelope)
{
	const zdo_simple_desc_req_t		FAR	*req;
	const wpan_endpoint_table_entry_t		*ep = NULL;
	uint8_t *buffer;
	int retval;

	PACKED_STRUCT _response {
		uint8_t									transaction;
		zdo_simple_desc_resp_header_t		header;
		zdo_simple_desc_header_t			descriptor;
		uint8_t									clusters[80];
	} rsp;
	/// number of clusters that fit in cluster list, minus 2 count bytes
	#define MAX_CLUSTERS ((sizeof rsp.clusters - 2) / sizeof(uint16_t))

	if (envelope == NULL)
	{
		return -EINVAL;
	}

	// Make sure we know our 16-bit network address before responding.
	if (envelope->dev->address.network == WPAN_NET_ADDR_UNDEFINED)
	{
		#ifdef ZIGBEE_ZDO_VERBOSE
			printf( "%s: ignoring request; don't know my address\n",
				__FUNCTION__);
		#endif

		return -ENODATA;
	}

	req = envelope->payload;
	rsp.descriptor.endpoint = req->endpoint;

	#ifdef ZIGBEE_ZDO_VERBOSE
		printf( "%s: request for endpoint 0x%02X:\n", __FUNCTION__,
				rsp.descriptor.endpoint);
	#endif

	rsp.transaction = req->transaction;
	rsp.header.network_addr_le = htole16( envelope->dev->address.network);
	rsp.header.length = 0;
	rsp.header.status = ZDO_STATUS_SUCCESS;

	// 053474r18 2.4.4.1.5.1: "If the endpoint field ... does not fall within
	// the correct range specified in Table 2.49, the remote device shall set
	// the Status field to INVALID_EP, set the Length field to 0 and not
	// include the SimpleDescriptor field."
	if (rsp.descriptor.endpoint == 0 || rsp.descriptor.endpoint == 255)
	{
		rsp.header.status = ZDO_STATUS_INVALID_EP;

		#ifdef ZIGBEE_ZDO_VERBOSE
			printf( "%s: responding with %s for ep 0x%02x\n",
				__FUNCTION__, "INVALID_EP", rsp.descriptor.endpoint);
		#endif
	}
	else		// endpoint is valid, but is it active on this device?
	{
		ep = wpan_endpoint_match( envelope->dev, rsp.descriptor.endpoint,
			WPAN_APS_PROFILE_ANY);

		// 053474r18 2.4.4.1.5.1: "If the endpoint field does not correspond to
		// an active endpoint, the remote device shall set the Status field to
		// NOT_ACTIVE, set the Length field to 0, and not include the
		// SimpleDescriptor field."
		if (ep == NULL)
		{
			rsp.header.status = ZDO_STATUS_NOT_ACTIVE;

			#ifdef ZIGBEE_ZDO_VERBOSE
				printf( "%s: responding with %s for ep 0x%02x\n",
					__FUNCTION__, "NOT_ACTIVE", rsp.descriptor.endpoint);
			#endif
		}
	}

	if (ep != NULL)
	{
		rsp.descriptor.profile_id_le = htole16( ep->profile_id);
		rsp.descriptor.device_id_le = htole16( ep->device_id);
		rsp.descriptor.device_version = ep->device_version;

		buffer = &rsp.clusters[0];
		retval = _simple_desc_cluster_list( &buffer, MAX_CLUSTERS,
			WPAN_CLUST_FLAG_INPUT, ep->cluster_table);
		if (retval >= 0)
		{
			retval = _simple_desc_cluster_list( &buffer, MAX_CLUSTERS - retval,
				WPAN_CLUST_FLAG_OUTPUT, ep->cluster_table);
		}
		if (retval < 0)
		{
			rsp.header.status = ZDO_STATUS_INSUFFICIENT_SPACE;
		}
		else
		{
			rsp.header.length = (uint8_t) (buffer - (uint8_t *)&rsp.descriptor);

			// Unicast to the originator of the SIMPLE_DESC_REQ command.
			return zdo_send_response( envelope, &rsp, buffer - (uint8_t *)&rsp);
		}
	}

	// Unicast error to the originator of the SIMPLE_DESC_REQ command.
	// Send everything up to the start of the SimpleDescriptor.
	return zdo_send_response( envelope, &rsp,
		offsetof( struct _response, descriptor));

	#undef MAX_CLUSTERS
}


/*** BeginHeader _zdo_active_ep_respond */
int _zdo_active_ep_respond( const wpan_envelope_t FAR *envelope);
/*** EndHeader */
#ifdef __XBEE_PLATFORM_HCS08
	#pragma MESSAGE DISABLE C5909		// Assignment in condition is OK
#endif
/**
	@internal
	@brief
	Respond to an Active Endpoint (Active_EP) request (ZigBee spec 2.4.3.1.6).

	This ZDO request is used to get a list of active endoints on a device.

	@param[in]	envelope		envelope of request

	@retval	0	successfully sent response
	@retval	-ENODATA	don't know network address of \p envelope->dev
	@retval	!0	error parsing request or sending response
*/
zigbee_zdo_debug
int _zdo_active_ep_respond( const wpan_envelope_t FAR *envelope)
{
	const wpan_endpoint_table_entry_t	*ep;
	PACKED_STRUCT {
		uint8_t									transaction;
		zdo_active_ep_rsp_header_t			header;
		uint8_t									endpoints[ZDO_MAX_ENDPOINTS];
	} rsp;
	uint8_t *active_list;

	if (envelope == NULL)
	{
		return -EINVAL;
	}

	// Make sure we know our 16-bit network address before responding.
	if (envelope->dev->address.network == WPAN_NET_ADDR_UNDEFINED)
	{
		#ifdef ZIGBEE_ZDO_VERBOSE
			printf( "%s: ignoring request; don't know my address\n",
				__FUNCTION__);
		#endif

		return -ENODATA;
	}

	#ifdef ZIGBEE_ZDO_VERBOSE
		printf( "%s: %u-byte request:\n", __FUNCTION__, envelope->length);
		hex_dump( envelope->payload, envelope->length, HEX_DUMP_FLAG_TAB);
	#endif

	active_list = rsp.endpoints;
	ep = NULL;
	while ( (ep = wpan_endpoint_get_next( envelope->dev, ep)) != NULL)
	{
		if (ep->endpoint != WPAN_ENDPOINT_ZDO)
		{
			*active_list++ = ep->endpoint;
		}
	}

	// DEVNOTE: Not looking at network_addr_le field of request -- should we?

	rsp.transaction =
		((const zdo_active_ep_req_t FAR *)envelope->payload)->transaction;
	rsp.header.status = ZDO_STATUS_SUCCESS;
	rsp.header.network_addr_le = htole16( envelope->dev->address.network);
	rsp.header.ep_count = (uint8_t) (active_list - rsp.endpoints);

	// Unicast to the originator of the ACTIVE_EP_REQ command.
	return zdo_send_response( envelope, &rsp, active_list - (uint8_t *)&rsp);
}
#ifdef __XBEE_PLATFORM_HCS08
	#pragma MESSAGE DEFAULT C5909		// restore C5909 (Assignment in condition)
#endif

/*** BeginHeader zdo_handler */
/*** EndHeader */
/**
	@brief
	Process ZDO frames (received on endpoint 0 with Profile ID 0).

	@param[in]	envelope		envelope of received ZDO frame, contains address,
									endpoint, profile, and cluster info
	@param[in]	ep_state		pointer to endpoint's state structure
									(used for tracking transactions)

	@retval	0	successfully processed
	@retval	!0	error trying to process frame
*/
zigbee_zdo_debug
int zdo_handler( const wpan_envelope_t FAR *envelope,
																wpan_ep_state_t FAR *ep_state)
{
	uint8_t	response[2];
	uint8_t	transaction_id;
	int retval;

	if (envelope == NULL)
	{
		return -EINVAL;
	}

	// first byte of payload is transaction ID
	transaction_id = *(const uint8_t FAR *)envelope->payload;
	#ifdef ZIGBEE_ZDO_VERBOSE
		printf( "%s: received ZDO clust=0x%04x, trans=0x%02x\n",
			__FUNCTION__, envelope->cluster_id, transaction_id);
		hex_dump( envelope->payload, envelope->length, HEX_DUMP_FLAG_TAB);
	#endif

	switch (envelope->cluster_id)
	{
		case ZDO_SIMPLE_DESC_REQ:
			return _zdo_simple_desc_respond( envelope);

		case ZDO_ACTIVE_EP_REQ:
			return _zdo_active_ep_respond( envelope);

		case ZDO_MATCH_DESC_REQ:
			return _zdo_match_desc_respond( envelope);

		case ZDO_DEVICE_ANNCE:
			{
			#if defined ZIGBEE_ZDO_VERBOSE
				const zdo_device_annce_t FAR *annce = envelope->payload;

				addr64 addr_be;
				char buffer[ADDR64_STRING_LENGTH];

				memcpy_letobe( &addr_be, &annce->ieee_address_le, 8);
				printf( "%s: Device Announce %" PRIsFAR " (0x%04x) cap 0x%02x\n",
					__FUNCTION__, addr64_format( buffer, &addr_be),
					le16toh( annce->network_addr_le), annce->capability);
			#endif

				// 053474r19 2.4.4.1, "The server shall not supply a response to
				// the Device_annce."
				return 0;
			}
	}

	if (ZDO_CLUST_IS_RESPONSE( envelope->cluster_id))
	{
		retval = wpan_conversation_response( ep_state, transaction_id, envelope);

		if (retval != -EINVAL)
		{
			#ifdef ZIGBEE_ZDO_VERBOSE
				printf( "%s: matched to conversation and processed (ret=%d)\n",
					__FUNCTION__, retval);
			#endif
			return retval;
		}
		#ifdef ZIGBEE_ZDO_VERBOSE
			printf( "%s: no matching conversation for trans 0x%02x\n",
				__FUNCTION__, transaction_id);
		#endif
	}

	// If this is an unsupported RESPONSE cluster (high bit set) or a broadcast
	// request, ignore it.
	// 053474r19 2.4.4, "For all broadcast addressed requests (of any boradcast
	// address type) to the server, if the command is not supported, the server
	// shall drop the packet."
	if (	ZDO_CLUST_IS_RESPONSE(envelope->cluster_id)
		|| (envelope->options & WPAN_ENVELOPE_BROADCAST_ADDR) )
	{
		#ifdef ZIGBEE_ZDO_VERBOSE
			printf( "%s: ignoring  %scast to unsupported cluster 0x%" PRIx16 "\n",
				__FUNCTION__, (envelope->options & WPAN_ENVELOPE_BROADCAST_ADDR)
												? "broad" : "uni", envelope->cluster_id);
		#endif
		return 0;
	}

	#ifdef ZIGBEE_ZDO_VERBOSE
		printf( "%s: sending NOT_SUPPORTED for cluster ID %" PRIx16 "\n",
			__FUNCTION__, envelope->cluster_id);
	#endif

	response[0] = transaction_id;
	response[1] = ZDO_STATUS_NOT_SUPPORTED;

	#ifdef ZIGBEE_ZDO_VERBOSE
		printf( "%s: UNSUPPORTED CLUSTER 0x%04x (trans 0x%02x)\n",
			__FUNCTION__, envelope->cluster_id, transaction_id);
		hex_dump( envelope->payload, envelope->length, HEX_DUMP_FLAG_TAB);
		printf( "%s: response: %02x %02x\n", __FUNCTION__, response[0],
			response[1]);
	#endif

	return zdo_send_response( envelope, &response, 2);
}

/*** BeginHeader zdo_send_bind_req */
/*** EndHeader */
// documented in zigbee/zdo.h
zigbee_zdo_debug
int zdo_send_bind_req( wpan_envelope_t *envelope, uint16_t type,
	wpan_response_fn callback, void FAR *context)
{
	wpan_envelope_t				bind_env;
	PACKED_STRUCT {
		uint8_t						transaction;
		zdo_bind_address_req_t	req;
	} zdo;
	int								trans;

	if (envelope == NULL || (type != ZDO_BIND_REQ && type != ZDO_UNBIND_REQ))
	{
		return -EINVAL;
	}

	// build envelope for ZDO Bind Request
	_zdo_envelope_create( &bind_env, envelope->dev, &envelope->ieee_address);
	bind_env.network_address = envelope->network_address;
	bind_env.cluster_id = type;		// Bind_req or Unbind_req cluster ID
	bind_env.payload = &zdo;
	bind_env.length = sizeof zdo;

	// associate the callback with the transaction ID for this request
	trans = wpan_conversation_register( zdo_endpoint_state( bind_env.dev),
		callback, context, ZDO_CONVERSATION_TIMEOUT);
	if (trans < 0)
	{
		#ifdef ZIGBEE_ZDO_VERBOSE
			printf( "%s: error %d calling %s\n",
				__FUNCTION__, trans, "wpan_conversation_register");
		#endif
		return trans;
	}

	// build the ZDO Bind Request packet
	zdo.transaction = (uint8_t) trans;
	// Envelope IEEE Address is always big-endian (is NOT host byte order).
	memcpy_betole( &zdo.req.header.src_address_le, &envelope->ieee_address, 8);
	zdo.req.header.src_endpoint = envelope->dest_endpoint;
	zdo.req.header.cluster_id_le = htole16( envelope->cluster_id);
	zdo.req.header.dst_addr_mode = ZDO_BIND_DST_MODE_ADDR;
	memcpy_betole( &zdo.req.dst_address_le, &envelope->dev->address.ieee, 8);
	zdo.req.dst_endpoint = envelope->source_endpoint;

	// send the request
	return wpan_envelope_send( &bind_env);
}


/*** BeginHeader zdo_mgmt_leave_req */
/*** EndHeader */
// documented in zigbee/zdo.h
zigbee_zdo_debug
int zdo_mgmt_leave_req( wpan_dev_t *dev, const addr64 *address, uint16_t flags)
{
	wpan_envelope_t			envelope;
	PACKED_STRUCT {
		uint8_t						transaction;
 		zdo_mgmt_leave_req_t		request;
	} zdo;

	if (dev == NULL)
	{
		return -EINVAL;
	}

	zdo.request.device_address = *_zdo_envelope_create( &envelope, dev,
																			address);

	envelope.cluster_id = ZDO_MGMT_LEAVE_REQ;
	envelope.payload = &zdo;
	envelope.length = sizeof zdo;
	if (flags & ZDO_MGMT_LEAVE_REQ_ENCRYPTED)
	{
		envelope.options |= WPAN_CLUST_FLAG_ENCRYPT;
	}

	zdo.request.flags = (uint8_t) flags;
	zdo.transaction = wpan_endpoint_next_trans( _zdo_endpoint_of( dev));

	return wpan_envelope_send( &envelope);
}

/*** BeginHeader zdo_send_descriptor_req */
/*** EndHeader */
zigbee_zdo_debug
int zdo_send_descriptor_req( wpan_envelope_t *envelope, uint16_t cluster,
	uint16_t addr_of_interest, wpan_response_fn callback,
	const void FAR *context)
{
	int retval;
	PACKED_STRUCT {
		uint8_t					transaction;
 		uint16_t					network_addr_le;
	} zdo;

	if (envelope == NULL)
	{
		return -EINVAL;
	}

	retval = wpan_conversation_register( zdo_endpoint_state( envelope->dev),
		callback, context, ZDO_CONVERSATION_TIMEOUT);
	if (retval < 0)
	{
		return retval;
	}

	zdo.transaction = (uint8_t) retval;
	zdo.network_addr_le = htole16( addr_of_interest);

	// wpan_envelope_create leaves profile and endpoints set to 0.  Only need
	// to change them if the ZDO endpoint/profile is non-zero (which it isn't).
	#if WPAN_PROFILE_ZDO != 0
		envelope->profile_id = WPAN_PROFILE_ZDO;
	#endif
	#if WPAN_ENDPOINT_ZDO != 0
		envelope->source_endpoint = envelope->dest_endpoint = WPAN_ENDPOINT_ZDO;
	#endif

	envelope->cluster_id = cluster;
	envelope->payload = &zdo;
	envelope->length = sizeof zdo;

	retval = wpan_envelope_send( envelope);

	envelope->payload = NULL;
	envelope->length = 0;

	return retval;
}

/*** BeginHeader zdo_send_nwk_addr_req */
/*** EndHeader */
/**	@internal
	@brief Callback function to process NWK_addr response frames.

	The \a context field of the conversation is the location to store the
	16-bit address returned in the response.

	@see zdo_send_nwk_addr_req()
*/
zigbee_zdo_debug
int _zdo_process_nwk_addr_resp( wpan_conversation_t FAR *conversation,
	const wpan_envelope_t FAR *envelope)
{
	uint16_t FAR *net_addr;
	const PACKED_STRUCT {
		uint8_t							transaction;
		zdo_nwk_addr_rsp_header_t	header;
	} FAR *response;

	// conversation is never NULL in a wpan_response_fn

	net_addr = conversation->context;
	if (envelope == NULL)
	{
		#ifdef ZIGBEE_ZDO_VERBOSE
			printf( "%s: timeout waiting for response to transaction 0x%02x\n",
				__FUNCTION__, conversation->transaction_id);
		#endif
		*net_addr = ZDO_NET_ADDR_TIMEOUT;
	}
	else
	{
		response = envelope->payload;
		if (response->header.status == ZDO_STATUS_SUCCESS)
		{
			*net_addr = le16toh( response->header.net_remote_le);
			#ifdef ZIGBEE_ZDO_VERBOSE
				printf( "%s: network addr for transaction 0x%02x is 0x%04x\n",
					__FUNCTION__, response->transaction, *net_addr);
			#endif
		}
		else
		{
			#ifdef ZIGBEE_ZDO_VERBOSE
				printf( "%s: error 0x%02x on transaction 0x%02x\n", __FUNCTION__,
					response->header.status, response->transaction);
			#endif
			*net_addr = ZDO_NET_ADDR_ERROR;
		}
	}

	return WPAN_CONVERSATION_END;
}

// documented in zigbee/zdo.h
zigbee_zdo_debug
int zdo_send_nwk_addr_req( wpan_dev_t *dev, const addr64 FAR *ieee_be,
	uint16_t FAR *net)
{
	zdo_nwk_addr_req_t req;
	wpan_envelope_t	envelope;
	int transaction;
	#ifdef ZIGBEE_ZDO_VERBOSE
		char addr64_buf[ADDR64_STRING_LENGTH];
	#endif

	if (dev == NULL || ieee_be == NULL || net == NULL)
	{
		return -EINVAL;
	}

	_zdo_envelope_create( &envelope, dev, ieee_be);
	envelope.cluster_id = ZDO_NWK_ADDR_REQ;
	envelope.payload = &req;
	envelope.length = sizeof req;

	transaction = wpan_conversation_register( zdo_endpoint_state( dev),
		_zdo_process_nwk_addr_resp, net, ZDO_CONVERSATION_TIMEOUT);
	if (transaction < 0)
	{
		return transaction;
	}

	req.transaction = (uint8_t) transaction;
	memcpy_betole( &req.ieee_address_le, ieee_be, 8);
	req.request_type = ZDO_REQUEST_TYPE_SINGLE;
	req.start_index = 0;

	// Caller checks that address for response to change to device's 16-bit network
	// address, ZDO_NET_ADDR_TIMEOUT on timeout or ZDO_NET_ADDR_ERROR on error.
	*net = ZDO_NET_ADDR_PENDING;

	#ifdef ZIGBEE_ZDO_VERBOSE
		printf( "%s: NWK_addr for %" PRIsFAR " (trans 0x%02x)\n", __FUNCTION__,
			addr64_format( addr64_buf, ieee_be), req.transaction);
	#endif

	return wpan_envelope_send( &envelope);
}

/*** BeginHeader zdo_send_ieee_addr_req */
/*** EndHeader */
/**	@internal
	@brief Callback function to process IEEE_addr response frames.

	The \a context field of the conversation is the location to store the
	64-bit address returned in the response.

	@sa zdo_send_ieee_addr_req()
*/
zigbee_zdo_debug
int _zdo_process_ieee_addr_resp( wpan_conversation_t FAR *conversation,
	const wpan_envelope_t FAR *envelope)
{
	addr64 FAR *ieee_be;
	const PACKED_STRUCT {
		uint8_t							transaction;
		zdo_ieee_addr_rsp_header_t	header;
	} FAR *response;
	#ifdef ZIGBEE_ZDO_VERBOSE
		char addr64_buf[ADDR64_STRING_LENGTH];
	#endif

	// conversation is never NULL in a wpan_response_fn

	ieee_be = conversation->context;
	if (envelope == NULL)
	{
		#ifdef ZIGBEE_ZDO_VERBOSE
			printf( "%s: timeout waiting for response to transaction 0x%02x\n",
				__FUNCTION__, conversation->transaction_id);
		#endif
		*ieee_be = *ZDO_IEEE_ADDR_TIMEOUT;
	}
	else
	{
		response = envelope->payload;
		if (response->header.status == ZDO_STATUS_SUCCESS)
		{
			memcpy_letobe( ieee_be, &response->header.ieee_remote_le,
																				sizeof *ieee_be);
			#ifdef ZIGBEE_ZDO_VERBOSE
				printf( "%s: IEEE_addr for %" PRIsFAR " (trans 0x%02x)\n",
					__FUNCTION__, addr64_format( addr64_buf, ieee_be),
					response->transaction);
			#endif
		}
		else
		{
			#ifdef ZIGBEE_ZDO_VERBOSE
				printf( "%s: error 0x%02x on transaction 0x%02x\n", __FUNCTION__,
					response->header.status, response->transaction);
			#endif
			memset( ieee_be, 0, sizeof *ieee_be);
		}
	}

	return WPAN_CONVERSATION_END;
}

// documented in zigbee/zdo.h
zigbee_zdo_debug
int zdo_send_ieee_addr_req( wpan_dev_t *dev, uint16_t net_addr,
	addr64 FAR *ieee_be)
{
	zdo_ieee_addr_req_t req;
	wpan_envelope_t	envelope;
	int transaction;

	if (dev == NULL || ieee_be == NULL)
	{
		return -EINVAL;
	}

	wpan_envelope_create( &envelope, dev, WPAN_IEEE_ADDR_UNDEFINED, net_addr);
	envelope.cluster_id = ZDO_IEEE_ADDR_REQ;
	envelope.payload = &req;
	envelope.length = sizeof req;

	transaction = wpan_conversation_register( zdo_endpoint_state( dev),
		_zdo_process_ieee_addr_resp, ieee_be, ZDO_CONVERSATION_TIMEOUT);
	if (transaction < 0)
	{
		return transaction;
	}

	req.transaction = (uint8_t) transaction;
	req.network_addr_le = htole16( net_addr);
	req.request_type = ZDO_REQUEST_TYPE_SINGLE;
	req.start_index = 0;

	// Caller checks that address for response to change to device's 16-bit network
	// address, or WPAN_NET_ADDR_BCAST_ALL_NODES on timeout
	*ieee_be = *ZDO_IEEE_ADDR_PENDING;

	#ifdef ZIGBEE_ZDO_VERBOSE
		printf( "%s: IEEE_addr for 0x%04X (trans 0x%02x)\n", __FUNCTION__,
			net_addr, req.transaction);
	#endif

	return wpan_envelope_send( &envelope);
}
