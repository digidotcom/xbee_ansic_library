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
	@file xbee/transparent_serial.h

	Support code for the "Digi Transparent Serial" cluster (cluster 0x0011 of
	endpoint 0xE8).
*/

#ifndef XBEE_TRANSPARENT_SERIAL_H
#define XBEE_TRANSPARENT_SERIAL_H

#include "xbee/platform.h"
#include "wpan/aps.h"

XBEE_BEGIN_DECLS

/**
	@brief
	Send data to the "Digi Transparent Serial" cluster (cluster 0x0011 of
	endpoint 0xE8).

	@param[in,out]	envelope	Envelope with device, addresses, payload, length
					and options filled in.  This function sets the profile,
					endpoints, and cluster fields of the envelope.

	@retval	0	data sent
	@retval	!0	error trying to send request

	@sa xbee_transparent_serial_str

	@note This is a preliminary API and may change in a future release.
*/
int xbee_transparent_serial( wpan_envelope_t *envelope);

/**
	@brief
	Send string to the "Digi Transparent Serial" cluster (cluster 0x0011 of
	endpoint 0xE8).

	@param[in,out]	envelope	Envelope with device, addresses and options
									filled in.  This function sets the payload,
									length, profile, endpoints, and cluster fields
									of the envelope.
	@param[in]		data		string to send

	@retval	0	data sent
	@retval	!0	error trying to send request

	@sa xbee_transparent_serial

	@note This is a preliminary API and may change in a future release.
*/
int xbee_transparent_serial_str( wpan_envelope_t *envelope,
																	const char FAR *data);

XBEE_END_DECLS

// If compiling in Dynamic C, automatically #use the appropriate C file.
#ifdef __DC__
	#use "xbee_transparent_serial.c"
#endif

#endif
