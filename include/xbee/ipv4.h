/*
 * Copyright (c) 2017 Digi International Inc.,
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
	@addtogroup xbee_ipv4
	@{
	@file xbee/ipv4.h

	Support code for XBee IPv4 frames.
*/

/*
	Dev notes:
	
	Consider this a starting point.  I think we could develop something
	based on the wpan/aps.h APIs for using "envelope" structures to
	route frames.  Implementing Berkeley sockets is going to take a bit
	of work (and buffering), but keeping tables for open and listening
	sockets with handlers to receive ipv4_envelope_t structures should
	work well for typical use cases.
*/

#ifndef XBEE_IPV4_H
#define XBEE_IPV4_H

#include "xbee/platform.h"
#include "xbee/device.h"

#if (!XBEE_WIFI_ENABLED) && (!XBEE_CELLULAR_ENABLED)
	#error "At least one of XBEE_WIFI_ENABLED and XBEE_CELLULAR_ENABLED " \
		"must be defined as non-zero to use this header."
#endif

XBEE_BEGIN_DECLS

/// Transmit IPv4 data. [Wi-Fi, Cellular]
#define XBEE_FRAME_TRANSMIT_IPV4			0x20

/// Sent upon receiving IPv4 data. [Wi-Fi, Cellular]
#define XBEE_FRAME_RECEIVE_IPV4			0xB0

#define XBEE_IPV4_PROTOCOL_UDP			0
#define XBEE_IPV4_PROTOCOL_TCP			1
#define XBEE_IPV4_PROTOCOL_SSL			4

#define XBEE_IPV4_TX_OPT_TCP_CLOSE		(1<<1)

#define XBEE_IPV4_MAX_PAYLOAD				1500

/// Header of XBee API frame type 0x20 (#XBEE_FRAME_TRANSMIT_IPV4);
/// sent from host to XBee.
typedef PACKED_STRUCT xbee_header_transmit_ipv4_t {
	uint8_t			frame_type;				///< XBEE_FRAME_TRANSMIT_IPV4 (0x20)
	uint8_t			frame_id;
	uint32_t			remote_addr_be;
	uint16_t			remote_port_be;
	uint16_t			local_port_be;
	uint8_t			protocol;
	uint8_t			options;					///< see XBEE_IPV4_TX_OPT_xxx
} xbee_header_transmit_ipv4_t;

/// Format of XBee API frame type 0xB0 (#XBEE_FRAME_RECEIVE_IPV4);
/// received from XBee by host.
typedef PACKED_STRUCT xbee_frame_receive_ipv4_t {
	uint8_t			frame_type;				///< XBEE_FRAME_RECEIVE_IPV4 (0xB0)
	uint32_t			remote_addr_be;
	uint16_t			local_port_be;
	uint16_t			remote_port_be;
	uint8_t			protocol;
	uint8_t			status;
	uint8_t			payload[1];				///< multi-byte payload
} xbee_frame_receive_ipv4_t;

/*
	Dev notes:
	- We'll keep remote address in big-endian format, since it's easier to
	  always deal with it that way, and we always used 64-bit IEEE addresses
	  on 802.15.4 networks in big-endian byte order.
	- Easy to reply to a message, just copy the start of the envelope.
	  (offsetof(ipv4_envelope_t, options) bytes)
	
	For docs:
	@sa xbee_ipv4_ntoa(), xbee_ipv4_protocol_str()
*/
/* Similar concept to wpan_envelope_t, needs some more documentation. */
typedef struct xbee_ipv4_envelope_t {
	// first group of members are common when creating reply envelope
	xbee_dev_t *xbee;							///< interface received on/to send to
	uint32_t	remote_addr_be;				///< remote device's IP address
	uint16_t local_port;						///< port on this device
	uint16_t remote_port;					///< port on remote device
	uint8_t protocol;							///< protocol for message

	// second group of members are modified when creating reply envelope
	uint16_t options;
	const void FAR *payload;				///< contents of message
	uint16_t length;							///< number of bytes in payload
} xbee_ipv4_envelope_t;


/**
	@brief Address a reply envelope using fields from a received envelope.
	
	@param[out]	reply		envelope structure to fill
	@param[in]	original	received envelope we're replying to
	
	@retval	0			envelope addressed
	@retval	-EINVAL	invalid parameter passed to function
*/
int xbee_ipv4_envelope_reply(xbee_ipv4_envelope_t FAR *reply,
	const xbee_ipv4_envelope_t FAR *original);

	
/**
	@brief Print the contents of an IPv4 envelope to stdout.
*/
void xbee_ipv4_envelope_dump(const xbee_ipv4_envelope_t FAR *envelope,
	bool_t include_payload);


/**
	@brief Send an IPv4 packet.
	
	@param[in]	envelope		Structure with information to send.
	
	@retval	>0				Frame ID of message, to correlate with Transmit
								Status frame received from XBee module.
	@retval	-EINVAL		Invalid parameter passed to function.
	@retval	-E2BIG		Payload too large to send.
	
	@sa xbee_ipv4_envelope_reply()
*/
int xbee_ipv4_envelope_send(const xbee_ipv4_envelope_t FAR *envelope);

/**
	@brief Dump received IPv4 frame to stdout.
	
	This is a sample frame handler that simply prints received IPv4 frame
	to the screen.  Include it in your frame handler table as:
	
		{ XBEE_FRAME_RECEIVE_IPV4, 0, xbee_ipv4_receive_dump, NULL }

	See the function help for xbee_frame_handler_fn() for full
	documentation on this function's API.
*/
int xbee_ipv4_receive_dump(xbee_dev_t *xbee, const void FAR *raw,
	uint16_t length, void FAR *context);

/**
	@brief Format a big-endian IP address from an XBee frame as a
	"dotted quad", four decimal numbers separated by '.' (e.g., "192.0.2.31").
	
	@param[out]	buffer	buffer at least 16-bytes long to hold dotted quad
	@param[in]	ip_be		IP address in big-endian byte order
	
	@retval	0			IP address successfully converted
	@retval	-EINVAL	NULL pointer passed for \a buffer
*/
int xbee_ipv4_ntoa(char buffer[16], uint32_t ip_be);

/**
	@brief Takes an address separated by '.' (e.g., "192.0.2.31") and outputs
	a 32-bit big-endian address
	
	@param[in]	cp			A NULL terminated string containing a dotted quad
	@param[out]	ip_be		IP address in big-endian byte order
	
	@retval	0			IP address successfully converted
	@retval	-EINVAL		Malformed string/ipv4 address	
*/
int xbee_ipv4_aton(const char * cp, uint32_t *ip_be);

/**
	@brief A string description of the "protocol" byte from an XBee IPv4 frame.

	Copies a string to the output buffer (e.g., "TCP") or the pattern "[P99]"
	for unknown protocol values (99 or 0x63) in this example).  Doesn't do
	anything if \a buffer is NULL.
	
	@param[out]	buffer	buffer at least 8-bytes long to hold description
	@param[in]	protocol	protocol byte from an XBee IPv4 frame
	
	@retval				buffer passed to function
*/
char *xbee_ipv4_protocol_str(char buffer[8], uint8_t protocol);

XBEE_END_DECLS

// If compiling in Dynamic C, automatically #use the appropriate C file.
#ifdef __DC__
	#use "xbee_sms.c"
#endif

#endif
