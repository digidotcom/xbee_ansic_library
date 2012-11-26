/*
 * Copyright (c) 2008-2012 Digi International Inc.,
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
	@addtogroup wpan_aps
	@{
	@file wpan_aps.c
	Base WPAN/ZigBee layer, responsible for endpoints and clusters.
*/

/*** BeginHeader */
#include <errno.h>
#include <stddef.h>
#include <stdio.h>

#include "xbee/platform.h"				// may set WPAN_APS_VERBOSE macro
#include "wpan/types.h"
#include "wpan/aps.h"
#include "zigbee/zdo.h"
#include "zigbee/zcl.h"

#ifndef __DC__
	#define wpan_aps_debug
#elif defined WPAN_APS_DEBUG
	#define wpan_aps_debug		__debug
#else
	#define wpan_aps_debug		__nodebug
#endif
/*** EndHeader */

/*** BeginHeader wpan_cluster_match */
/*** EndHeader */
/** @brief
	Search a cluster table for a matching cluster ID.

	@param[in]	match	ID to match
	@param[in]	mask	Flags to match against the \c flags member of
							the wpan_cluster_table_entry_t structure.
							If any flags match, the entry is returned.  Typically
							one of
							- #WPAN_CLUST_FLAG_INPUT (or #WPAN_CLUST_FLAG_SERVER)
							- #WPAN_CLUST_FLAG_OUTPUT (or #WPAN_CLUST_FLAG_CLIENT)
	@param[in]	entry	pointer to list of entries to search


	@retval	NULL	\a entry is invalid or search reached
						#WPAN_CLUSTER_END_OF_LIST without finding a match.
	@retval	!NULL	Pointer to matching entry from table.

	@see wpan_endpoint_get_next(), wpan_endpoint_match()
*/
wpan_aps_debug
const wpan_cluster_table_entry_t *wpan_cluster_match( uint16_t match,
	uint8_t mask, const wpan_cluster_table_entry_t *entry)
{
	if (! entry)
	{
		return NULL;
	}

	while ( entry->cluster_id != WPAN_CLUSTER_END_OF_LIST)
	{
		if (entry->cluster_id == match && (mask & entry->flags))
		{
			// found a match
			return entry;
		}
		++entry;
	}
	return NULL;
}


/*** BeginHeader wpan_endpoint_get_next */
/*** EndHeader */
/** @brief
	Function used to walk a device's endpoint table.

	For most devices, this
	will just walk the entries in dev->endpoint_table.  For custom applications
	a function may dynamically return entries.

	Use of this function is the only way to walk the endpoint table.

	@param[in]	dev	device with endpoint table to walk
	@param[in]	ep		NULL to return first entry in table, or a pointer
							previously returned by this function to get the
							next entry

	@retval	NULL	\a dev is invalid or reached end of table
	@retval	!NULL	next entry from table

	@see wpan_endpoint_match(), wpan_cluster_match()
*/
wpan_aps_debug
const wpan_endpoint_table_entry_t *wpan_endpoint_get_next( wpan_dev_t *dev,
	const wpan_endpoint_table_entry_t *ep)
{
	if (dev == NULL)
	{
		return NULL;
	}

	// If this endpoint has a custom function for walking the endpoint table,
	// make use of it.
	if (dev->endpoint_get_next)
	{
		return dev->endpoint_get_next( dev, ep);
	}

	if (ep == NULL)					// return first entry in table
	{
		return dev->endpoint_table;
	}

	++ep;									// advance to next entry in table
	if (ep->endpoint == WPAN_ENDPOINT_END_OF_LIST)
	{
		return NULL;					// reached end of table
	}

	return ep;
}

/*** BeginHeader wpan_endpoint_match */
/*** EndHeader */
#ifdef __XBEE_PLATFORM_HCS08
	#pragma MESSAGE DISABLE C5909		// Assignment in condition is OK
#endif
/**
	@brief
	Walk a device's endpoint table looking for a matching endpoint ID
	and profile ID.

	Used by the endpoint dispatcher and ZDO layer to describe available
	endpoints on this device.

	@param[in]	dev			Device with endpoint table to search.
	@param[in]	endpoint		Endpoint number to search for.
	@param[in]	profile_id	Profile to match or WPAN_APS_PROFILE_ANY to
									search on endpoint number only.

	@retval	NULL	\a dev is invalid or reached end of table without finding
						a match
	@retval	!NULL	next entry from table

	@see wpan_endpoint_of_cluster(), wpan_endpoint_get_next(),
			wpan_cluster_match(), wpan_endpoint_of_envelope()
*/
wpan_aps_debug
const wpan_endpoint_table_entry_t *wpan_endpoint_match( wpan_dev_t *dev,
	uint8_t endpoint, uint16_t profile_id)
{
	const wpan_endpoint_table_entry_t *ep;
	bool_t matchany;

	// wpan_endpoint_get_next tests for NULL dev

	matchany = (profile_id == WPAN_APS_PROFILE_ANY);
	ep = NULL;
	while ( (ep = wpan_endpoint_get_next( dev, ep)) != NULL)
	{
		if (endpoint == ep->endpoint &&
			(matchany || profile_id == ep->profile_id))
		{
			return ep;
		}
	}
	return NULL;
}
#ifdef __XBEE_PLATFORM_HCS08
	#pragma MESSAGE DEFAULT C5909		// restore C5909 (Assignment in condition)
#endif


/*** BeginHeader wpan_endpoint_of_envelope */
/*** EndHeader */
/**
	@brief
	Look up the endpoint table entry for the source endpoint of an envelope.

	@param[in]	env			Envelope for lookup.  Uses \a env->source_endpoint
					and \a env->profile_id to find the endpoint table entry for
					\a env->dev.

	@retval	NULL	invalid parameter, or reached end of table without finding
						a match
	@retval	!NULL	entry from table

	@see wpan_endpoint_of_cluster(), wpan_endpoint_get_next(),
			wpan_cluster_match(), wpan_endpoint_match()
*/
wpan_aps_debug
const wpan_endpoint_table_entry_t
*wpan_endpoint_of_envelope( const wpan_envelope_t *env)
{
	if (env == NULL)
	{
		return NULL;
	}
	return wpan_endpoint_match( env->dev, env->source_endpoint, env->profile_id);
}


/*** BeginHeader wpan_endpoint_of_cluster */
/*** EndHeader */
#ifdef __XBEE_PLATFORM_HCS08
	#pragma MESSAGE DISABLE C5909		// Assignment in condition is OK
#endif
/** @brief
	Walk a device's endpoint table looking for a matching profile ID
	and cluster ID.

	Used to find the correct endpoint to use for sending ZCL client requests.

	@param[in]	dev			Device with endpoint table to search.
	@param[in]	profile_id	Profile to match or WPAN_APS_PROFILE_ANY to
									search on cluster ID only.
	@param[in]	cluster_id	Cluster ID to search for.
	@param[in]	mask	Flags to match against the \c flags member of
							the wpan_cluster_table_entry_t structure.
							If any flags match, the entry is returned.  Typically
							one of
							- #WPAN_CLUST_FLAG_INPUT (or #WPAN_CLUST_FLAG_SERVER)
							- #WPAN_CLUST_FLAG_OUTPUT (or #WPAN_CLUST_FLAG_CLIENT)

	@retval	NULL	\a dev is invalid or reached end of table without finding
						a match
	@retval	!NULL	matching entry from table

	@see wpan_endpoint_match(), wpan_endpoint_get_next(), wpan_cluster_match()
*/
wpan_aps_debug
const wpan_endpoint_table_entry_t *wpan_endpoint_of_cluster( wpan_dev_t *dev,
	uint16_t profile_id, uint16_t cluster_id, uint8_t mask)
{
	const wpan_endpoint_table_entry_t *ep;
	bool_t matchany;

	// wpan_endpoint_get_next tests for NULL dev

	matchany = (profile_id == WPAN_APS_PROFILE_ANY);
	ep = NULL;
	while ( (ep = wpan_endpoint_get_next( dev, ep)) != NULL)
	{
		if ((matchany || profile_id == ep->profile_id)
			&& wpan_cluster_match( cluster_id, mask, ep->cluster_table))
		{
			#ifdef WPAN_APS_VERBOSE
				printf( "%s: ep 0x%02x matched profile 0x%04x, cluster 0x%04x, "
					"mask 0x%02x\n", __FUNCTION__,
					ep->endpoint, profile_id, cluster_id, mask);
			#endif
			return ep;
		}
	}
	#ifdef WPAN_APS_VERBOSE
		printf( "%s: no match for profile 0x%04x, cluster 0x%04x, "
			"mask 0x%02x\n", __FUNCTION__,
			profile_id, cluster_id, mask);
	#endif
	return NULL;
}
#ifdef __XBEE_PLATFORM_HCS08
	#pragma MESSAGE DEFAULT C5909		// restore C5909 (Assignment in condition)
#endif


/*** BeginHeader wpan_conversation_register */
/*** EndHeader */
/** @brief
	Add a conversation to the table of tracked conversations.

	@param[in,out]	state		endpoint state associated with sending endpoint
	@param[in]		handler	handler to call when responses come back,
									or \c NULL to increment and return the
									endpoint's transaction ID
	@param[in]		context	pointer stored in conversation table and passed
									to callback handler
	@param[in]		timeout	number of seconds before generating timeout,
									or 0 for none

	@retval	0-255		transaction ID to use in sent frame
	@retval	-EINVAL	state is invalid (NULL)
	@retval	-ENOSPC	table is full

	@see wpan_endpoint_next_trans
*/
wpan_aps_debug
int wpan_conversation_register( wpan_ep_state_t FAR *state,
	wpan_response_fn handler, const void FAR *context, uint16_t timeout)
{
	wpan_conversation_t FAR *conversation;
	uint_fast8_t i;

	if (! state)
	{
		return -EINVAL;
	}

	if (! handler)
	{
		// caller just wants the next transaction ID
		return ++state->last_transaction;
	}

	conversation = state->conversations;
	for (i = WPAN_MAX_CONVERSATIONS; i; ++conversation, --i)
	{
		if (conversation->handler == NULL)
		{
			// cast away the const -- allow const and non-const in conversation
			conversation->context = (void FAR *)context;
			conversation->handler = handler;
			if (timeout != 0)
			{
				timeout += (uint16_t) xbee_seconds_timer();
				if (timeout == 0)
				{
					// timeout of 0 is reserved for "never", so add an extra second
					timeout = 1;
				}
			}
			conversation->timeout = timeout;
			return (conversation->transaction_id = ++state->last_transaction);
		}
	}

	return -ENOSPC;
}

/*** BeginHeader wpan_conversation_delete */
/*** EndHeader */
/** @brief
	Delete a conversation from an endpoint's conversation table.

	@param[in,out]	conversation	conversation to delete
*/
wpan_aps_debug
void wpan_conversation_delete( wpan_conversation_t FAR *conversation)
{
	if (conversation != NULL)
	{
		_f_memset( conversation, 0, sizeof *conversation);
	}
}

/*** BeginHeader _wpan_endpoint_expire_conversations */
void _wpan_endpoint_expire_conversations( wpan_ep_state_t FAR *state);
/*** EndHeader */
/**
	@internal @brief
	Walk an endpoint's conversation table and expire any conversations
	that have timed out.

	@param[in]	state		endpoint state (from endpoint table)
*/
wpan_aps_debug
void _wpan_endpoint_expire_conversations( wpan_ep_state_t FAR *state)
{
	wpan_conversation_t FAR *conversation;
	uint_fast8_t i;
	uint16_t now;

	if (state == NULL)
	{
		return;
	}

	/*
		Notes regarding timeout calculation:  We just store the lower 16 bits of
		the xbee_seconds_timer() value of when we should timeout.  We can tell
		whether we're before or after that time by subtracting the target time
		from the curent time.  If the result as a signed integer is >= 0, we
		have passed the selected timeout value.
	*/

	now = (uint16_t) xbee_seconds_timer();
	conversation = state->conversations;
	for (i = WPAN_MAX_CONVERSATIONS; i; ++conversation, --i)
	{
		if (conversation->handler != NULL
			&& conversation->timeout != 0
			&& (int16_t)(now - conversation->timeout) >= 0)
		{
			// send timeout to conversation's handler, ignore the response
			conversation->handler( conversation, NULL);
			wpan_conversation_delete( conversation);
		}
	}
}


/*** BeginHeader wpan_conversation_response */
/*** EndHeader */
/**
	@brief
	Searches the endpoint's table of active conversations (outstanding
	requests waiting for responses) for a given transaction.

	Hands the response frame off to the handler registered to the conversation
	with wpan_conversation_register().

	@param[in]	state		endpoint state (from endpoint table) or NULL to look
								it up using the envelope
	@param[in]	transaction_id		transaction ID from frame payload,
								used to look up conversation
	@param[in]	envelope	envelope from response frame, passed to conversation's
								handler

	@retval	-EINVAL	invalid parameter passed to function, or conversation not
							found in table
	@retval	0			handler processed response
	@retval	!0			handler reported an error while processing response
*/

wpan_aps_debug
int wpan_conversation_response( wpan_ep_state_t FAR *state,
	uint8_t transaction_id, const wpan_envelope_t FAR *envelope)
{
	wpan_conversation_t FAR *conversation;
	uint_fast8_t i;
	int				retval;
	const wpan_endpoint_table_entry_t *ep;

	if (envelope == NULL)
	{
		return -EINVAL;
	}

	if (state == NULL)
	{
		ep = wpan_endpoint_match( envelope->dev, envelope->dest_endpoint,
															envelope->profile_id);
		if (ep == NULL)
		{
			return -EINVAL;
		}

		state = ep->ep_state;
	}

	conversation = state->conversations;
	for (i = WPAN_MAX_CONVERSATIONS; i; ++conversation, --i)
	{
		if (conversation->transaction_id == transaction_id)
		{
			#ifdef WPAN_APS_VERBOSE
				printf( "%s: matched conversation %u (handler=%p)\n", __FUNCTION__,
					WPAN_MAX_CONVERSATIONS - i, conversation->handler);
			#endif
			if (conversation->handler)
			{
				retval = conversation->handler( conversation, envelope);
				if (retval == WPAN_CONVERSATION_END)
				{
					wpan_conversation_delete( conversation);
				}
				#ifdef WPAN_APS_VERBOSE
					else if (retval != WPAN_CONVERSATION_CONTINUE)
					{
						printf( "%s: invalid reponse %d from conversation handler\n",
							__FUNCTION__, retval);
					}
				#endif
				return (retval < 0) ? retval : 0;
			}
		}
	}

	return -EINVAL;			// not found
}

/*** BeginHeader wpan_endpoint_next_trans */
/*** EndHeader */
/** @brief
	Increment and return the endpoint's transaction ID counter.

	@param[in]	ep		entry from endpoint table

	@retval	0-255	current transaction ID for endpoint
*/
wpan_aps_debug
uint8_t wpan_endpoint_next_trans( const wpan_endpoint_table_entry_t *ep)
{
	if (! ep || ! ep->ep_state)
	{
		return 0;
	}
	return ++(ep->ep_state->last_transaction);
}

/*** BeginHeader wpan_envelope_dispatch */
/*** EndHeader */
/**
	@internal
	@brief Dispatch a message for a given endpoint.

	Should only be called from wpan_envelope_dispatch.

	Looks up the cluster ID in the endpoint's cluster table and passes
	\p envelope off to the cluster handler (if a matching cluster was
	found) or the endpoint handler.

	@param[in]	envelope		structure containing all necessary information
									about message (endpoints, cluster, profile, etc.)
	@param[in]	ep				endpoint to dispatch envelope to

	@retval	0	successfully dispatched message
	@retval	-ENOENT	no handler for this endpoint/cluster
	@retval	!0	error dispatching messages
*/
wpan_aps_debug
int _wpan_endpoint_dispatch( wpan_envelope_t *envelope,
	const wpan_endpoint_table_entry_t *ep)
{
	const wpan_cluster_table_entry_t		*clust;

	#ifdef WPAN_APS_VERBOSE
		printf( "%s: found entry for endpoint 0x%02x\n", __FUNCTION__,
			envelope->dest_endpoint);
	#endif
	// Match either an input or an output cluster, since the ZigBee layer
	// doesn't contain information on the direction of the frame.
	clust = wpan_cluster_match( envelope->cluster_id, WPAN_CLUST_FLAG_INOUT,
		ep->cluster_table);
	if (clust)
	{
		// If ZCL cluster requires encryption, make sure frame was encrypted.
		// (test for FLAG_ENCRYPT set and FLAG_NOT_ZCL not set).
		if (!(envelope->options & WPAN_ENVELOPE_RX_APS_ENCRYPT)
			&& !(clust->flags & WPAN_CLUST_FLAG_NOT_ZCL))
		{
			// Send a failure response if we require encryption, or this
			// is a unicast frame and we only accept unencrypted broadcasts.
			if ((clust->flags & WPAN_CLUST_FLAG_ENCRYPT)
				|| ((clust->flags & WPAN_CLUST_FLAG_ENCRYPT_UNICAST)
					&& !(envelope->options & WPAN_ENVELOPE_BROADCAST_ADDR)))
			{
				#ifdef WPAN_APS_VERBOSE
					printf( "%s: sending FAILURE for unencrypted APS frame\n",
						__FUNCTION__);
				#endif
				// send failure, we don't accept unencrypted broadcast frames
				return zcl_invalid_cluster( envelope, NULL);
			}
		}

		envelope->options |= clust->flags;

		if (! clust->handler)
		{
			#ifdef WPAN_APS_VERBOSE
				printf( "%s: cluster 0x%04x has null handler\n",
					__FUNCTION__, envelope->cluster_id);
			#endif
		}
		else
		{
			#ifdef WPAN_APS_VERBOSE
				printf( "%s: calling handler for cluster 0x%04x\n",
					__FUNCTION__, envelope->cluster_id);
			#endif
			// Note that we cast away const when selecting the context to pass
			// to the cluster handler and assume that the main program has set
			// up its cluster table correctly (and knows whether its context
			// pointer should be treated as const or not).
			return clust->handler( envelope, (void FAR *) clust->context);
		}
	}

	// We don't have that cluster (or the cluster doesn't have a handler) so
	// use the endpoint's handler.
	if (ep->handler)
	{
		#ifdef WPAN_APS_VERBOSE
			printf( "%s: no entry for cluster 0x%04x, use default handler\n",
				__FUNCTION__, envelope->cluster_id);
		#endif

		return ep->handler( envelope, ep->ep_state);
	}

	#ifdef WPAN_APS_VERBOSE
		printf( "%s: no handler for 0x%02x/0x%04x, ignoring frame\n",
			__FUNCTION__, envelope->dest_endpoint, envelope->cluster_id);
	#endif
	return -ENOENT;
}

/** @brief
	Find the matching endpoint for the provided \p envelope and have it process
	the frame.

	In the case of the broadcast endpoint (255), dispatches the
	frame to all endpoints matching the envelope's profile ID.

	Looks up the destination endpoint and cluster ID in the device's endpoint
	table and passes \p envelope off to the cluster handler (if a matching
	cluster was found) or the endpoint handler.

	@param[in]	envelope		structure containing all necessary information
									about message (endpoints, cluster, profile, etc.)

	@retval	0	successfully dispatched message
	@retval	-ENOENT	no handler for this endpoint/cluster
	@retval	!0	error dispatching messages
*/
#ifdef __XBEE_PLATFORM_HCS08
	#pragma MESSAGE DISABLE C5909		// Assignment in condition is OK
#endif
wpan_aps_debug
int wpan_envelope_dispatch( wpan_envelope_t *envelope)
{
	const wpan_endpoint_table_entry_t	*ep;
	uint_fast8_t								match_ep;
	int											retval = -ENOENT;

	#ifdef WPAN_APS_VERBOSE
		printf( "%s: RX ", __FUNCTION__);
		wpan_envelope_dump( envelope);
	#endif

	// find matching endpoints
	match_ep = envelope->dest_endpoint;

	if (match_ep == WPAN_ENDPOINT_BROADCAST)
	{
		// broadcast endpoint, dispatch to all endpoints on device matching
		// the given profile ID

		envelope->options |= WPAN_ENVELOPE_BROADCAST_EP;
		ep = NULL;
		// Assignment in next line is intentional (Warning C5909)
		while ( (ep = wpan_endpoint_get_next( envelope->dev, ep)) != NULL)
		{
			if (envelope->profile_id == ep->profile_id)
			{
				// update endpoint to match endpoint we're dispatching to
				envelope->dest_endpoint = ep->endpoint;

				// clear cluster flags possibly set by previous call to
				// _wpan_endpoint_dispatch
				envelope->options &= ~WPAN_ENVELOPE_CLUSTER_FLAGS;

				if (_wpan_endpoint_dispatch( envelope, ep) == 0)
				{
					retval = 0;		// dispatched to at least one endpoint
				}
			}
		}
		return retval;
	}

	ep = wpan_endpoint_match( envelope->dev, match_ep, envelope->profile_id);
	if ( ep != NULL)
	{
		return _wpan_endpoint_dispatch( envelope, ep);
	}

	#ifdef WPAN_APS_VERBOSE
		printf( "%s: no entry for ep 0x%02x profile 0x%04x, ignoring frame\n",
			__FUNCTION__, match_ep, envelope->profile_id);
	#endif

	return retval;
}
#ifdef __XBEE_PLATFORM_HCS08
	#pragma MESSAGE DEFAULT C5909		// restore C5909 (Assignment in condition)
#endif


/*** BeginHeader wpan_envelope_create */
/*** EndHeader */
/** @brief
	Starting with a blank envelope, fill in the device, 64-bit IEEE address and
	16-bit network address of the destination.  The profile ID, cluster ID,
	source/destination endpoints and envelope options are all be set to zero.

	@param[out]	envelope			envelope to populate
	@param[in]	dev				device that will send this envelope
	@param[in]	ieee				64-bit IEEE/MAC address of recipient or one of
				- #WPAN_IEEE_ADDR_COORDINATOR (send to coordinator)
				- #WPAN_IEEE_ADDR_BROADCAST (broadcast packet)
				- #WPAN_IEEE_ADDR_UNDEFINED (use network address only)
	@param[in]	network_addr	16-bit network address of recipient or one of
				- #WPAN_NET_ADDR_COORDINATOR for the coordinator (use the
					coordinator's actual IEEE address for the \a ieee parameter).
				- #WPAN_NET_ADDR_BCAST_ALL_NODES (broadcast to all nodes)
				- #WPAN_NET_ADDR_BCAST_NOT_ASLEEP (broadcast to awake nodes)
				- #WPAN_NET_ADDR_BCAST_ROUTERS (broadcast to routers/coordinator)
				- #WPAN_NET_ADDR_UNDEFINED (use 64-bit address only)

	When sending broadcast packets, use WPAN_IEEE_ADDR_BROADCAST and
	WPAN_NET_ADDR_UNDEFINED.  Don't set both addresses to broadcast.

	When using DigiMesh protocol, always set the \p network_addr to
	WPAN_NET_ADDR_UNDEFINED.

	@see wpan_envelope_reply()
*/
wpan_aps_debug
void wpan_envelope_create( wpan_envelope_t *envelope, wpan_dev_t *dev,
	const addr64 FAR *ieee, uint16_t network_addr)
{
	if (envelope != NULL)
	{
		memset( envelope, 0, sizeof (*envelope));

		envelope->dev = dev;
		envelope->ieee_address = *ieee;
		envelope->network_address = network_addr;
	}
}

/*** BeginHeader wpan_envelope_reply */
/*** EndHeader */
/** @brief
	Create a reply envelope based on the envelope received from a
	remote node.

	Copies the interface, addresses, profile and cluster from the
	original envelope, and then swaps the source and destination
	endpoints.

	Note that it may be necessary for the caller to change the
	\a cluster_id as well, after building the reply envelope.
	For example, ZDO responses set the high bit of the request's
	cluster ID.

	@param[out]	reply		Buffer for storing the reply envelope.

	@param[in]	original	Original envelope, received from a remote node,
								to base the reply on.

	@retval	0			addressed reply envelope
	@retval	-EINVAL	either parameter is \c NULL or they point to
							the same address

	@see wpan_envelope_create()
*/
wpan_aps_debug
int wpan_envelope_reply( wpan_envelope_t FAR *reply,
	const wpan_envelope_t FAR *original)
{
	if (! (reply && original) || original == reply)
	{
		return -EINVAL;
	}

	// Copy dev, ieee_address, network_address, profile_id, and cluster_id.
	_f_memcpy( reply, original, offsetof( wpan_envelope_t, source_endpoint));

	// swap source and destination endpoints
	reply->source_endpoint = original->dest_endpoint;
	reply->dest_endpoint = original->source_endpoint;

	// Use APS encryption if replying to an encrypted frame.
	reply->options = (original->options & WPAN_ENVELOPE_RX_APS_ENCRYPT)
		? WPAN_CLUST_FLAG_ENCRYPT : 0;

	// Clear payload
	reply->payload = NULL;
	reply->length = 0;

	return 0;
}

/*** BeginHeader wpan_envelope_send */
/*** EndHeader */
/** @brief
	Send a message to an endpoint using address and payload information stored
	in a wpan_envelope_t structure.

	@param[in]	envelope		envelope of request to send

	@retval	0	request sent
	@retval	!0	error trying to send request
*/
wpan_aps_debug
int wpan_envelope_send( const wpan_envelope_t FAR *envelope)
{
	if (envelope == NULL
		|| envelope->dev == NULL
		|| envelope->dev->endpoint_send == NULL)
	{
		#ifdef WPAN_APS_VERBOSE
			printf( "%s: return -EINVAL; NULL envelope->dev->send\n",
				__FUNCTION__);
		#endif
		return -EINVAL;
	}

	#ifdef WPAN_APS_VERBOSE
		printf( "%s: TX ", __FUNCTION__);
		wpan_envelope_dump( envelope);
	#endif
	return envelope->dev->endpoint_send( envelope,
		(envelope->options & WPAN_CLUST_FLAG_ENCRYPT)
		? WPAN_SEND_FLAG_ENCRYPTED : WPAN_SEND_FLAG_NONE );
}

/*** BeginHeader wpan_envelope_dump */
/*** EndHeader */
/** @brief
	Debugging function to dump the contents of an envelope to stdout.

	Displays
	all fields from the envelope, plus the contents of the payload

	@param[in]	envelope		envelope to dump
*/
wpan_aps_debug
void wpan_envelope_dump( const wpan_envelope_t FAR *envelope)
{
	char addr[ADDR64_STRING_LENGTH];

	if (envelope == NULL)
	{
		printf( "NULL envelope\n");
	}
	else
	{
		printf( "ep %u->%u, profile 0x%04x, clust 0x%04x\n",
			envelope->source_endpoint, envelope->dest_endpoint,
			envelope->profile_id, envelope->cluster_id);
		printf( "remote %" PRIsFAR " (0x%04x), options=0x%04x, "
			"%u-byte payload:\n", addr64_format( addr, &envelope->ieee_address),
			envelope->network_address, envelope->options, envelope->length);
		hex_dump( envelope->payload, envelope->length, HEX_DUMP_FLAG_OFFSET);
	}
}

/*** BeginHeader wpan_tick */
/*** EndHeader */
/**
	@brief
	Calls the underlying hardware tick function to process received frames,
	and times out expired conversations.

	@param[in]	dev	WPAN device to tick

	@retval	>=0		number of frames processed during the tick
	@retval	-EINVAL	device does not have a \c tick function assigned to it
	@retval	<0			some other error encountered during the tick
*/
#ifdef __XBEE_PLATFORM_HCS08
	#pragma MESSAGE DISABLE C5909		// Assignment in condition is OK
#endif
wpan_aps_debug
int wpan_tick( wpan_dev_t *dev)
{
	const wpan_endpoint_table_entry_t *ep;
	int retval = -EINVAL;

	if (dev != NULL)
	{
		if (dev->tick)
		{
			retval = dev->tick( dev);
		}

		// walk endpoint table, expiring each endpoint's conversations
		ep = NULL;
		while ( (ep = wpan_endpoint_get_next( dev, ep)) != NULL)
		{
			_wpan_endpoint_expire_conversations( ep->ep_state);
		}
	}

	return retval;
}
#ifdef __XBEE_PLATFORM_HCS08
	#pragma MESSAGE DEFAULT C5909		// restore C5909 (Assignment in condition)
#endif
