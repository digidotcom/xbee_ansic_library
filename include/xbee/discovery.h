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
	@file xbee/discovery.h
	Header for code related to "Node Discovery" (the ATND command, 0x95 frames)

	Note that Node Discovery isn't supported on XBee Smart Energy firmware.

	A host can initiate Node Discovery by sending an ATND command to its local
	XBee.  The command can include a parameter of the NI string of a single
	node on the network to discover.  See the xbee_disc_discover_nodes() API
	for sending this request.

	Responses come in as AT Command Response frames (type 0x88) containing a
	Node ID message.

	Additionally, nodes on a network will broadcast unsolicited (i.e., not in
	response to an ATND command) Node ID messages to the rest of the network
	in two instances:

		1) The device has ATJN set to 1, and it has just joined the network.

		2) The user has configured D0 as the commissioning button, and presses
			the commissioning button once.

	The receiving node will pass the message on to the host in one of two ways:

		1) If AO is set to 0, the host receives a Node ID frame (type 0x95).

		2) If AO is non-zero, the host receives an Explicit Rx frame (type 0x91)
			for the Node ID Message cluster (0x0095) of the Digi Data endpoint
			(0xE8).

	This library provides an API layer for handling these responses.  The host
	needs to follow these steps to receive NODE ID messages.

		1) Write a function to receive the messages, with the following signature:
				void node_discovered( xbee_dev_t *xbee, const xbee_node_id_t *rec)

		2) Register that function at startup to receive messages:
				xbee_disc_add_node_id_handler( &my_xbee, &node_discovered);

		3) Include the ATND Response handler in the xbee_frame_handlers table
			using the macro XBEE_FRAME_HANDLE_ATND_RESPONSE.

		4) If using a module with ATAO set to 0, include the Node ID handler
			in the xbee_frame_handlers table using the macro
			XBEE_FRAME_HANDLE_AO0_NODEID.

		5) If using a module with ATAO set to 1 or 3, include the Node ID Cluster
			handler in the Digi Data endpoint's cluster table, using the
			XBEE_DISC_DIGI_DATA_CLUSTER_ENTRY macro, and include the Explicit RX
			handler in the xbee_frame_handlers table using the macro
			XBEE_FRAME_HANDLE_RX_EXPLICIT.
*/

#ifndef XBEE_DISCOVERY_H
#define XBEE_DISCOVERY_H

#include "xbee/platform.h"
#include "xbee/device.h"
#include "wpan/types.h"

XBEE_BEGIN_DECLS

/// Maximum length of a Node ID string (ATNI value) (excludes null terminator).
#define XBEE_DISC_MAX_NODEID_LEN 20

typedef PACKED_STRUCT xbee_node_id1_t {
	uint16_t			network_addr_be;		///< ATMY value [0xFFFE on DigiMesh]
	addr64			ieee_addr_be;			///< ATSH and ATSL
	/// null-terminated ATNI value (variable length)
	char				node_info[XBEE_DISC_MAX_NODEID_LEN + 1];
} xbee_node_id1_t;

// data following variable-length node info
typedef PACKED_STRUCT xbee_node_id2_t {
	uint16_t			parent_addr_be;		///< ATMP value [0xFFFE on DigiMesh]

	uint8_t			device_type;			///< one of XBEE_ND_DEVICE_* macro values
		#define XBEE_ND_DEVICE_TYPE_COORD	0
		#define XBEE_ND_DEVICE_TYPE_ROUTER	1
		#define XBEE_ND_DEVICE_TYPE_ENDDEV	2

	uint8_t			source_event;			///< event that generated the ND frame
		#define XBEE_ND_SOURCE_EVENT_NONE			0
		#define XBEE_ND_SOURCE_EVENT_PUSHBUTTON	1
		#define XBEE_ND_SOURCE_EVENT_JOINED			2
		#define XBEE_ND_SOURCE_EVENT_POWER_CYCLE	3

	uint16_t			profile_be;				///< Digi Profile ID (0xC105)
	uint16_t			manufacturer_be;		///< Manufacturer ID (0x101E)

	/// ATDD of remote device [optional field enabled with ATNO on DigiMesh]
	uint32_t			device_id_be;

	/// RSSI of packet [optional field enabled with ATNO on DigiMesh]
	uint8_t			rssi;
} xbee_node_id2_t;

/// format of 0x95 frames received from XBee
typedef PACKED_STRUCT xbee_frame_node_id_t {
	uint8_t				frame_type;			///< XBEE_FRAME_NODE_ID (0x95)
	addr64				ieee_address_be;
	uint16_t				network_address_be;
	uint8_t				options;
	/// variable-length data, parsed with xbee_disc_nd_parse()
	xbee_node_id1_t	node_data;
	// an xbee_node_id2_t follows the variable-length xbee_node_id1_t
} xbee_frame_node_id_t;

/// parsed Node ID in host-byte-order and fixed length fields
typedef struct xbee_node_id_t {
	addr64			ieee_addr_be;		///< ATSH and ATSL
	uint16_t			network_addr;		///< ATMY value
	uint16_t			parent_addr;		///< ATMP
	/// one of XBEE_ND_DEVICE_TYPE_COORD, _ROUTER or _ENDDEV
	uint8_t			device_type;
	/// ATNI value (variable length, null-terminated)
	char				node_info[XBEE_DISC_MAX_NODEID_LEN + 1];
} xbee_node_id_t;

/**
	@brief
	Parse a Node Discovery response and store it in an xbee_node_id_t structure.

	@param[out]	parsed			buffer to hold parsed Node ID message
	@param[in]	source			source of Node ID message, starting with the
										16-bit network address
	@param[in]	source_length	number of bytes in source of Node ID message

	@retval	0						Node ID message parsed and stored in /a parsed
	@retval	-EINVAL				invalid parameter passed to function
	@retval	-EBADMSG				error parsing Node ID message
*/
int xbee_disc_nd_parse( xbee_node_id_t FAR *parsed, const void FAR *source,
		int source_length);

/**
	@brief
	Debugging function used to dump an xbee_node_id_t structure to stdout.

	@param[in]	ni		pointer to an xbee_node_id_t structure
*/
void xbee_disc_node_id_dump( const xbee_node_id_t FAR *ni);

/**
	@brief
	Return a string ("Coord", "Router", "EndDev", or "???") description for
	the "Device Type" field of a Node ID message.

	@param[in]	device_type		the device_type field from a Node ID message

	@returns pointer to a string describing the type, or "???" if
				\a device_type is invalid

	@sa xbee_node_id_t
*/
const char *xbee_disc_device_type_str( uint8_t device_type);

/**
	@brief Process messages sent to the Node ID Message cluster (0x0095) of
			the Digi Data endpoint (0xE8) when ATAO != 0.

	This function parses the frame and then passes an xbee_node_id_t
	structure on to Node ID handlers configured with
	xbee_disc_add_node_id_handler().

	Do not call this function directly, it should be included in the
	Cluster table of the Digi Data endpoint by using the macro
	XBEE_DISC_DIGI_DATA_CLUSTER_ENTRY.

	Hosts communicating to XBee modules running with ATAO set to zero do not
	need to use this handler.

	See the function help for wpan_aps_handler_fn() for full documentation
	on this function's API.

	@sa XBEE_DISC_DIGI_DATA_CLUSTER_ENTRY, xbee_disc_nodeid_frame_handler,
		xbee_disc_atnd_response_handler
*/
int xbee_disc_nodeid_cluster_handler( const wpan_envelope_t FAR *envelope,
	void FAR *context);

/// Include xbee_disc_nodeid_cluster_handler in the cluster table for the
/// the Digi Data endpoint.
#define XBEE_DISC_DIGI_DATA_CLUSTER_ENTRY		\
	{ DIGI_CLUST_NODEID_MESSAGE, xbee_disc_nodeid_cluster_handler, NULL,	\
		WPAN_CLUST_FLAG_INPUT | WPAN_CLUST_FLAG_NOT_ZCL}

/**
	@brief Process AT Command Response frames (type 0x88), looking for ATND
			responses to parse and pass to Node ID handlers.

	This function parses the frame and then passes an xbee_node_id_t
	structure on to Node ID handlers configured with
	xbee_disc_add_node_id_handler(). If the ATND command fails or returns
	a timeout, it passes a NULL xbee_node_id_t to the functions.

	Do not call this function directly, it should be included in the
	XBee Frame Handlers table by using the macro XBEE_FRAME_HANDLE_ATND_RESPONSE.

	See the function help for xbee_frame_handler_fn() for full
	documentation on this function's API.

	@sa XBEE_FRAME_HANDLE_ATND_RESPONSE, xbee_disc_nodeid_frame_handler,
		xbee_disc_nodeid_cluster_handler
*/
int xbee_disc_atnd_response_handler( xbee_dev_t *xbee, const void FAR *raw,
		uint16_t length, void FAR *context);

/// Include xbee_disc_atnd_response_handler in xbee_frame_handlers[].
#define XBEE_FRAME_HANDLE_ATND_RESPONSE		\
	{ XBEE_FRAME_LOCAL_AT_RESPONSE, 0, xbee_disc_atnd_response_handler, NULL }

/**
	@brief Process Node Identification frames (type 0x95), sent when ATAO = 0.

	These frames are sent to the host when ATAO is set to 0 and a remote node:
		- Has ATD0 set to 1, and someone presses the commissioning button once.
		- Has ATJN set to 1, and has joined the network.

	This function parses a the frame and then passes an xbee_node_id_t
	structure on to Node ID handlers configured with
	xbee_disc_add_node_id_handler().

	Do not call this function directly, it should be included in the
	XBee Frame Handlers table by using the macro XBEE_FRAME_HANDLE_AO0_NODEID.
	Hosts communicating to XBee modules running with ATAO set to a non-zero
	value do not need to use this handler.

	See the function help for xbee_frame_handler_fn() for full
	documentation on this function's API.

	@sa xbee_disc_nodeid_cluster_handler, xbee_disc_atnd_response_handler
*/
int xbee_disc_nodeid_frame_handler(xbee_dev_t *xbee, const void FAR *raw,
		uint16_t length, void FAR *context);

/// Include xbee_disc_nodeid_frame_handler in xbee_frame_handlers[] (only
/// necessary if ATAO is set to 0).
#define XBEE_FRAME_HANDLE_AO0_NODEID			\
	{ XBEE_FRAME_NODE_ID, 0, xbee_disc_nodeid_frame_handler, NULL }

/**
	@brief Designate a function to receive parsed Node ID messages on a given
			XBee device.

	@param[in]	xbee		XBee device to monitor
	@param[in]	fn			function to receive parsed Node ID messages

	@retval	0				function assigned
	@retval	-EINVAL		invalid parameter passed to function
	@retval	-ENOSPC		can't assign handler to device

	@sa xbee_disc_remove_node_id_handler
 */
int xbee_disc_add_node_id_handler( xbee_dev_t *xbee, xbee_disc_node_id_fn fn);

/**
	@brief Remove a function registered to receive parsed Node ID messages on a
			given XBee device.

	@param[in]	xbee		XBee device to monitor
	@param[in]	fn			function to receive parsed Node ID messages

	@retval	0				function removed
	@retval	-EINVAL		invalid parameter passed to function
	@retval	-ENOENT		function isn't assigned to this XBee module

	@sa xbee_disc_remove_node_id_handler
 */
int xbee_disc_remove_node_id_handler( xbee_dev_t *xbee,
		xbee_disc_node_id_fn fn);

/** @brief Send an ATND command to the XBee, initiating node discovery for
			all nodes or a specific node's "node identification" (ATNI) string.

	Responses from remote nodes are parsed and then passed on to Node ID
	handlers configured with xbee_disc_add_node_id_handler().

	@param[in]	xbee			XBee device to use as the target
	@param[in]	identifier	the ATNI (node identification) string of a device
									on the network, or NULL to discover all devices

	@retval	0			command sent
	@retval	-EINVAL	an invalid parameter was passed to the function
	@retval	-EBUSY	transmit serial buffer is full, or XBee is not accepting
							serial data (deasserting /CTS signal).

	@sa xbee_disc_add_node_id_handler, xbee_disc_remove_node_id_handler
*/
int xbee_disc_discover_nodes( xbee_dev_t *xbee, const char *identifier);

XBEE_END_DECLS

// If compiling in Dynamic C, automatically #use the appropriate C file.
#ifdef __DC__
	#use "xbee_discovery.c"
#endif

#endif
