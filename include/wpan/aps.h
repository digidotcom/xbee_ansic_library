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
	@file wpan/aps.h
	Cluster/Endpoint layer for WPAN networks (ZigBee and DigiMesh).
*/

#ifndef __WPAN_APS_H
#define __WPAN_APS_H

#include "wpan/types.h"

XBEE_BEGIN_DECLS

/**
	The "envelope" is used to gather all necessary information about a given
	frame on the network.  Note that all members of the structure are in
	host byte order.

	The low byte of the \a options field is a copy of the cluster flags for
	the cluster that received the data.  Use #WPAN_ENVELOPE_CLUSTER_FLAGS
	to mask off the byte, and then compare to the various WPAN_CLUST_FLAG_*
	macros.

	On a DigiMesh network, the 16-bit network address is always set to
	WPAN_NET_ADDR_UNDEFINED (0xFFFE).
*/
typedef struct wpan_envelope_t {
	// first group of members are common when creating reply envelope
	struct wpan_dev_t		*dev;					///< interface received on/to send to
	addr64					ieee_address;		///< remote 64-bit address
	uint16_t					network_address;	///< remote 16-bit address
	uint16_t					profile_id;			///< Profile ID
	uint16_t					cluster_id;			///< Cluster ID

	// second group of members are modified when creating reply envelope
	uint8_t					source_endpoint;	///< endpoint on sender (source)
	uint8_t					dest_endpoint;		///< endpoint on recipient (dest)
	uint16_t					options;
		/// mask of WPAN_CLUST_FLAG_* flags for the cluster tied to this envelope
		#define WPAN_ENVELOPE_CLUSTER_FLAGS		0x00FF
		/// frame was received on a broadcast address
		#define WPAN_ENVELOPE_BROADCAST_ADDR	0x0100
		/// frame was received on the broadcast endpoint (0xFF)
		#define WPAN_ENVELOPE_BROADCAST_EP		0x0200
		/// frame was received with APS encryption
		#define WPAN_ENVELOPE_RX_APS_ENCRYPT	0x0400

	const void		FAR	*payload;			///< contents of message
	uint16_t 				length;				///< number of bytes in payload
} wpan_envelope_t;

/** @name ZigBee Stack Profile IDs
	4-bit values used in ZigBee beacons
@{
*/
/// Network Specific
#define WPAN_STACK_PROFILE_PROPRIETARY				0x0
/// ZigBee (2006)
#define WPAN_STACK_PROFILE_ZIGBEE					0x1
/// ZigBee PRO (2007)
#define WPAN_STACK_PROFILE_ZIGBEE_PRO				0x2
//@}

//@name Profile IDs
//@{
/// ZigBee Device Object (aka ZigBee Device Profile)
#define WPAN_PROFILE_ZDO				0x0000

/// Smart Energy Profile
#define WPAN_PROFILE_SMART_ENERGY	0x0109

/// Digi International, mfg-specific
#define WPAN_PROFILE_DIGI				0xC105
//@}

/** @name Manufacturer IDs
	Contact the ZigBee Alliance to have a Manufacturer ID assigned to your
	company.  DO NOT use the Digi Manufacturer ID for your own
	manufacturer-specific profiles/clusters/attributes.
@{
*/
/// Digi International (MaxStream)
#define WPAN_MANUFACTURER_DIGI		0x101E
/// Digi International
#define WPAN_MANUFACTURER_DIGI2		0x1087

/// Test Vendor #1
#define WPAN_MANUFACTURER_TEST1		0xFFF1
/// Test Vendor #2
#define WPAN_MANUFACTURER_TEST2		0xFFF2
/// Test Vendor #3
#define WPAN_MANUFACTURER_TEST3		0xFFF3
//@}

//@name List of fixed endpoints
//@{
/// ZigBee Device Object/Profile
#define WPAN_ENDPOINT_ZDO			0x00
/// Digi Smart Energy
#define WPAN_ENDPOINT_DIGI_SE		0x5E
/// Digi Device Objects
#define WPAN_ENDPOINT_DDO			0xE6
/// Digi Data
#define WPAN_ENDPOINT_DIGI_DATA	0xE8
/// Broadcast Endpoint
#define WPAN_ENDPOINT_BROADCAST	0xFF
//@}

/// Digi Data cluster IDs (endpoint #WPAN_ENDPOINT_DIGI_DATA)
// (cast to int required by Codewarrior/HCS08 platform if enum is signed)
enum wpan_clust_digi
{
	DIGI_CLUST_SLEEP_SYNC			= 0x0009,	///< DigiMesh Sleeping Router Sync
	DIGI_CLUST_SERIAL					= 0x0011,	///< Serial data
	DIGI_CLUST_LOOPBACK				= 0x0012,	///< Serial loopback transmit
	DIGI_CLUST_ND_COMMAND			= 0x0015,	///< Digi ND Command
	DIGI_CLUST_DN_COMMAND			= 0x0016,	///< Digi DN Command
	DIGI_CLUST_REMOTE_COMMAND		= 0x0021,	///< Remote Command Request
	DIGI_CLUST_NR_COMMAND			= 0x0022,	///< Digi NR Command
	DIGI_CLUST_IODATA					= 0x0092,	///< Unsolicited I/O data sample
	DIGI_CLUST_WATCHPORT				= 0x0094,	///< Unsolicited Watchport sensor
	DIGI_CLUST_NODEID_MESSAGE		= 0x0095,	///< Node ID Message
	DIGI_CLUST_DN_RESPONSE			= 0x0096,	///< Digi DN Response
	DIGI_CLUST_REMOTE_RESPONSE		= 0x00A1,	///< Digi Remote Command Response
	DIGI_CLUST_NR_RESPONSE			= 0x00A2,	///< Digi NR Response
	DIGI_CLUST_INDIRECT_ROUTE_ERR	= 0x00B0,
	DIGI_CLUST_PROG_XBEE_OTA_UPD	= 0x1000,	///< Start OTA update of PXBee App

	// Unsure of how these clusters are used.
	DIGI_CLUST_NBRFWUPDATE			= 0x71FE,	///< Neighbor FW update
	DIGI_CLUST_REMFWUPDATE			= 0x71FF,	///< Remote FW update
	DIGI_CLUST_FWUPDATERESP			= (int)0xF1FF,	///< FW update response
};

/**
	@brief
	General handler used in the cluster table.

	Dispatcher searches an endpoint's cluster table for a matching cluster,
	and hands the frame off to the cluster's handler.

	If the cluster's handler is set to \c NULL, or the frame is for a cluster
	ID that does not appear in the table, the dispatcher hands the frame
	off to the endpoint's handler.

	@param[in]		envelope	information about the frame (addresses, endpoint,
									profile, cluster, etc.)
	@param[in,out]	context	user context (from cluster table)

	@retval	0	handled data
	@retval	!0	some sort of error processing data
*/
typedef int (*wpan_aps_handler_fn)(
	const wpan_envelope_t	FAR	*envelope,
	void							FAR	*context
);

// Actual definition comes later, declare here so we can use it in
// function pointer typedefs.
struct wpan_ep_state_t;

/**
	@brief
	General handler used in the endpoint table.

	If a cluster's handler is set to \c NULL, or the frame is for a cluster
	ID that does not appear in the table, the dispatcher hands the frame
	off to the endpoint's handler.

	@param[in]		envelope	information about the frame (addresses, endpoint,
									profile, cluster, etc.)
	@param[in,out]	ep_state	pointer to endpoint state structure

	@retval	0	handled data
	@retval	!0	some sort of error processing data
*/
typedef int (*wpan_ep_handler_fn)(
	const wpan_envelope_t	FAR	*envelope,
	struct wpan_ep_state_t	FAR	*ep_state
);

/// Information on each cluster associated with an endpoint.
typedef struct wpan_cluster_table_entry_t {
	/// 16-bit cluster id, in host byte order.  WPAN_CLUSTER_END_OF_LIST
	/// (0xFFFF) marks the end of the list.  Clusters 0x0000 to 0x7FFF
	/// are Standard ZigBee clusters, 0x8000 to 0xFBFF are reserved and
	/// 0xFC00 to 0xFFFF are manufacturer-specific (with a standard ZigBee
	/// profile).
	uint16_t								cluster_id;

	/// Function to receive all frames for this cluster, or NULL to have
	/// endpoint's handler process the frame.
	wpan_aps_handler_fn				handler;

	/// Declared \c const so initializers can use \c const or non-\c const
	/// pointers.  The \c const is discarded before passing the context
	/// on to wpan_aps_handler_fn().  For a ZCL endpoint, \c context points
	/// to an attribute tree (zcl_attribute_tree_t).
	const void					FAR *context;

	/// flags that apply to this cluster, see WPAN_CLUST_FLAG_* macros
	uint8_t								flags;
		/** @name
			@{
			Values for \c flags field of wpan_cluster_table_entry_t.
		*/
		/// no flags
		#define WPAN_CLUST_FLAG_NONE					0x00
		/// input/server cluster (typically receives requests)
		#define WPAN_CLUST_FLAG_INPUT					0x01
		/// output/client cluster (typically receives responses)
		#define WPAN_CLUST_FLAG_OUTPUT				0x02
		/// both client and server cluster
		#define WPAN_CLUST_FLAG_INOUT					0x03
		/// alias name for input cluster (uses ZCL terminology)
		#define WPAN_CLUST_FLAG_SERVER				WPAN_CLUST_FLAG_INPUT
		/// alias name for output cluster (uses ZCL terminology)
		#define WPAN_CLUST_FLAG_CLIENT				WPAN_CLUST_FLAG_OUTPUT
		/// Data sent or received by this cluster must be encrypted.
		/// Do not accept unencrypted broadcast messages.
		/// If using this flag on a non-ZCL cluster, be sure to set
		/// WPAN_CLUST_FLAG_NOT_ZCL as well.
		#define WPAN_CLUST_FLAG_ENCRYPT				0x10
		/// Unicast data sent or received by this cluster must be
		/// encrypted, but unencrypted broadcast frames are OK.
		/// If using this flag on a non-ZCL cluster, be sure to set
		/// WPAN_CLUST_FLAG_NOT_ZCL as well.
		#define WPAN_CLUST_FLAG_ENCRYPT_UNICAST	0x20
		/// this cluster is NOT using the ZigBee Cluster Library (ZCL)
		#define WPAN_CLUST_FLAG_NOT_ZCL				0x80
		//@}
} wpan_cluster_table_entry_t;

/// Information on each endpoint on this device.
typedef struct wpan_endpoint_table_entry_t
{
	/// Endpoint ID, 0 to 254.  255 (0xFF) is used as an end-of-table marker.
	uint8_t							endpoint;

	/// This endpoint's profile ID.  See WPAN_PROFILE_* macros for some known
	/// profile IDs.
	uint16_t							profile_id;

	/// Function to receive all frames for invalid clusters, or clusters with
	/// a \c NULL handler.
	wpan_ep_handler_fn			handler;

	/// Structure used to track transactions and conversations on ZDO/ZDP and
	/// ZCL endpoints.  Should be NULL for other types of endpoints.
	struct wpan_ep_state_t	FAR	*ep_state;

	/// This endpoint's device ID.
	uint16_t							device_id;

	/// Lower 4 bits are used, upper 4 are reserved and should be 0.
	uint8_t							device_version;

	/// Pointers to a list of clusters that ends with WPAN_CLUST_ENTRY_LIST_END.
	/// Maximum of 255 input and 255 output clusters (Simple Descriptor uses
	/// 8-bit fields for input cluster count and output cluster count).
	const wpan_cluster_table_entry_t	*cluster_table;
} wpan_endpoint_table_entry_t;

/// Cluster ID used to mark the end of \c cluster_table in
/// wpan_endpoint_table_entry_t.
#define WPAN_CLUSTER_END_OF_LIST		0xFFFF

/// Macro for a wpan_cluster_table_entry_t that can be used to mark the end
/// of the table.
#define WPAN_CLUST_ENTRY_LIST_END	{ WPAN_CLUSTER_END_OF_LIST }

// All wpan-capable devices need to start with the wpan_dev_t struct so that the
// wpan layer can share some elements (ieee_address, network_address, payload)
// and have access to the function pointers.
// Set function pointers to NULL if not implemented on a particular device.

/**
	@brief
	Function called by the WPAN APS layer to send a frame out on the network.

	This is part of the glue that links the XBee layer with the WPAN/ZigBee
	layers.

	@param[in]		envelope	information about the frame (addresses, endpoint,
									profile, cluster, payload, etc.)
	@param[in]		flags		flags related to sending, see WPAN_SEND_FLAG_*
									macros
				- #WPAN_SEND_FLAG_NONE: no special behavior
				- #WPAN_SEND_FLAG_ENCRYPTED: use APS layer encryption

	@retval	0				frame sent
	@retval	!0				error sending frame
	@retval	-EMSGSIZE	payload is too large

	@todo Add support for a broadcast radius?  Don't send Tx Status?  Use
			frame ID 0?

	@todo come up with standard error codes for the following possible errors?
	outbound frame buffer is full, invalid data in envelope, payload is too big
*/
typedef int (*wpan_endpoint_send_fn)( const wpan_envelope_t FAR *envelope,
	uint16_t flags);

/** @name
	Bitfields for \c flags parameter to a wpan_endpoint_send_fn().
	@{
*/
#define WPAN_SEND_FLAG_NONE			0x0000
#define WPAN_SEND_FLAG_ENCRYPTED		0x0001
//@}

/**
	@internal
	@brief
	Function called by the WPAN APS layer to configure the underlying network
	device.

	Exact implementation is currently undefined, but this entry
	exists for future expansion of the API.

	@param[in]		dev	device to configure
	@param			...	variable arguments to the function

	@retval	0	configuration request processed successfully
	@retval	>0	error parsing parameter #\c retval
*/
typedef int (*_wpan_config_fn)( struct wpan_dev_t *dev, ...);

/**
	@brief
	Function called by the WPAN APS layer to have the underlying device read
	and dispatch frames.

	@param[in]		dev	device to tick

	@return	number of frames processed during the tick
*/
typedef int (*wpan_tick_fn)( struct wpan_dev_t *dev);

/**
	@brief
	Custom function for walking the endpoint table.

	The application can define its own function to walk the endpoint table,
	possibly to support a dynamic table or a table where endpoints aren't
	always visible.

	@param[in]	dev	device with endpoint table to walk
	@param[in]	ep		NULL to return first entry in table, or a pointer
							previously returned by this function to get the
							next entry

	@retval	NULL	\a dev is invalid or reached end of table
	@retval	!NULL	next entry from table

	@see wpan_endpoint_match(), wpan_cluster_match(), wpan_endpoint_get_next()
*/
typedef const wpan_endpoint_table_entry_t *(*wpan_endpoint_get_next_fn)(
	struct wpan_dev_t *dev, const wpan_endpoint_table_entry_t *ep);

/**
	Structure used by the WPAN/ZigBee layers.  Contains information about the
	node (addresses, payload limit, capabilities) along with an endpoint
	list and function pointers to configure, tick and send packets through
	the underlying network interface.

	This is the abstraction layer between a physical XBee (or some other
	piece of hardware or gateway/router/tunnel device) and the networking
	layers.  Similar to a driver for an Ethernet NIC and the TCP/IP stack
	that runs on top of it.
*/
typedef struct wpan_dev_t {
//	_wpan_config_fn				config;			///< change device's cfg
	wpan_tick_fn					tick;				///< read and dispatch frames
	wpan_endpoint_send_fn 		endpoint_send;	///< send frame to an endpoint
	wpan_endpoint_get_next_fn	endpoint_get_next;	///< walk endpoint table

	wpan_address_t	address;						///< IEEE/MAC and network addresses

	/// max bytes in RF payload, need to refresh if encryption enabled/disabled
	uint16_t			payload;

	/// Bitfield describing the device's capabilities (maybe CAN_SLEEP,
	/// nodetype, etc.) and state.  Only valid if xbee_cmd_init_device() is
	/// used or ALL Modem Status messages are received from radio.
	/// User code should not access this directly, but use WPAN_DEV_IS_*
	/// macros instead.
	uint16_t			flags;
		#define WPAN_FLAG_NONE							0x0000
		/// device has joined a network (but not necessarily authenticated)
		#define WPAN_FLAG_JOINED						0x0001
		/// device completed Key Establishment and can send APS encrypted frames
		#define WPAN_FLAG_AUTHENTICATED				0x0002
		/// device has encryption enabled (EO non-zero)
		#define WPAN_FLAG_AUTHENTICATION_ENABLED	0x0004

	/// Pointer to a table of the device's endpoints, ending with
	/// #WPAN_ENDPOINT_TABLE_END.  Do not reference directly!  Use
	/// wpan_endpoint_get_next() to walk the table.
	const wpan_endpoint_table_entry_t	*endpoint_table;

} wpan_dev_t;

/// Macro to test whether a device has joined the network.
/// @param[in]	dev	(wpan_dev_t FAR *) to test
/// @retval 0	device has not joined the network
/// @retval	!0	device has joined the network
#define WPAN_DEV_IS_JOINED(dev)			((dev)->flags & WPAN_FLAG_JOINED)

/// Macro to test whether a device has authenticated with the trust center.
/// @param[in]	dev	(wpan_dev_t FAR *) to test
/// @retval 0	device has authenticated
/// @retval	!0	device has not authenticated
#define WPAN_DEV_IS_AUTHENTICATED(dev)	((dev)->flags & WPAN_FLAG_AUTHENTICATED)

/// Endpoint ID used to mark the end of \c endpoint_table in wpan_dev_t.
#define WPAN_ENDPOINT_END_OF_LIST	0xFF

/// Macro for a wpan_endpoint_table_entry_t that can be used to mark the end
/// of the table.
#define WPAN_ENDPOINT_TABLE_END		{ WPAN_ENDPOINT_END_OF_LIST }


int wpan_tick( wpan_dev_t *dev);

const wpan_cluster_table_entry_t *wpan_cluster_match( uint16_t match,
	uint8_t mask, const wpan_cluster_table_entry_t *entry);

const wpan_endpoint_table_entry_t *wpan_endpoint_get_next( wpan_dev_t *dev,
	const wpan_endpoint_table_entry_t *ep);

const wpan_endpoint_table_entry_t *wpan_endpoint_match( wpan_dev_t *dev,
	uint8_t endpoint, uint16_t profile_id);

const wpan_endpoint_table_entry_t
	*wpan_endpoint_of_envelope( const wpan_envelope_t *env);

const wpan_endpoint_table_entry_t *wpan_endpoint_of_cluster( wpan_dev_t *dev,
	uint16_t profile_id, uint16_t cluster_id, uint8_t mask);

// DEVNOTE: Do we need to use the platform-independent casting macros
//				to cast this value to uint16_t?
#define WPAN_APS_PROFILE_ANY		0xFFFF

void wpan_envelope_create( wpan_envelope_t *envelope, wpan_dev_t *dev,
	const addr64 FAR *ieee, uint16_t network_addr);

int wpan_envelope_reply( wpan_envelope_t FAR *reply,
	const wpan_envelope_t FAR *original);

int wpan_envelope_send( const wpan_envelope_t FAR *envelope);

void wpan_envelope_dump( const wpan_envelope_t FAR *envelope);

// Forward declaration of struct for use in function pointer declaration.
struct wpan_conversation_t;
/**
	@brief
	Handler registered with wpan_conversation_register() to process responses
	to outstanding requests.

	@param[in]	conversation	Pointer to entry in conversation table.  Handler
										can use the pointer to access the context and
										transaction ID from the conversation, or request
										deletion of the conversation via
										wpan_conversation_delete().  Never NULL.
	@param[in]	envelope			envelope with response or NULL if there was a
										timeout

	@retval	WPAN_CONVERSATION_END	end of conversation, no more responses
												expected
	@retval	WPAN_CONVERSATION_CONTINUE		leave conversation open, more
														responses are expected
*/
typedef int (*wpan_response_fn)(
	struct wpan_conversation_t	FAR	*conversation,
	const wpan_envelope_t		FAR	*envelope);

// Return values for wpan_response_fn() function handlers.
// Reserve 0 for invalid response and negative for error responses.
#define WPAN_CONVERSATION_END			1
#define WPAN_CONVERSATION_CONTINUE	2

/// Used to track conversations (outstanding requests) on an endpoint.
typedef struct wpan_conversation_t {
	uint8_t				transaction_id;
	void			FAR *context;
	/// Time to expire conversation (0 for never or lower 16 bits of
	/// xbee_seconds_timer) -- limited to 9 hours (32,768 seconds).
	/// We could save another byte if it's ok to limit to 2 minutes
	/// and we'll check for expirations at least every 7 seconds.
	uint16_t				timeout;				// time to expire converstation
	wpan_response_fn	handler;				// if NULL, record is unused
} wpan_conversation_t;

/// Number of outstanding requests to track in a wpan_ep_state_t associated
/// with an endpoint.
#ifndef WPAN_MAX_CONVERSATIONS
	#define WPAN_MAX_CONVERSATIONS 3
#endif

/// Default conversation recycle timeout (seconds)
#ifndef WPAN_CONVERSATION_TIMEOUT
	#define WPAN_CONVERSATION_TIMEOUT 30
#endif

/// Volatile part of an endpoint record used to track conversations
/// (requests waiting for responses).
typedef struct wpan_ep_state_t {
	uint8_t					last_transaction;
	wpan_conversation_t	conversations[WPAN_MAX_CONVERSATIONS];
} wpan_ep_state_t;

int wpan_conversation_register( wpan_ep_state_t FAR *state,
	wpan_response_fn handler, const void FAR *context, uint16_t timeout);
int wpan_conversation_response( wpan_ep_state_t FAR *state,
	uint8_t transaction_id, const wpan_envelope_t FAR *envelope);
uint8_t wpan_endpoint_next_trans( const wpan_endpoint_table_entry_t *ep);

int wpan_envelope_dispatch( wpan_envelope_t *envelope);

void wpan_conversation_delete( wpan_conversation_t FAR *conversation);

XBEE_END_DECLS

// If compiling in Dynamic C, automatically #use the appropriate C file.
#ifdef __DC__
	#use "wpan_aps.c"
#endif

#endif		// __WPAN_APS_H
