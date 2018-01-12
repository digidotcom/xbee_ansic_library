/*
 * Copyright (c) 2008-2013 Digi International Inc.,
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
	@addtogroup xbee_wpan
	@{
	@file xbee/wpan.h
*/

#ifndef __XBEE_WPAN_H
#define __XBEE_WPAN_H

#include "xbee/device.h"
#include "wpan/types.h"
#include "xbee/delivery_status.h"

XBEE_BEGIN_DECLS

/// Format of XBee API frame type 0x90 (#XBEE_FRAME_RECEIVE);
/// received from XBee by host.
typedef PACKED_STRUCT xbee_frame_receive_t {
	uint8_t			frame_type;				///< XBEE_FRAME_RECEIVE (0x90)
	addr64			ieee_address;
	uint16_t			network_address_be;
	uint8_t			options;					///< bitfield, see XBEE_RX_OPT_xxx macros
	uint8_t			payload[1];				///< multi-byte payload
} xbee_frame_receive_t;


/// Format of XBee API frame type 0x91 (#XBEE_FRAME_RECEIVE_EXPLICIT);
/// received from XBee by host.
typedef PACKED_STRUCT xbee_frame_receive_explicit_t {
	uint8_t			frame_type;				///< XBEE_FRAME_RECEIVE_EXPLICIT (0x91)
	addr64			ieee_address;
	uint16_t			network_address_be;
	uint8_t			source_endpoint;
	uint8_t			dest_endpoint;
	uint16_t			cluster_id_be;
	uint16_t			profile_id_be;
	uint8_t			options;					///< bitfield, see XBEE_RX_OPT_xxx macros
	uint8_t			payload[1];				///< multi-byte payload
} xbee_frame_receive_explicit_t;

int xbee_wpan_init( xbee_dev_t *xbee,
	const wpan_endpoint_table_entry_t *ep_table);

#define XBEE_FRAME_HANDLE_RX_EXPLICIT	\
	{ XBEE_FRAME_RECEIVE_EXPLICIT, 0, _xbee_handle_receive_explicit, NULL }

int _xbee_handle_receive_explicit( xbee_dev_t *xbee, const void FAR *raw,
	uint16_t length, void FAR *context);

/// Format of XBee API frame type 0x10 (#XBEE_FRAME_TRANSMIT); sent
/// from host to XBee.  Note that the network stack does not include a
/// function for sending this frame type -- use an explicit transmit frame
/// instead (type 0x11) with WPAN_ENDPOINT_DIGI_DATA as the source and
/// destination endpoint, DIGI_CLUST_SERIAL as the cluster ID and
/// WPAN_PROFILE_DIGI as the profile ID.  Or, use the xbee_transparent_serial()
/// function from xbee_transparent_serial.c to fill in those fields and
/// send the data.
typedef PACKED_STRUCT xbee_header_transmit_t {
	uint8_t			frame_type;				///< XBEE_FRAME_TRANSMIT (0x10)
	uint8_t			frame_id;
	addr64			ieee_address;
	uint16_t			network_address_be;
	uint8_t			broadcast_radius;		///< set to 0 for maximum hop value
	uint8_t			options;					///< combination of XBEE_TX_OPT_* macros
} xbee_header_transmit_t;

/// Format of XBee API frame type 0x11 (#XBEE_FRAME_TRANSMIT_EXPLICIT); sent
/// from host to XBee.
typedef PACKED_STRUCT xbee_header_transmit_explicit_t {
	uint8_t			frame_type;				///< XBEE_FRAME_TRANSMIT_EXPLICIT (0x11)
	uint8_t			frame_id;
	addr64			ieee_address;
	uint16_t			network_address_be;
	uint8_t			source_endpoint;
	uint8_t			dest_endpoint;
	uint16_t			cluster_id_be;
	uint16_t			profile_id_be;
	uint8_t			broadcast_radius;		///< set to 0 for maximum hop value
	uint8_t			options;					///< combination of XBEE_TX_OPT_* macros
} xbee_header_transmit_explicit_t;

/*	@name
	Options for \c options field of xbee_header_transmit_t and
	xbee_header_transmit_explicit_t.
	@{
*/
/// XBee Transmit Option: Disable ACK [ZigBee and DigiMesh]
#define XBEE_TX_OPT_DISABLE_ACK				0x01
/// XBee Transmit Option: Disable Route Discovery [DigiMesh]
#define XBEE_TX_OPT_DISABLE_ROUTE_DISC		0x02
/// XBee Transmit Option: Enable Unicast NACK messages [DigiMesh]
#define XBEE_TX_OPT_ENABLE_UNICAST_NACK	0x04
/// XBee Transmit Option: Enable Unicast Trace Route messages [DigiMesh]
#define XBEE_TX_OPT_ENABLE_UNICAST_TRACE	0x08
/// XBee Transmit Option: Enable APS encryption (if EE=1) [ZigBee]
#define XBEE_TX_OPT_APS_ENCRYPT				0x20
/// XBee Transmit Option: Use extended timeout for this destination. [ZigBee]
#define XBEE_TX_OPT_EXTENDED_TIMEOUT		0x40
/// XBee Transmit Option: Point-Multipoint [DigiMesh]
#define XBEE_TX_OPT_MODE_POINT_MULTIPOINT	(1<<6)
/// XBee Transmit Option: Repeater mode (directed broadcast) [DigiMesh]
#define XBEE_TX_OPT_MODE_REPEATER			(2<<6)
/// XBee Transmit Option: DigiMesh (not available on 10k product) [DigiMesh]
#define XBEE_TX_OPT_MODE_DIGIMESH			(3<<6)
//@}

typedef PACKED_STRUCT xbee_frame_transmit_status_t {
	uint8_t			frame_type;			//< XBEE_FRAME_TRANSMIT_STATUS (0x8B)
	uint8_t			frame_id;
	uint16_t			network_address_be;
	uint8_t			retries;			// # of application Tx retries
	uint8_t			delivery; /// See xbee/delivery_status.h
	uint8_t			discovery;		// bitfield
/** @name
	Values for \c discovery member of xbee_frame_transmit_status_t.
	@{
*/
	/// XBee Transmit Discovery Status: No Discovery Overhead [ZigBee, DigiMesh]
		#define XBEE_TX_DISCOVERY_NONE				 0x00

	/// XBee Transmit Discovery Status: Address Discovery [ZigBee]
		#define XBEE_TX_DISCOVERY_ADDRESS			 0x01

	/// XBee Transmit Discovery Status: Route Discovery [ZigBee, DigiMesh]
		#define XBEE_TX_DISCOVERY_ROUTE				 0x02

	/// XBee Transmit Discovery Status: Extended Timeout Discovery [ZigBee]
		#define XBEE_TX_DISCOVERY_EXTENDED_TIMEOUT 0x40
//@}

} xbee_frame_transmit_status_t;

/**
	@internal
	Dummy frame handler for 0x8B (XBEE_FRAME_TRANSMIT_STATUS) frames.
	Placeholder until we integrate processing of those frames into the stack.
	@see xbee_frame_handler_fn()

	@todo Figure out what needs to happen with these frames.  Does atcmd
			layer need to pass errors down to callbacks?  Registered endpoints?
			Tracked conversations?
*/
int _xbee_handle_transmit_status( xbee_dev_t *xbee,
	const void FAR *frame, uint16_t length, void FAR *context);

#define XBEE_FRAME_HANDLE_TRANSMIT_STATUS	\
	{ XBEE_FRAME_TRANSMIT_STATUS, 0, _xbee_handle_transmit_status, NULL }

/**
	@brief
	Frame handler for 0x8B (XBEE_FRAME_TRANSMIT_STATUS) frames -- dumps transmit
	status to STDOUT for debugging purposes.

	View the documentation of xbee_frame_handler_fn() for this function's
	parameters and return value.

	@see XBEE_FRAME_TRANSMIT_STATUS_DEBUG, xbee_frame_handler_fn()
*/
int xbee_frame_dump_transmit_status( xbee_dev_t *xbee,
	const void FAR *frame, uint16_t length, void FAR *context);

/**
	Add this macro to the list of XBee frame handlers to have transmit status
	frames dumped to STDOUT.
*/
#define XBEE_FRAME_TRANSMIT_STATUS_DEBUG \
	{ XBEE_FRAME_TRANSMIT_STATUS, 0, xbee_frame_dump_transmit_status, NULL }

XBEE_END_DECLS

// If compiling in Dynamic C, automatically #use the appropriate C file.
#ifdef __DC__
	#use "xbee_wpan.c"
#endif

#endif
