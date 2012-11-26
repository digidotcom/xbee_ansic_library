/*
 * Copyright (c) 2012 Digi International Inc.,
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
	@addtogroup xbee_route
	@{
	@file xbee/route.h
	Receive inbound route records, and send "Create Source Route" frames.
*/


#ifndef XBEE_ROUTE_H
#define XBEE_ROUTE_H

#include "xbee/device.h"

XBEE_BEGIN_DECLS

/// Number of addresses that can appear in a Create Source Route request
/// or a Route Record Indicator frame.
#define XBEE_ROUTE_MAX_ADDRESS_COUNT			11

#define XBEE_FRAME_CREATE_SOURCE_ROUTE			0x21
typedef PACKED_STRUCT xbee_frame_create_source_route_t
{
	/// #XBEE_FRAME_CREATE_SOURCE_ROUTE (0x21)
	uint8_t				frame_type;

	/// ID, always set to 0
	uint8_t				frame_id;

	/// 64-bit IEEE address (big-endian) of destination.
	addr64				ieee_address;

	/// 16-bit network address (big-endian) of destination.
	uint16_t				network_address_be;

	/// Always set to XBEE_ROUTE_OPTIONS_NONE.
	uint8_t				route_options;
		#define XBEE_ROUTE_OPTIONS_NONE				0x00
		
	/// Number of addresses in route, excluding source and destination.
	/// Must be between 1 and XBEE_ROUTE_MAX_ADDRESS_COUNT.
	uint8_t				address_count;
	
	/// Network addresses of nodes in route, starting next to destination
	/// and ending with node next to source (us).
	uint16_t				route_address_be[XBEE_ROUTE_MAX_ADDRESS_COUNT];
} xbee_frame_create_source_route_t;


#define XBEE_FRAME_ROUTE_RECORD_INDICATOR		0xA1
typedef PACKED_STRUCT xbee_frame_route_record_indicator_t
{
	/// #XBEE_FRAME_ROUTE_RECORD_INDICATOR (0xA1)
	uint8_t				frame_type;

	/// 64-bit IEEE address (big-endian) of source.
	addr64				ieee_address;

	/// 16-bit network address (big-endian) of source.
	uint16_t				network_address_be;

	/// Combination of XBEE_RX_OPT_XXX macro values.
	uint8_t				receive_options;
		
	/// Number of addresses in route, excluding source and destination.
	/// Must be between 1 and XBEE_ROUTE_MAX_ADDRESS_COUNT.
	uint8_t				address_count;
	
	/// Network addresses of nodes in route, starting next to destination
	/// and ending with node next to source (us).
	uint16_t				route_address_be[XBEE_ROUTE_MAX_ADDRESS_COUNT];
} xbee_frame_route_record_indicator_t;

int xbee_route_dump_record_indicator( xbee_dev_t *xbee,
	const void FAR *frame, uint16_t length, void FAR *context);

/**
	Add this macro to the list of XBee frame handlers to have route record
	indicators dumped to STDOUT.
*/
#define XBEE_ROUTE_DUMP_RECORD_INDICATOR	\
	{	XBEE_FRAME_ROUTE_RECORD_INDICATOR, 0, \
		xbee_route_dump_record_indicator, NULL }


#define XBEE_FRAME_ROUTE_MANY_TO_ONE_REQ		0xA3
/// The many-to-one route request indicator frame is sent out the serial port
/// whenever a many-to-one route request is received
typedef PACKED_STRUCT xbee_frame_route_many_to_one_req_t
{
	/// #XBEE_FRAME_ROUTE_MANY_TO_ONE_REQ (0xA3)
	uint8_t				frame_type;

	/// 64-bit IEEE address (big-endian) of source.
	addr64				ieee_address;

	/// 16-bit network address (big-endian) of source.
	uint16_t				network_address_be;

	/// Reserved field always set to 0.
	uint8_t				reserved;
} xbee_frame_route_many_to_one_req_t;

int xbee_route_dump_many_to_one_req( xbee_dev_t *xbee,
	const void FAR *frame, uint16_t length, void FAR *context);

/**
	Add this macro to the list of XBee frame handlers to have route record
	indicators dumped to STDOUT.
*/
#define XBEE_ROUTE_DUMP_MANY_TO_ONE_REQ	\
	{	XBEE_FRAME_ROUTE_MANY_TO_ONE_REQ, 0, \
		xbee_route_dump_many_to_one_req, NULL }

XBEE_END_DECLS

// If compiling in Dynamic C, automatically #use the appropriate C file.
#ifdef __DC__
	#use "xbee_route.c"
#endif

#endif

