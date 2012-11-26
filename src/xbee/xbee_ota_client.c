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
	@addtogroup xbee_ota_client
	@{
	@file xbee_ota_client.c

	Support code for over-the-air (OTA) firmware updates of application code
	on Programmable XBee target.

	Preliminary API may change in future releases.
*/
/*** BeginHeader */
#include <string.h>

#include "xbee/platform.h"
#include "xbee/ota_client.h"
#include "xbee/cbuf.h"
/*** EndHeader */

/*** BeginHeader _xbee_ota_transparent_rx */
/*** EndHeader */
/*
	Thoughts on generalizing this function...

	There could be a table of ieee addresses and associated functions to pass
	payloads to.  The last entry of the table would be a "catchall" for
	paylaods from devices not in the table.

	"context" of this function would be the table.  Functions that get the
	payloads would just accept the envelope parameter.

	Maybe better to just have a list of functions that can see all of the
	payloads?
*/
// documented in xbee/ota_client.h
int _xbee_ota_transparent_rx( const wpan_envelope_t FAR *envelope,
	void FAR *context)
{
	xbee_ota_t FAR *ota = context;

	#ifdef XBEE_OTA_VERBOSE
		printf( "%s: got %u-byte transparent data:\n", __FUNCTION__,
			envelope->length);
		hex_dump( envelope->payload, envelope->length, HEX_DUMP_FLAG_TAB);
	#endif

	// flag in xbee_ota_t to ignore data until we're actually doing update?
	if (memcmp( &ota->target, &envelope->ieee_address, 8) == 0)
	{
		xbee_cbuf_put( &ota->rxbuf.cbuf, envelope->payload,
														(uint8_t) envelope->length);
	}
#ifdef XBEE_OTA_VERBOSE
	else
	{
		printf( "%s: ignoring transparent serial data from wrong address\n",
			__FUNCTION__);
	}
#endif

	return 0;
}


/*** BeginHeader xbee_ota_init */
/*** EndHeader */
/** @internal

	Function assigned to the \c stream.read function pointer of an
	xbee_xmodem_state_t object.  Reads data from the target specified in
	the xbee_ota_t structure.

	@param[in]		context	xbee_ota_t structure
	@param[in,out]	buffer	buffer to store read data
	@param[in]		bytes		maximum number of bytes to write to \c buffer

	@sa xbee_xmodem_read_fn()
*/
int _xbee_ota_xmodem_read( void FAR *context, void FAR *buffer, int16_t bytes)
{
	// Function must follow prototype of xbee_xmodem_read_fn, so first
	// parameter is always a void pointer.  Cast it to the correct type.
	xbee_ota_t FAR *ota = context;

	if (context == NULL || buffer == NULL || bytes < 0)
	{
		return -EINVAL;
	}

	return xbee_cbuf_get( &ota->rxbuf.cbuf, buffer,
		(bytes > 255) ? 255 : (uint_fast8_t) bytes);
}

/** @internal

	Function assigned to the \c stream.write function pointer of an
	xbee_xmodem_state_t object.  Sends data to the target specified in
	the xbee_ota_t structure.

	@param[in]		context	xbee_ota_t structure
	@param[in,out]	buffer	source of data to send
	@param[in]		bytes		number of bytes to send

	@return Number of bytes sent to target.

	@sa xbee_xmodem_write_fn()
*/
int _xbee_ota_xmodem_write( void FAR *context, const void FAR *buffer,
																					int16_t bytes)
{
	// Function must follow prototype of xbee_xmodem_write_fn, so first
	// parameter is always a void pointer.  Cast it to the correct type.
	xbee_ota_t FAR *ota = context;
	wpan_envelope_t	envelope;

	if (context == NULL || buffer == NULL || bytes < 0)
	{
		return -EINVAL;
	}

	wpan_envelope_create( &envelope, ota->dev, &ota->target,
																	WPAN_NET_ADDR_UNDEFINED);
	envelope.payload = buffer;
	envelope.length = bytes;
	envelope.options = (ota->flags & XBEE_OTA_FLAG_APS_ENCRYPT)
															? WPAN_CLUST_FLAG_ENCRYPT : 0;

	if (xbee_transparent_serial( &envelope))
	{
		// error on send, try again
		#ifdef XBEE_OTA_VERBOSE
			printf( "%s: %s failed\n", __FUNCTION__, "xbee_transparent_serial");
		#endif
		return 0;
	}

	// successfully sent packet of <bytes> bytes
	#ifdef XBEE_OTA_VERBOSE
		printf ("%s: sent %u bytes\n", __FUNCTION__, bytes);
	#endif

	return bytes;
}

// documented in xbee/ota_client.h
int xbee_ota_init( xbee_ota_t *ota, wpan_dev_t *dev, const addr64 *target)
{
	wpan_envelope_t							envelope;
	int											error;

	if (ota == NULL || dev == NULL || target == NULL)
	{
		return -EINVAL;
	}

	ota->dev = dev;
	ota->target = *target;

	xbee_cbuf_init( &ota->rxbuf.cbuf, 255);

	wpan_envelope_create( &envelope, dev, target, WPAN_NET_ADDR_UNDEFINED);
	envelope.profile_id = WPAN_PROFILE_DIGI;
	envelope.source_endpoint = WPAN_ENDPOINT_DIGI_DATA;
	envelope.dest_endpoint = WPAN_ENDPOINT_DIGI_DATA;
	envelope.cluster_id = DIGI_CLUST_PROG_XBEE_OTA_UPD;
	envelope.options = (ota->flags & XBEE_OTA_FLAG_APS_ENCRYPT) ?
		WPAN_CLUST_FLAG_ENCRYPT : WPAN_SEND_FLAG_NONE;

	if (ota->auth_length)
	{
		envelope.length = ota->auth_length;
		envelope.payload = ota->auth_data;
	}
	else
	{
		// payload must be at least one byte (default to '\0')
		envelope.length = 1;
		envelope.payload = "";
	}

	// tell app to start receiving firmware update
	error = wpan_envelope_send( &envelope);
	if (error)
	{
		return error;
	}

	// Tell bootloader to start receiving in case app isn't running.  Better to
	// do this second, in case application on target is using Transparent
	// Serial for something else.
	envelope.payload = "F";
	envelope.length = 1;
	envelope.cluster_id = DIGI_CLUST_SERIAL;
	wpan_envelope_send( &envelope);

	xbee_xmodem_tx_init( &ota->xbxm, XBEE_XMODEM_FLAG_64);

	return xbee_xmodem_set_stream( &ota->xbxm, _xbee_ota_xmodem_read,
													_xbee_ota_xmodem_write, ota);
}
