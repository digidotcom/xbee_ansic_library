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
	@file xbee_route.c
	Receive inbound route records, and send "Create Source Route" frames.
*/

/*** BeginHeader */
#include <stddef.h>
#include <stdio.h>
#include "xbee/byteorder.h"
#include "xbee/route.h"
/*** EndHeader */

/*** BeginHeader xbee_route_dump_record_indicator */
/*** EndHeader */
int xbee_route_dump_record_indicator( xbee_dev_t *xbee,
	const void FAR *frame, uint16_t length, void FAR *context)
{
	const xbee_frame_route_record_indicator_t FAR *record = frame;
	char buffer[ADDR64_STRING_LENGTH];
	const uint16_t FAR *addr_be;
	int i;
	
	if (frame == NULL)
	{
		return -EINVAL;
	}
	if (record->address_count == 0
		&& record->address_count > XBEE_ROUTE_MAX_ADDRESS_COUNT)
	{
		printf( "invalid address count, must be 1 to %u (not %u)\n",
			XBEE_ROUTE_MAX_ADDRESS_COUNT, record->address_count);
	}
	else if (length ==
			offsetof( xbee_frame_route_record_indicator_t, route_address_be) +
			record->address_count * sizeof *record->route_address_be)
	{
		printf( "route record indicator from %" PRIsFAR " (0x%04X):\n",
			addr64_format( buffer, &record->ieee_address),
			be16toh( record->network_address_be));
		if (record->receive_options & XBEE_RX_OPT_ACKNOWLEDGED)
		{
			printf( "acknowledged, ");
		}
		if (record->receive_options & XBEE_RX_OPT_BROADCAST)
		{
			printf( "broadcast, ");
		}
		printf( "\t%u addresses:\n\tdest -> ", record->address_count);
		addr_be = record->route_address_be;
		for (i = record->address_count; i; ++addr_be, --i)
		{
			printf( "0x%04X -> ", be16toh( *addr_be));
		}
		puts( "source");

		return 0;
	}
	
	puts( "malformed route record indicator:");
	hex_dump( frame, length, HEX_DUMP_FLAG_TAB);
	return -EINVAL;
}

/*** BeginHeader xbee_route_dump_many_to_one_req */
/*** EndHeader */
int xbee_route_dump_many_to_one_req( xbee_dev_t *xbee,
	const void FAR *frame, uint16_t length, void FAR *context)
{
	const xbee_frame_route_many_to_one_req_t FAR *request = frame;
	char buffer[ADDR64_STRING_LENGTH];
	
	if (frame != NULL && length == sizeof *request)
	{
		printf( "many to one route request from %" PRIsFAR " (0x%04X):\n",
			addr64_format( buffer, &request->ieee_address),
			be16toh( request->network_address_be));
		return 0;
	}

	puts( "malformed many-to-one route request:");
	hex_dump( frame, length, HEX_DUMP_FLAG_TAB);
	return -EINVAL;
}


