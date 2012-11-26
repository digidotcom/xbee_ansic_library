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
	@addtogroup xbee_transparent
	@{
	@file xbee_transparent_serial.c

	Support code for the "Digi Transparent Serial" cluster (cluster 0x0011 of
	endpoint 0xE8).
*/

/*** BeginHeader */
#include <string.h>

#include "xbee/platform.h"
#include "xbee/transparent_serial.h"
/*** EndHeader */

/*** BeginHeader xbee_transparent_serial */
/*** EndHeader */
// documented in xbee/transparent_serial.h
int xbee_transparent_serial( wpan_envelope_t *envelope)
{
	// fill in rest of envelope
	envelope->profile_id = WPAN_PROFILE_DIGI;
	envelope->source_endpoint = envelope->dest_endpoint
																= WPAN_ENDPOINT_DIGI_DATA;
	envelope->cluster_id = DIGI_CLUST_SERIAL;

	return wpan_envelope_send( envelope);
}

/*** BeginHeader xbee_transparent_serial_str */
/*** EndHeader */
// documented in xbee/transparent_serial.h
int xbee_transparent_serial_str( wpan_envelope_t *envelope,
																	const char FAR *data)
{
	envelope->payload = data;
	envelope->length = strlen( data);

	return xbee_transparent_serial( envelope);
}

