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
	@addtogroup xbee_ota_server
	@{
	@file xbee_ota_server.c

	Code to add an OTA Server Cluster to a device.  It receives notification
	to start an update, and then calls back to the bootloader to receive
	that update.
*/

/*** BeginHeader */
#include <stdio.h>
#include "xbee/ota_server.h"
/*** EndHeader */

/*** BeginHeader xbee_ota_server_cmd */
/*** EndHeader */
int xbee_ota_server_cmd( const wpan_envelope_t FAR *envelope,
	void FAR *context)
{
	uint16_t options;
	const char *err = NULL;

	if (envelope == NULL)
	{
		// Stack should not pass NULL envelope; cover this condition for safety
		return -EINVAL;
	}

	// If cluster configuration requires encryption, make sure this
	// frame was received encrypted.

	options = envelope->options &
		(WPAN_ENVELOPE_RX_APS_ENCRYPT | WPAN_CLUST_FLAG_ENCRYPT);
	if (options == WPAN_CLUST_FLAG_ENCRYPT)
	{
		// cluster requires encryption, but frame wasn't encrypted
		// This isn't ZCL, so we can't send a Default Response and we
		// just have to ignore the message.
		err = "encryption required";
	}
	else
	{
		// call function provided by the application for starting the update
		err = xbee_update_firmware_ota( envelope, context);

		// xbee_update_firmware_ota won't return if it's able to install the
		// update and reboot
	}

	if (err)
	{
		wpan_envelope_t reply;

		if (wpan_envelope_reply( &reply, envelope) == 0)
		{
			reply.payload = err;
			reply.length = strlen( err);

			wpan_envelope_send( &reply);
		}
	}

	return 0;
}
